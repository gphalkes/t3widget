/* Copyright (C) 2011-2013 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#ifdef HAS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <t3widget/extclipboard.h>
#include <t3widget/log.h>
#include <t3widget/ptr.h>

// FIXME: remove incr_sends on long periods of inactivity
/* This file contains parallel implementation of the X11 integration by use of
   Xlib or XCB.

   To maintain the same implementation method for both Xlib and XCB, a small
   sacrifice to good coding practice has been made in the XCB implementation:
   the reply from xcb_get_property_reply is saved in a global variable, and
   free'd in x11_free_property_data. The last routine _ignores_ its argument.
*/
#ifdef USE_XLIB
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#else
#include <xcb/xcb.h>
#endif

namespace t3_widget {

using timeout_t = std::chrono::time_point<std::chrono::system_clock>;
static timeout_t timeout_time(int microseconds) {
  return std::chrono::system_clock::now() + std::chrono::microseconds(microseconds);
}

#define DATA_BLOCK_SIZE 4000

// Minimal set of typedefs and definitions to allow the common parts of the x11_imp_t class
// to be implemented in a separate base class.
#ifdef USE_XLIB
typedef Atom x11_atom_t;
typedef Window x11_window_t;
#else
using x11_atom_t = xcb_atom_t;
using x11_window_t = xcb_window_t;
#endif

static const char *atom_names[] = {
    "CLIPBOARD",     "PRIMARY", "TARGETS",   "TIMESTAMP", "MULTIPLE", "UTF8_STRING",
    "GDK_SELECTION",  // Use same name as GDK to save on atoms
    "INCR",          "ATOM",    "ATOM_PAIR",
};
enum {
  CLIPBOARD,
  PRIMARY,
  /* The following 4 targets must remain in this order and consecutive. */
  TARGETS,
  TIMESTAMP,
  MULTIPLE,
  UTF8_STRING,

  GDK_SELECTION,
  INCR,
  ATOM,
  ATOM_PAIR,
};
#define ATOM_COUNT (sizeof(atom_names) / sizeof(atom_names[0]))

class x11_base_t {
 public:
  ~x11_base_t() = default;

  bool init() {
    if (pipe(wakeup_pipe) < 0) {
      return false;
    }
    x11_initialized = true;
    return true;
  }

  bool is_initialized() const { return x11_initialized; }
  bool has_error() const { return x11_error; }

  void x11_acknowledge_wakeup(fd_set *fds) {
    /* Clear data from wake-up pipe */
    if (FD_ISSET(wakeup_pipe[0], fds)) {
      char buffer[8];
      read(wakeup_pipe[0], buffer, sizeof(buffer));
    }
  }

  int x11_fill_fds(fd_set *fds) {
    FD_ZERO(fds);
    FD_SET(wakeup_pipe[0], fds);
    return wakeup_pipe[0] + 1;
  }

  void send_wakeup() { write(wakeup_pipe[1], &wakeup_pipe, 1); }

  x11_atom_t get_atom(int atom) { return atoms[atom]; }
  x11_window_t get_window() { return window; }
  size_t get_max_data() { return max_data; }
  /* The atoms are arranged such that the targets we have available are consecutive. */
  unsigned char *get_targets_list() { return reinterpret_cast<unsigned char *>(&atoms[TARGETS]); }

 protected:
  bool x11_initialized = false;
  bool x11_error = false;
  int wakeup_pipe[2]{-1, -1};
  size_t max_data = 0;
  x11_window_t window = 0;
  x11_atom_t atoms[ATOM_COUNT];
};

#ifdef USE_XLIB
typedef Time x11_time_t;

typedef XEvent x11_event_t;
typedef XPropertyEvent x11_property_event_t;
typedef XSelectionEvent x11_selection_event_t;
typedef XSelectionClearEvent x11_selection_clear_event_t;
typedef XSelectionRequestEvent x11_selection_request_event_t;

#define X11_PROPERTY_REPLACE PropModeReplace
#define X11_PROPERTY_APPEND PropModeAppend
#define X11_PROPERTY_PREPEND PropModePrepend
#define X11_CURRENT_TIME CurrentTime
#define X11_ATOM_WM_NAME XA_WM_NAME
#define X11_ATOM_STRING XA_STRING
#define X11_PROPERTY_NOTIFY PropertyNotify
#define X11_SELECTION_NOTIFY SelectionNotify
#define X11_SELECTION_CLEAR SelectionClear
#define X11_SELECTION_REQUEST SelectionRequest
#define X11_ATOM_NONE None
#define X11_PROPERTY_NEW_VALUE PropertyNewValue
#define X11_PROPERTY_DELETE PropertyDelete
#define x11_response_type type

class x11_impl_t : public x11_base_t {
 public:
  x11_impl_t() : display(nullptr) {}
  ~x11_impl_t() {
    if (display != nullptr) x11_close_display();
  }

  bool init_x11() {
    x11_window_t root;
    char **atom_names_local = nullptr;
    int black;
    size_t i;

    /* The XInternAtoms call expects an array of char *, not of const char *. Thus
       we make a copy here, which we later discard again. */
    if ((atom_names_local = (char **)malloc(sizeof(char *) * ATOM_COUNT)) == nullptr) return false;

    /* First initialize all the pointers to nullptr, such that we can do a simple goto
       error_exit if some allocation fails. */
    for (i = 0; i < ATOM_COUNT; i++) atom_names_local[i] = nullptr;
    for (i = 0; i < ATOM_COUNT; i++) {
      if ((atom_names_local[i] = strdup(atom_names[i])) == nullptr) goto error_exit;
    }

    // Make sure X11 errors don't abort the program
    XSetErrorHandler(error_handler);
    XSetIOErrorHandler(io_error_handler);

    if ((display = XOpenDisplay(nullptr)) == nullptr) goto error_exit;

    max_data = XMaxRequestSize(display);
    if (max_data > DATA_BLOCK_SIZE * 4 + 100)
      max_data = DATA_BLOCK_SIZE * 4;
    else
      max_data = max_data - 100;

    root = XDefaultRootWindow(display);
    black = BlackPixel(display, DefaultScreen(display));
    window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, black, black);

    /* Discard all events, but process errors. */
    XSync(display, True);
    if (x11_error) goto error_exit;

    if (!XInternAtoms(display, atom_names_local, ATOM_COUNT, False, atoms)) goto error_exit;

    for (i = 0; i < ATOM_COUNT; i++) free(atom_names_local[i]);
    free(atom_names_local);
    atom_names_local = nullptr;

    x11_select_prop_change(window, true);

    /* Discard all events, but process errors. */
    XSync(display, True);
    if (x11_error) goto error_exit;

    x11_initialized = true;
    if (!x11_base_t::init()) goto error_exit;

    lprintf("X11 interface initialized\n");
    return true;

  error_exit:
    if (atom_names_local != nullptr) {
      for (i = 0; i < ATOM_COUNT; i++) free(atom_names_local[i]);
      free(atom_names_local);
    }
    if (display != nullptr) XCloseDisplay(display);
    return false;
  }

  void x11_flush() {
    XFlush(display);
    write(wakeup_pipe[1], &wakeup_pipe, 1);
  }

  void x11_set_selection_owner(x11_atom_t selection, x11_window_t win, x11_time_t since) {
    XSetSelectionOwner(display, selection, win, since);
  }

  x11_window_t x11_get_selection_owner(x11_atom_t selection) {
    return XGetSelectionOwner(display, selection);
  }

  void x11_change_property(x11_window_t win, x11_atom_t property, x11_atom_t type, int format,
                           int mode, const unsigned char *data, int nelements) {
    XChangeProperty(display, win, property, type, format, mode, data, nelements);
  }

  bool x11_get_window_property(x11_window_t win, x11_atom_t property, long long_offset,
                               long long_length, bool del, x11_atom_t req_type,
                               x11_atom_t *actual_type_return, int *actual_format_return,
                               unsigned long *nitems_return, unsigned long *bytes_after_return,
                               unsigned char **prop_return) {
    return XGetWindowProperty(display, win, property, long_offset, long_length, del, req_type,
                              actual_type_return, actual_format_return, nitems_return,
                              bytes_after_return, prop_return) == Success;
  }

  void x11_select_prop_change(x11_window_t win, bool interested) {
    XSelectInput(display, win, interested ? PropertyChangeMask : 0);
  }

  void x11_free_property_data(void *data) { XFree(data); }

  void x11_delete_property(x11_window_t win, x11_atom_t property) {
    XDeleteProperty(display, win, property);
  }

  void x11_convert_selection(x11_atom_t selection, x11_atom_t type, x11_atom_t property,
                             x11_window_t win, x11_time_t req_time) {
    XConvertSelection(display, selection, type, property, win, req_time);
  }

  void x11_send_event(x11_window_t win, bool propagate, long event_mask, x11_event_t *event_send) {
    XSendEvent(display, win, propagate, event_mask, event_send);
  }

  void x11_close_display() {
    XCloseDisplay(display);
    display = nullptr;
  }

  x11_event_t *x11_probe_event() {
    if (!XPending(display)) return nullptr;
    XNextEvent(display, &last_event);
    return &last_event;
  }

  void x11_free_event(x11_event_t *event) { (void)event; }

  int x11_fill_fds(fd_set *fds) {
    int max_from_base = x11_base_t::x11_fill_fds(fds);
    FD_SET(ConnectionNumber(display), fds);
    return std::max(max_from_base, ConnectionNumber(display) + 1);
  }

  /** Handle error reports, i.e. BadAtom and such, from the server. */
  static int error_handler(Display *_display, XErrorEvent *error);

  /** Handle IO errors. */
  static int io_error_handler(Display *_display);

 private:
  Display *display;
  XEvent last_event;
};

#else
using x11_time_t = xcb_timestamp_t;

using x11_event_t = xcb_generic_event_t;
using x11_property_event_t = xcb_property_notify_event_t;
using x11_selection_event_t = xcb_selection_notify_event_t;
using x11_selection_clear_event_t = xcb_selection_clear_event_t;
using x11_selection_request_event_t = xcb_selection_request_event_t;

#define X11_PROPERTY_REPLACE XCB_PROP_MODE_REPLACE
#define X11_PROPERTY_APPEND XCB_PROP_MODE_APPEND
#define X11_PROPERTY_PREPEND XCB_PROP_MODE_PREPEND
#define X11_CURRENT_TIME XCB_TIME_CURRENT_TIME
#define X11_ATOM_WM_NAME XCB_ATOM_WM_NAME
#define X11_ATOM_STRING XCB_ATOM_STRING
#define X11_PROPERTY_NOTIFY XCB_PROPERTY_NOTIFY
#define X11_SELECTION_NOTIFY XCB_SELECTION_NOTIFY
#define X11_SELECTION_CLEAR XCB_SELECTION_CLEAR
#define X11_SELECTION_REQUEST XCB_SELECTION_REQUEST
#define X11_ATOM_NONE XCB_ATOM_NONE
#define X11_PROPERTY_NEW_VALUE XCB_PROPERTY_NEW_VALUE
#define X11_PROPERTY_DELETE XCB_PROPERTY_DELETE
#define x11_response_type response_type

class x11_impl_t : public x11_base_t {
 public:
  ~x11_impl_t() {
    if (property_reply != nullptr) {
      x11_free_property_data(nullptr);
    }
    if (connection != nullptr) {
      x11_close_display();
    }
  }

  bool init_x11() {
    uint32_t values[2];
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    xcb_generic_error_t *error = nullptr;
    cleanup_func_ptr<xcb_connection_t, xcb_disconnect>::t local_connection;
    xcb_screen_t *screen;
    xcb_intern_atom_cookie_t cookies[ATOM_COUNT];
    size_t i;

    if ((local_connection = xcb_connect(nullptr, nullptr)) == nullptr) {
      return false;
    }

    for (i = 0; i < ATOM_COUNT; i++) {
      cookies[i] = xcb_intern_atom(local_connection, 0, strlen(atom_names[i]), atom_names[i]);
    }
    for (i = 0; i < ATOM_COUNT; i++) {
      xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(local_connection, cookies[i], nullptr);
      if (reply == nullptr) {
        return false;
      }

      atoms[i] = reply->atom;
      free(reply);
    }

    max_data = xcb_get_maximum_request_length(local_connection);
    if (max_data > DATA_BLOCK_SIZE * 4 + 100) {
      max_data = DATA_BLOCK_SIZE * 4;
    } else {
      max_data = max_data - 100;
    }

    screen = xcb_setup_roots_iterator(xcb_get_setup(local_connection)).data;
    window = xcb_generate_id(local_connection);
    values[0] = screen->white_pixel;
    values[1] = XCB_EVENT_MASK_PROPERTY_CHANGE;

    if ((error = xcb_request_check(
             local_connection,
             xcb_create_window_checked(local_connection, 0, window, screen->root, 0, 0, 1, 1, 0,
                                       XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask,
                                       values))) != nullptr) {
      free(error);
      return false;
    }

    xcb_flush(local_connection);

    if (!x11_base_t::init()) {
      return false;
    }

    x11_initialized = true;
    connection = local_connection.release();

    lprintf("X11 interface initialized\n");
    return true;
  }

  void x11_flush() { xcb_flush(connection); }

  void x11_set_selection_owner(x11_atom_t selection, x11_window_t win, x11_time_t since) {
    xcb_set_selection_owner(connection, win, selection, since);
  }

  x11_window_t x11_get_selection_owner(x11_atom_t selection) {
    xcb_window_t result;
    xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(connection, selection);
    xcb_get_selection_owner_reply_t *reply =
        xcb_get_selection_owner_reply(connection, cookie, nullptr);

    if (reply == nullptr) {
      x11_error = true;
      return XCB_WINDOW_NONE;
    }

    result = reply->owner;
    free(reply);
    return result;
  }

  void x11_change_property(x11_window_t win, x11_atom_t property, x11_atom_t type, int format,
                           int mode, const unsigned char *data, int nelements) {
    xcb_change_property(connection, mode, win, property, type, format, nelements, data);
  }

  bool x11_get_window_property(x11_window_t win, x11_atom_t property, long long_offset,
                               long long_length, bool del, x11_atom_t req_type,
                               x11_atom_t *actual_type_return, int *actual_format_return,
                               unsigned long *nitems_return, unsigned long *bytes_after_return,
                               unsigned char **prop_return) {
    xcb_get_property_cookie_t cookie =
        xcb_get_property(connection, del, win, property, req_type, long_offset, long_length);
    property_reply = xcb_get_property_reply(connection, cookie, nullptr);

    if (property_reply == nullptr) {
      return false;
    }

    *nitems_return = xcb_get_property_value_length(property_reply);
    *prop_return = (unsigned char *)xcb_get_property_value(property_reply);
    *actual_type_return = property_reply->type;
    *actual_format_return = property_reply->format;
    *bytes_after_return = property_reply->bytes_after;
    return true;
  }

  void x11_select_prop_change(x11_window_t win, bool interested) {
    uint32_t value = interested ? XCB_EVENT_MASK_PROPERTY_CHANGE : 0;
    xcb_change_window_attributes(connection, win, XCB_CW_EVENT_MASK, &value);
  }

  void x11_free_property_data(void *data) {
    (void)data;
    free(property_reply);
    property_reply = nullptr;
  }

  void x11_delete_property(x11_window_t win, x11_atom_t property) {
    xcb_delete_property(connection, win, property);
  }

  void x11_convert_selection(x11_atom_t selection, x11_atom_t type, x11_atom_t property,
                             x11_window_t win, x11_time_t req_time) {
    xcb_convert_selection(connection, win, selection, type, property, req_time);
  }

  void x11_send_event(x11_window_t win, bool propagate, long event_mask, x11_event_t *event_send) {
    xcb_send_event(connection, propagate, win, event_mask, (const char *)event_send);
  }

  void x11_close_display() {
    xcb_disconnect(connection);
    connection = nullptr;
  }

  x11_event_t *x11_probe_event() {
    x11_event_t *result;
    xcb_flush(connection);
    result = xcb_poll_for_event(connection);
    /* FIXME: we really should figure out what causes this to happen, and if we can
       recover. But for now we just set the x11_error to true, to prevent the
       interface from becoming unresponsive. */
    if (result == nullptr && xcb_connection_has_error(connection)) {
      x11_error = true;
    }
    return result;
  }

  void x11_free_event(x11_event_t *event) { free(event); }

  int x11_fill_fds(fd_set *fds) {
    int max_from_parent = x11_base_t::x11_fill_fds(fds);
    int fd = xcb_get_file_descriptor(connection);
    FD_SET(fd, fds);
    return std::max(max_from_parent, fd + 1);
  }

 private:
  xcb_connection_t *connection = nullptr;
  xcb_get_property_reply_t *property_reply = nullptr;
};
#endif

//============================= END OF X11 IMPLEMENTATION SPECIFIC CODE ========================

class x11_driver_t {
 public:
  x11_driver_t() : clipboard_mutex_lock(clipboard_mutex, std::defer_lock_t()) {}

  bool init_x11() {
    if (!x11.init_x11()) {
      return false;
    }
    x11_event_thread = std::thread(process_events_wrapper);
    return true;
  }

  void lock() { clipboard_mutex_lock.lock(); }

  void unlock() { clipboard_mutex_lock.unlock(); }

  /** Stop the X11 event processing. */
  void stop_x11() {
    if (!x11.is_initialized()) {
      return;
    }

    clipboard_mutex.lock();
    end_connection = true;
    /* If x11_error has been set, the event handling thread will stop, or will
       have stopped already. Also, if this is the case, the connection is broken,
       which means we can't send anything anyway. Thus we skip the client message
       if x11_error is set. */
    if (!x11.has_error()) {
      x11.x11_close_display();
    }

    clipboard_mutex.unlock();
    x11.send_wakeup();
    x11_event_thread.join();
  }

#define x11_working() (x11.is_initialized() && !x11.has_error())

  linked_ptr<std::string>::t get_selection(bool clipboard) {
    timeout_t timeout = timeout_time(1000000);
    linked_ptr<std::string>::t result;

    /* NOTE: the clipboard is supposed to be locked when this routine is called. */

    /* If X11 was not initialized, or an IO error occured, we can skip the stuff
       below, because it won't work. */
    if (!x11_working()) {
      return clipboard ? clipboard_data : primary_data;
    }

    /* If we currently own the selection that is requested, there is no need to go
       through the X server. */
    if ((clipboard && clipboard_owner_since == X11_CURRENT_TIME) ||
        (!clipboard && primary_owner_since == X11_CURRENT_TIME)) {
      action = clipboard ? CONVERT_CLIPBOARD : CONVERT_PRIMARY;
      x11.x11_change_property(x11.get_window(), X11_ATOM_WM_NAME, X11_ATOM_STRING, 8,
                              X11_PROPERTY_APPEND, nullptr, 0);
      x11.x11_flush();
      if (clipboard_signal.wait_until(clipboard_mutex_lock, timeout) != std::cv_status::timeout &&
          conversion_succeeded) {
        result = new std::string(retrieved_data);
      }
      action = ACTION_NONE;
    } else {
      result = clipboard ? clipboard_data : primary_data;
    }
    return result;
  }

  void claim_selection(bool clipboard, std::string *data) {
    timeout_t timeout = timeout_time(1000000);

    if (!x11_working()) {
      if (clipboard) {
        clipboard_data = data;
      } else {
        primary_data = data;
      }
      return;
    }

    std::unique_lock<std::mutex> l(clipboard_mutex);

    if (clipboard) {
      /* If we don't own the selection, reseting is a no-op. */
      if (clipboard_owner_since == X11_CURRENT_TIME && data == nullptr) {
        return;
      }

      action = CLAIM_CLIPBOARD;
      clipboard_data = data;
    } else {
      /* If we don't own the selection, reseting is a no-op. */
      if (primary_owner_since == X11_CURRENT_TIME && data == nullptr) {
        return;
      }
      action = CLAIM_PRIMARY;
      primary_data = data;
    }

    if (data != nullptr) {
      x11.x11_change_property(x11.get_window(), X11_ATOM_WM_NAME, X11_ATOM_STRING, 8,
                              X11_PROPERTY_APPEND, nullptr, 0);
    } else {
      x11.x11_set_selection_owner(x11.get_atom(clipboard ? CLIPBOARD : PRIMARY), X11_ATOM_NONE,
                                  X11_CURRENT_TIME);
    }

    x11.x11_flush();
    /* FIXME: we really should figure out what causes this to happen, and if we can
       recover. But for now we just set the x11_error to true, to prevent the
       interface from becoming unresponsive. */
    clipboard_signal.wait_until(l, timeout);
    action = ACTION_NONE;
  }

  void release_selections() {
    timeout_t timeout = timeout_time(1000000);

    if (!x11_working()) {
      return;
    }

    std::unique_lock<std::mutex> l(clipboard_mutex);
    if (clipboard_owner_since == X11_CURRENT_TIME && primary_owner_since == X11_CURRENT_TIME) {
      return;
    }

    action = RELEASE_SELECTIONS;
    if (clipboard_owner_since != X11_CURRENT_TIME) {
      x11.x11_set_selection_owner(x11.get_atom(CLIPBOARD), X11_ATOM_NONE, X11_CURRENT_TIME);
    }
    if (primary_owner_since != X11_CURRENT_TIME) {
      x11.x11_set_selection_owner(x11.get_atom(PRIMARY), X11_ATOM_NONE, X11_CURRENT_TIME);
    }
    x11.x11_flush();
    clipboard_signal.wait_until(l, timeout);
    action = ACTION_NONE;
  }

  static x11_driver_t *implementation;

 private:
  /** Retrieve data set by another X client on our window.
          @return The number of bytes received, or -1 on failure.
  */
  long retrieve_data() {
    x11_atom_t actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = nullptr;
    unsigned long offset = 0;

    do {
      /* To limit the size of the transfer, we get the data in smallish blocks.
         That is, at most 16K. This will in most cases result in a single transfer,
         but in some cases we must iterate until we have all data. In that case
         we need to set offset, which happens to be in 4 byte words rather than
         bytes. */
      if (!x11.x11_get_window_property(x11.get_window(), x11.get_atom(GDK_SELECTION), offset / 4,
                                       DATA_BLOCK_SIZE, false, x11.get_atom(UTF8_STRING),
                                       &actual_type, &actual_format, &nitems, &bytes_after,
                                       &prop)) {
        retrieved_data.clear();
        return -1;
      } else {
        retrieved_data.append((char *)prop, nitems);
        offset += nitems;
        x11.x11_free_property_data(prop);
      }
      prop = nullptr;
    } while (bytes_after);
    return offset;
  }

  /** Claim a selection. */
  x11_time_t claim(x11_time_t since, x11_atom_t selection) {
    x11.x11_set_selection_owner(selection, x11.get_window(), since);
    if (x11.x11_get_selection_owner(selection) != x11.get_window()) {
      since = X11_CURRENT_TIME;
    }
    clipboard_signal.notify_one();
    return since;
  }

  /** Send data requested by another X client.
          @param requestor The @c x11_window_t of the requesting X client.
          @param target The requested conversion.
          @param property The property which must be used for sending the data.
          @param data The data to send.
          @param since The timestamp at which we aquired the requested selection.
          @return A boolean indicating succes.
  */
  bool send_selection(x11_window_t requestor, x11_atom_t target, x11_atom_t property,
                      linked_ptr<std::string>::t data, x11_time_t since) {
    if (target == x11.get_atom(TARGETS)) {
      x11.x11_change_property(requestor, property, x11.get_atom(ATOM), 32, X11_PROPERTY_REPLACE,
                              x11.get_targets_list(), 4);
      return true;
    } else if (target == x11.get_atom(TIMESTAMP)) {
      x11.x11_change_property(requestor, property, x11.get_atom(TIMESTAMP), 32,
                              X11_PROPERTY_REPLACE, (unsigned char *)&since, 1);
      return true;
    } else if (target == x11.get_atom(UTF8_STRING)) {
      if (data == nullptr) {
        return false;
      }
      /* If the data is too large to send in a single go (which is an arbitrary number,
         unless limited by the maximum request size), we use the INCR protocol. */
      if (data->size() < x11.get_max_data()) {
        x11.x11_change_property(requestor, property, x11.get_atom(UTF8_STRING), 8,
                                X11_PROPERTY_REPLACE, (const unsigned char *)data->data(),
                                data->size());
      } else {
        long size = data->size();
        incr_sends.push_back(incr_send_data_t(requestor, data, property));
        x11.x11_select_prop_change(requestor, true);
        x11.x11_change_property(requestor, property, x11.get_atom(INCR), 32, X11_PROPERTY_REPLACE,
                                (unsigned char *)&size, 1);
      }
      return true;
    } else if (target == x11.get_atom(MULTIPLE)) {
      x11_atom_t actual_type, *requested_conversions;
      int actual_format;
      unsigned long nitems, bytes_after, i;

      if (!x11.x11_get_window_property(requestor, property, 0, 100, false, x11.get_atom(ATOM_PAIR),
                                       &actual_type, &actual_format, &nitems, &bytes_after,
                                       (unsigned char **)&requested_conversions) ||
          bytes_after != 0 || nitems & 1) {
        return false;
      }

      for (i = 0; i < nitems; i += 2) {
        if (requested_conversions[i] == x11.get_atom(MULTIPLE) ||
            !send_selection(requestor, requested_conversions[i], requested_conversions[i + 1], data,
                            since)) {
          requested_conversions[i + 1] = X11_ATOM_NONE;
        }
      }
      x11.x11_change_property(requestor, property, x11.get_atom(ATOM_PAIR), 32,
                              X11_PROPERTY_REPLACE, (unsigned char *)requested_conversions, nitems);
      return true;
    } else {
      return false;
    }
  }

  /** Handle an incoming PropertyNotify event.

          There are so many different things which may be going on when we receive a
          PropertyNotify event, that it has its own routine.
  */
  void handle_property_notify(x11_property_event_t *event) {
    if (event->window == x11.get_window()) {
      if (event->atom == X11_ATOM_WM_NAME) {
        /* If we changed the name atom of our window, we needed a timestamp
           to perform another request. The request we want to perform is
           signalled by the action variable. */
        switch (action) {
          case CONVERT_CLIPBOARD:
          case CONVERT_PRIMARY:
            retrieved_data.clear();
            conversion_succeeded = false;
            conversion_started_at = event->time;
            /* Make sure that the target property does not exist */
            x11.x11_delete_property(x11.get_window(), x11.get_atom(GDK_SELECTION));
            x11.x11_convert_selection(
                x11.get_atom(action == CONVERT_CLIPBOARD ? CLIPBOARD : PRIMARY),
                x11.get_atom(UTF8_STRING), x11.get_atom(GDK_SELECTION), x11.get_window(),
                conversion_started_at);
            break;
          case CLAIM_CLIPBOARD:
            clipboard_owner_since = claim(event->time, x11.get_atom(CLIPBOARD));
            break;
          case CLAIM_PRIMARY:
            primary_owner_since = claim(event->time, x11.get_atom(PRIMARY));
            break;
          default:
            break;
            action = ACTION_NONE;
        }
      } else if (event->atom == x11.get_atom(GDK_SELECTION)) {
        /* This event may happen all the time, but in some cases it means there
           is more data to receive for an INCR transfer. */
        if (receive_incr && event->state == X11_PROPERTY_NEW_VALUE) {
          long result;
          if ((result = retrieve_data()) <= 0) {
            receive_incr = false;
            conversion_succeeded = result == 0;
            clipboard_signal.notify_one();
          }
          x11.x11_delete_property(x11.get_window(), x11.get_atom(GDK_SELECTION));
        }
      }
    } else {
      if (event->state != X11_PROPERTY_DELETE || incr_sends.empty()) {
        return;
      }

      /* In this case we received a PropertyNotify for a window that is not ours.
         That should only happen if we are trying to do an INCR send to another
         client. */
      for (incr_send_list_t::iterator iter = incr_sends.begin(); iter != incr_sends.end(); iter++) {
        if (iter->window == event->window) {
          unsigned long size = std::min(iter->data->size() - iter->offset, x11.get_max_data());
          x11.x11_change_property(iter->window, iter->property, x11.get_atom(UTF8_STRING), 8,
                                  X11_PROPERTY_REPLACE,
                                  (const unsigned char *)iter->data->data() + iter->offset, size);
          if (size == 0) {
            x11.x11_select_prop_change(iter->window, false);
            incr_sends.erase(iter);
            return;
          }
          iter->offset += size;
          return;
        }
      }
      x11.x11_select_prop_change(event->window, false);
    }
  }

  static void process_events_wrapper() { implementation->process_events(); }

  /** Thread to process incoming events. */
  void process_events() {
    x11_event_t *event = nullptr; /* Shut up compiler by initializing. */
    fd_set saved_read_fds;
    int fd_max;

    clipboard_mutex.lock();
    fd_max = x11.x11_fill_fds(&saved_read_fds);

    while (true) {
      /* The order of the checks here is important: we check the end_connection
         first, because that means the connection may be closed. We also check
         for errors on the connection here, because probing when the connection
         is no longer valid makes no sense. Then of course we probe for events.
         But the probe for events may detect another error, so we check the
         x11_error flag again before continuing. */
      while (!end_connection && !x11.has_error() && (event = x11.x11_probe_event()) == nullptr &&
             !x11.has_error()) {
        fd_set read_fds;

        /* Use select to wait for more events when there are no more left. In
           this case we also release the mutex, such that the rest of the library
           may interact with the clipboard. */
        read_fds = saved_read_fds;
        clipboard_mutex.unlock();
        select(fd_max, &read_fds, nullptr, nullptr, nullptr);
        x11.x11_acknowledge_wakeup(&read_fds);
        clipboard_mutex.lock();
      }
      if (x11.has_error() || end_connection) {
        clipboard_mutex.unlock();
        return;
      }

      switch (event->x11_response_type & ~0x80) {
        case X11_PROPERTY_NOTIFY:
          handle_property_notify((x11_property_event_t *)event);
          break;

        case X11_SELECTION_NOTIFY: {
          x11_selection_event_t *selection_notify = (x11_selection_event_t *)event;
          /* Conversion failed. */
          if (selection_notify->property == X11_ATOM_NONE) {
            if (action == CONVERT_CLIPBOARD || action == CONVERT_PRIMARY) {
              clipboard_signal.notify_one();
            }
            break;
          }

          if (action != CONVERT_CLIPBOARD && action != CONVERT_PRIMARY) {
            x11.x11_delete_property(x11.get_window(), selection_notify->property);
            break;
          }

          if (selection_notify->property != x11.get_atom(GDK_SELECTION) ||
              selection_notify->time != conversion_started_at ||
              (action == CONVERT_CLIPBOARD &&
               selection_notify->selection != x11.get_atom(CLIPBOARD)) ||
              (action == CONVERT_PRIMARY && selection_notify->selection != x11.get_atom(PRIMARY)) ||
              (selection_notify->target != x11.get_atom(UTF8_STRING) &&
               selection_notify->target != x11.get_atom(INCR))) {
            x11.x11_delete_property(x11.get_window(), selection_notify->property);
            clipboard_signal.notify_one();
            break;
          }

          if (selection_notify->target == x11.get_atom(INCR)) {
            /* OK, here we go. The selection owner uses the INCR protocol. Shudder. */
            receive_incr = true;
          } else if (selection_notify->target == x11.get_atom(UTF8_STRING)) {
            if (retrieve_data() >= 0) {
              conversion_succeeded = true;
            }
            clipboard_signal.notify_one();
          } else {
            clipboard_signal.notify_one();
          }
          x11.x11_delete_property(x11.get_window(), x11.get_atom(GDK_SELECTION));
          break;
        }
        case X11_SELECTION_CLEAR: {
          x11_selection_clear_event_t *clear_event = (x11_selection_clear_event_t *)event;
          if (clear_event->selection == x11.get_atom(CLIPBOARD)) {
            clipboard_owner_since = X11_CURRENT_TIME;
            clipboard_data = nullptr;
          } else if (clear_event->selection == x11.get_atom(PRIMARY)) {
            primary_owner_since = X11_CURRENT_TIME;
            primary_data = nullptr;
          }

          if ((action == RELEASE_SELECTIONS && clipboard_owner_since == X11_CURRENT_TIME &&
               primary_owner_since == X11_CURRENT_TIME) ||
              action == CLAIM_CLIPBOARD || action == CLAIM_PRIMARY) {
            clipboard_signal.notify_one();
          }
          break;
        }
        case X11_SELECTION_REQUEST: {
          x11_selection_event_t reply;
          linked_ptr<std::string>::t data;
          x11_time_t since;
          x11_selection_request_event_t *request_event = (x11_selection_request_event_t *)event;

          /* Some other X11 client is requesting our selection. */

          reply.x11_response_type = X11_SELECTION_NOTIFY;
          reply.requestor = request_event->requestor;
          reply.selection = request_event->selection;
          reply.target = request_event->target;
          reply.time = request_event->time;
          if (request_event->target == x11.get_atom(MULTIPLE) &&
              request_event->property == X11_ATOM_NONE) {
            reply.property = X11_ATOM_NONE;
          } else {
            reply.property = request_event->property == X11_ATOM_NONE ? request_event->target
                                                                      : request_event->property;
            if (request_event->selection == x11.get_atom(CLIPBOARD) &&
                clipboard_owner_since != X11_CURRENT_TIME) {
              data = clipboard_data;
              since = clipboard_owner_since;
            } else if (request_event->selection == x11.get_atom(PRIMARY) &&
                       primary_owner_since != X11_CURRENT_TIME) {
              data = primary_data;
              since = primary_owner_since;
            } else {
              reply.property = X11_ATOM_NONE;
            }
          }

          if (reply.property != X11_ATOM_NONE &&
              !send_selection(request_event->requestor, request_event->target, reply.property, data,
                              since)) {
            reply.property = X11_ATOM_NONE;
          }

          x11.x11_send_event(request_event->requestor, false, 0, (x11_event_t *)&reply);
          break;
        }
        default:
          break;
      }
      x11.x11_free_event(event);
    }
  }

  x11_impl_t x11;

  enum clipboard_action_t {
    ACTION_NONE,
    CONVERT_CLIPBOARD,
    CONVERT_PRIMARY,
    CLAIM_CLIPBOARD,
    CLAIM_PRIMARY,
    RELEASE_SELECTIONS
  };
  clipboard_action_t action = ACTION_NONE;

  bool conversion_succeeded = false;
  bool end_connection = false;

  struct incr_send_data_t {
    x11_window_t window;
    linked_ptr<std::string>::t data;
    x11_atom_t property;
    size_t offset;

    incr_send_data_t(x11_window_t _window, linked_ptr<std::string>::t &_data, x11_atom_t _property)
        : window(_window), data(_data), property(_property), offset(0) {}
  };
  using incr_send_list_t = std::list<incr_send_data_t>;
  incr_send_list_t incr_sends;
  bool receive_incr = false;

  /* Use X11_CURRENT_TIME as "Invalid" value, as it will never be returned by anything. */
  x11_time_t clipboard_owner_since = X11_CURRENT_TIME, primary_owner_since = X11_CURRENT_TIME;
  x11_time_t conversion_started_at = 0;

  std::thread x11_event_thread;
  std::mutex clipboard_mutex;
  std::unique_lock<std::mutex> clipboard_mutex_lock;
  std::condition_variable clipboard_signal;

  std::string retrieved_data;

#ifdef USE_XLIB
  friend class x11_impl_t;
#endif
};
x11_driver_t *x11_driver_t::implementation;

#ifdef USE_XLIB
int x11_impl_t::error_handler(Display *_display, XErrorEvent *error) {
#ifdef _T3_WIDGET_DEBUG
  char error_text[1024];
  XGetErrorText(_display, error->error_code, error_text, sizeof(error_text));

  lprintf("X11 error handler: %s\n", error_text);
#else
  (void)_display;
  (void)error;
#endif
  if (!x11_driver_t::implementation->x11.x11_initialized)
    x11_driver_t::implementation->x11.x11_error = true;
  return 0;
}

/** Handle IO errors. */
int x11_impl_t::io_error_handler(Display *_display) {
  (void)_display;
  lprintf("X11 IO error\n");
  /* Note that this is the only place this variable is ever written, so there
     is no problem in not using a lock here. */
  x11_driver_t::implementation->x11.x11_error = true;
  return 0;
}
#endif

static bool init_x11() {
  lprintf("Starting X11 initialization\n");
  if (!x11_driver_t::implementation) {
    x11_driver_t::implementation = new x11_driver_t;
  }
  if (!x11_driver_t::implementation->init_x11()) {
    delete x11_driver_t::implementation;
    x11_driver_t::implementation = nullptr;
    lprintf("X11 initialization failed!\n");
    return false;
  }
  return true;
}

static void release_selections() {
  if (!x11_driver_t::implementation) {
    return;
  }
  x11_driver_t::implementation->release_selections();
}

static linked_ptr<std::string>::t get_selection(bool clipboard) {
  if (!x11_driver_t::implementation) {
    return clipboard ? clipboard_data : primary_data;
  }
  return x11_driver_t::implementation->get_selection(clipboard);
}

static void claim_selection(bool clipboard, std::string *data) {
  if (!x11_driver_t::implementation) {
    return;
  }
  x11_driver_t::implementation->claim_selection(clipboard, data);
}

static void lock() {
  if (!x11_driver_t::implementation) {
    return;
  }
  x11_driver_t::implementation->lock();
}

static void unlock() {
  if (!x11_driver_t::implementation) {
    return;
  }
  x11_driver_t::implementation->unlock();
}

static void stop_x11() {
  if (!x11_driver_t::implementation) {
    return;
  }
  x11_driver_t::implementation->stop_x11();
  delete x11_driver_t::implementation;
  x11_driver_t::implementation = nullptr;
}

extern "C" {
T3_WIDGET_API extclipboard_interface_t _t3_widget_extclipboard_calls = {EXTCLIPBOARD_VERSION,
                                                                        init_x11,
                                                                        release_selections,
                                                                        get_selection,
                                                                        claim_selection,
                                                                        lock,
                                                                        unlock,
                                                                        stop_x11};
};

};  // namespace
