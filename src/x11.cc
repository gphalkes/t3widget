/* Copyright (C) 2011 G.P. Halkes
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
#include <stdlib.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cstring>
#include <string>
#include <pthread.h>
#include <errno.h>
#include <list>

#ifdef HAS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "log.h"
#include "ptr.h"
#include "extclipboard.h"

// FIXME: remove incr_sends on long periods of inactivity

using namespace std;

namespace t3_widget {

enum clipboard_action_t {
	ACTION_NONE,
	CONVERT_CLIPBOARD,
	CONVERT_PRIMARY,
	CLAIM_CLIPBOARD,
	CLAIM_PRIMARY,
	RELEASE_SELECTIONS
};

struct incr_send_data_t {
	Window window;
	linked_ptr<string>::t data;
	Atom property;
	size_t offset;

	incr_send_data_t(Window _window, linked_ptr<string>::t &_data, Atom _property) : window(_window),
		data(_data), property(_property), offset(0) {}
};
typedef list<incr_send_data_t> incr_send_list_t;
static incr_send_list_t incr_sends;

static clipboard_action_t action;
static bool conversion_succeeded;

/* Use CurrentTime as "Invalid" value, as it will never be returned by anything. */
static Time clipboard_owner_since = CurrentTime,
	primary_owner_since = CurrentTime;
static Time conversion_started_at;

static Display *display;
static Window root, window;
static int wakeup_pipe[2];

static size_t max_data;

static bool receive_incr;

static pthread_t x11_event_thread;
static pthread_mutex_t clipboard_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t clipboard_signal = PTHREAD_COND_INITIALIZER;

static bool x11_initialized;
static bool x11_error;
static bool end_connection;

static const char *atom_names[] = {
	"CLIPBOARD",
	"PRIMARY",
	"TARGETS",
	"TIMESTAMP",
	"MULTIPLE",
	"UTF8_STRING",
	"GDK_SELECTION", // Use same name as GDK to save on atoms
	"INCR",
	"ATOM",
	"ATOM_PAIR",
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
static Atom atoms[ATOM_COUNT];


#define DATA_BLOCK_SIZE 4000
static string retrieved_data;

/** Retrieve data set by another X client on our window.
    @return The number of bytes received, or -1 on failure.
*/
static long retrieve_data(void) {
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop = NULL;
	unsigned long offset = 0;

	do {
		/* To limit the size of the transfer, we get the data in smallish blocks.
		   That is, at most 16K. This will in most cases result in a single transfer,
		   but in some cases we must iterate until we have all data. In that case
		   we need to set offset, which happens to be in 4 byte words rather than
		   bytes. */
		if (XGetWindowProperty(display, window, atoms[GDK_SELECTION], offset / 4,
			DATA_BLOCK_SIZE, False, atoms[UTF8_STRING], &actual_type, &actual_format,
			&nitems, &bytes_after, &prop) != Success)
		{
			retrieved_data.clear();
			return -1;
		} else {
			retrieved_data.append((char *) prop, nitems);
			offset += nitems;
			XFree(prop);
		}
		prop = NULL;
	} while (bytes_after);
	return offset;
}

/** Claim a selection. */
static Time claim(Time since, Atom selection) {
	XSetSelectionOwner(display, selection, window, since);
	if (XGetSelectionOwner(display, selection) != window)
		since = CurrentTime;
	pthread_cond_signal(&clipboard_signal);
	return since;
}

/** Send data requested by another X client.
    @param requestor The @c Window of the requesting X client.
    @param target The requested conversion.
    @param property The property which must be used for sending the data.
    @param data The data to send.
    @param since The timestamp at which we aquired the requested selection.
    @return A boolean indicating succes.
*/
static bool send_selection(Window requestor, Atom target, Atom property, linked_ptr<string>::t data, Time since) {
	if (target == atoms[TARGETS]) {
		/* The atoms are arranged such that the targets we have available are consecutive. */
		XChangeProperty(display, requestor, property, atoms[ATOM], 32, PropModeReplace, (unsigned char *) &atoms[TARGETS], 4);
		return true;
	} else if (target == atoms[TIMESTAMP]) {
		XChangeProperty(display, requestor, property, atoms[TIMESTAMP], 32, PropModeReplace, (unsigned char *) &since, 1);
		return true;
	} else if (target == atoms[UTF8_STRING]) {
		if (data == NULL)
			return false;
		/* If the data is too large to send in a single go (which is an arbitrary number,
		   unless limited by the maximum request size), we use the INCR protocol. */
		if (data->size() > max_data) {
			XChangeProperty(display, requestor, property, atoms[UTF8_STRING], 8, PropModeReplace,
				(unsigned char *) data->data(), data->size());
		} else {
			long size = data->size();
			incr_sends.push_back(incr_send_data_t(requestor, data, property));
			XSelectInput(display, requestor, PropertyChangeMask);
			XChangeProperty(display, requestor, property, atoms[INCR], 32, PropModeReplace, (unsigned char *) &size, 1);
		}
		return true;
	} else if (target == atoms[MULTIPLE]) {
		Atom actual_type, *requested_conversions;
		int actual_format;
		unsigned long nitems, bytes_after, i;

		if (XGetWindowProperty(display, requestor, property, 0, 100, False, atoms[ATOM_PAIR],
				&actual_type, &actual_format, &nitems, &bytes_after, (unsigned char **) &requested_conversions) != Success ||
				bytes_after != 0 || nitems & 1)
			return false;

		for (i = 0; i < nitems; i += 2) {
			if (requested_conversions[i] == atoms[MULTIPLE] || !send_selection(requestor, requested_conversions[i],
					requested_conversions[i + 1], data, since))
				requested_conversions[i + 1] = None;
		}
		XChangeProperty(display, requestor, property, atoms[ATOM_PAIR], 32, PropModeReplace,
			(unsigned char *) requested_conversions, nitems);
		return true;
	} else {
		return false;
	}
}

/** Handle an incoming PropertyNotify event.

    There are so many different things which may be going on when we receive a
    PropertyNotify event, that it has its own routine.
*/
static void handle_property_notify(XEvent &event) {
	if (event.xproperty.window == window) {
		if (event.xproperty.atom == XA_WM_NAME) {
			/* If we changed the name atom of our window, we needed a timestamp
			   to perform another request. The request we want to perform is
			   signalled by the action variable. */
			switch (action) {
				case CONVERT_CLIPBOARD:
				case CONVERT_PRIMARY:
					retrieved_data.clear();
					conversion_succeeded = false;
					conversion_started_at = event.xproperty.time;
					/* Make sure that the target property does not exist */
					XDeleteProperty(display, window, atoms[GDK_SELECTION]);
					XConvertSelection(display, atoms[action == CONVERT_CLIPBOARD ? CLIPBOARD : PRIMARY],
						atoms[UTF8_STRING], atoms[GDK_SELECTION], window, conversion_started_at);
					break;
				case CLAIM_CLIPBOARD:
					clipboard_owner_since = claim(event.xproperty.time, atoms[CLIPBOARD]);
					break;
				case CLAIM_PRIMARY:
					primary_owner_since = claim(event.xproperty.time, atoms[PRIMARY]);
					break;
				default:
					break;
			}
		} else if (event.xproperty.atom == atoms[GDK_SELECTION]) {
			/* This event may happen all the time, but in some cases it means there
			   is more data to receive for an INCR transfer. */
			if (receive_incr && event.xproperty.state == PropertyNewValue) {
				long result;
				if ((result = retrieve_data()) <= 0) {
					receive_incr = false;
					conversion_succeeded = result == 0;
					pthread_cond_signal(&clipboard_signal);
				}
				XDeleteProperty(display, window, atoms[GDK_SELECTION]);
			}
		}
	} else {
		if (event.xproperty.state != PropertyDelete || incr_sends.empty())
			return;

		/* In this case we received a PropertyNotify for a window that is not ours.
		   That should only happen if we are trying to do an INCR send to another
		   client. */
		for (incr_send_list_t::iterator iter = incr_sends.begin(); iter != incr_sends.end(); iter++) {
			if (iter->window == event.xproperty.window) {
				unsigned long size = iter->data->size() - iter->offset;
				if (size > max_data)
					size = max_data;
				XChangeProperty(display, iter->window, iter->property, atoms[UTF8_STRING], 8, PropModeReplace,
					(unsigned char *) iter->data->data() + iter->offset, size);
				if (size == 0) {
					XSelectInput(display, iter->window, 0);
					incr_sends.erase(iter);
					break;
				}
				iter->offset += size;
				break;
			}
		}
	}
}

/** Thread to process incoming events. */
static void *process_events(void *arg) {
	XEvent event;
	fd_set saved_read_fds;
	int fd_max;

	(void) arg;

	pthread_mutex_lock(&clipboard_mutex);
	FD_ZERO(&saved_read_fds);
	FD_SET(ConnectionNumber(display), &saved_read_fds);
	FD_SET(wakeup_pipe[0], &saved_read_fds);
	if (wakeup_pipe[0] < ConnectionNumber(display))
		fd_max = ConnectionNumber(display);
	else
		fd_max = wakeup_pipe[0];
	fd_max++;

	while (1) {
		while (!XPending(display)) {
			fd_set read_fds;

			/* Use select to wait for more events when there are no more left. In
			   this case we also release the mutex, such that the rest of the library
			   may interact with the clipboard. */
			read_fds = saved_read_fds;
			pthread_mutex_unlock(&clipboard_mutex);
			select(fd_max, &read_fds, NULL, NULL, NULL);

			/* Clear data from wake-up pipe */
			if (FD_ISSET(wakeup_pipe[0], &read_fds)) {
				char buffer[8];
				read(wakeup_pipe[0], buffer, sizeof(buffer));
			}
			pthread_mutex_lock(&clipboard_mutex);
			if (x11_error || end_connection) {
				pthread_mutex_unlock(&clipboard_mutex);
				return NULL;
			}
		}
		if (x11_error || end_connection) {
			pthread_mutex_unlock(&clipboard_mutex);
			return NULL;
		}
		XNextEvent(display, &event);

		switch (event.type) {
			case PropertyNotify:
				handle_property_notify(event);
				break;

			case SelectionNotify: {
				/* Conversion failed. */
				if (event.xselection.property == None) {
					if (action == CONVERT_CLIPBOARD || action == CONVERT_PRIMARY)
						pthread_cond_signal(&clipboard_signal);
					break;
				}

				if (action != CONVERT_CLIPBOARD && action != CONVERT_PRIMARY) {
					XDeleteProperty(display, window, event.xselection.property);
					break;
				}

				if (event.xselection.property != atoms[GDK_SELECTION] ||
						event.xselection.time != conversion_started_at ||
						(action == CONVERT_CLIPBOARD && event.xselection.selection != atoms[CLIPBOARD]) ||
						(action == CONVERT_PRIMARY && event.xselection.selection != atoms[PRIMARY]) ||
						(event.xselection.target != atoms[UTF8_STRING] && event.xselection.target != atoms[INCR]))
				{
					XDeleteProperty(display, window, event.xselection.property);
					pthread_cond_signal(&clipboard_signal);
					break;
				}

				if (event.xselection.target == atoms[INCR]) {
					/* OK, here we go. The selection owner uses the INCR protocol. Shudder. */
					receive_incr = true;
				} else if (event.xselection.target == atoms[UTF8_STRING]) {
					if (retrieve_data() >= 0)
						conversion_succeeded = true;
					pthread_cond_signal(&clipboard_signal);
				} else {
					pthread_cond_signal(&clipboard_signal);
				}
				XDeleteProperty(display, window, atoms[GDK_SELECTION]);
				break;
			}
			case SelectionClear:
				if (event.xselectionclear.selection == atoms[CLIPBOARD]) {
					clipboard_owner_since = CurrentTime;
					clipboard_data = NULL;
				} else if (event.xselectionclear.selection == atoms[PRIMARY]) {
					primary_owner_since = CurrentTime;
					primary_data = NULL;
				}

				if ((action == RELEASE_SELECTIONS && clipboard_owner_since == CurrentTime && primary_owner_since == CurrentTime) ||
						action == CLAIM_CLIPBOARD || action == CLAIM_PRIMARY)
					pthread_cond_signal(&clipboard_signal);
				break;

			case SelectionRequest: {
				XEvent reply;
				linked_ptr<string>::t data;
				Time since;

				/* Some other X11 client is requesting our selection. */

				reply.type = SelectionNotify;
				reply.xselection.requestor = event.xselectionrequest.requestor;
				reply.xselection.selection = event.xselectionrequest.selection;
				reply.xselection.target = event.xselectionrequest.target;
				if (event.xselectionrequest.target == atoms[MULTIPLE] && event.xselectionrequest.property == None) {
					reply.xselection.property = None;
				} else {
					reply.xselection.property = event.xselectionrequest.property == None ? event.xselectionrequest.target:
						event.xselectionrequest.property;
					if (event.xselectionrequest.selection == atoms[CLIPBOARD] && clipboard_owner_since != CurrentTime) {
						data = clipboard_data;
						since = clipboard_owner_since;
					} else if (event.xselectionrequest.selection == atoms[PRIMARY] && primary_owner_since != CurrentTime) {
						data = primary_data;
						since = primary_owner_since;
					} else {
						reply.xselection.property = None;
					}
				}

				if (reply.xselection.property != None && !send_selection(event.xselectionrequest.requestor,
						event.xselectionrequest.target, reply.xselection.property, data, since))
					reply.xselection.property = None;

				XSendEvent(display, event.xselectionrequest.requestor, False, 0, &reply);
				break;
			}
			default:
				break;
		}
	}
	return NULL;
}

/** Handle error reports, i.e. BadAtom and such, from the server. */
static int error_handler(Display *_display, XErrorEvent *error) {
#ifdef _T3_WIDGET_DEBUG
	char error_text[1024];
	XGetErrorText(_display, error->error_code, error_text, sizeof(error_text));

	lprintf("X11 error handler: %s\n", error_text);
#else
	(void) _display;
	(void) error;
#endif
	if (!x11_initialized)
		x11_error = true;
	return 0;
}

/** Handle IO errors. */
static int io_error_handler(Display *_display) {
	(void) _display;
	lprintf("X11 IO error\n");
	/* Note that this is the only place this variable is ever written, so there
	   is no problem in not using a lock here. */
	x11_error = true;
	return 0;
}

/** Stop the X11 event processing. */
static void stop_x11(void) {
	void *result;
	if (!x11_initialized)
		return;

	pthread_mutex_lock(&clipboard_mutex);
	end_connection = true;
	/* If x11_error has been set, the event handling thread will stop, or will
	   have stopped already. Also, if this is the case, the connection is broken,
	   which means we can't send anything anyway. Thus we skip the client message
	   if x11_error is set. */
	if (!x11_error)
		XCloseDisplay(display);

	pthread_cancel(x11_event_thread);
	pthread_mutex_unlock(&clipboard_mutex);
	pthread_join(x11_event_thread, &result);
}

/** Initialize the X11 connection. */
static bool init_x11(void) {
	int black;
	char **atom_names_local = NULL;
	long max_request;
	size_t i;

	/* The XInternAtoms call expects an array of char *, not of const char *. Thus
	   we make a copy here, which we later discard again. */
	if ((atom_names_local = (char **) malloc(sizeof(char *) * ATOM_COUNT)) == NULL)
		return false;

	/* First initialize all the pointers to NULL, such that we can do a simple goto
	   error_exit if some allocation fails. */
	for (i = 0; i < ATOM_COUNT; i++) atom_names_local[i] = NULL;
	for (i = 0; i < ATOM_COUNT; i++) {
		if ((atom_names_local[i] = strdup(atom_names[i])) == NULL)
			goto error_exit;
	}

	// Make sure X11 errors don't abort the program
	XSetErrorHandler(error_handler);
	XSetIOErrorHandler(io_error_handler);

	if ((display = XOpenDisplay(NULL)) == NULL)
		goto error_exit;

	max_request = XMaxRequestSize(display);
	if (max_request > DATA_BLOCK_SIZE * 4 + 100)
		max_data = DATA_BLOCK_SIZE * 4;
	else
		max_data = max_request - 100;

	root = XDefaultRootWindow(display);
	black = BlackPixel(display, DefaultScreen(display));
	window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, black, black);

	/* Discard all events, but process errors. */
	XSync(display, True);
	if (x11_error)
		goto error_exit;

	if (!XInternAtoms(display, atom_names_local, ATOM_COUNT, False, atoms))
		goto error_exit;

	for (i = 0; i < ATOM_COUNT; i++)
		free(atom_names_local[i]);
	free(atom_names_local);
	atom_names_local = NULL;

	XSelectInput(display, window, PropertyChangeMask);

	/* Discard all events, but process errors. */
	XSync(display, True);
	if (x11_error)
		goto error_exit;

	if (pipe(wakeup_pipe) < 0)
		goto error_exit;

	x11_initialized = true;
	pthread_create(&x11_event_thread, NULL, process_events, NULL);
	lprintf("X11 interface initialized\n");
	return true;

error_exit:
	if (atom_names_local != NULL) {
		for (i = 0; i < ATOM_COUNT; i++)
			free(atom_names_local[i]);
		free(atom_names_local);
	}
	if (display != NULL)
		XCloseDisplay(display);
	return false;
}

#define x11_working() (x11_initialized && !x11_error)

static struct timespec timeout_time(int usec) {
	struct timeval timeval;
	struct timespec result;

	gettimeofday(&timeval, NULL);
	timeval.tv_sec += usec / 1000000;
	timeval.tv_usec += usec % 1000000;
	if (timeval.tv_usec > 1000000) {
		timeval.tv_sec++;
		timeval.tv_usec -= 1000000;
	}
	result.tv_sec = timeval.tv_sec;
	result.tv_nsec = (long) timeval.tv_usec * 1000;
	return result;
}

static linked_ptr<string>::t get_selection(bool clipboard) {
	struct timespec timeout = timeout_time(1000000);
	linked_ptr<string>::t result;

	/* NOTE: the clipboard is supposed to be locked when this routine is called. */

	/* If X11 was not initialized, or an IO error occured, we can skip the stuff
	   below, because it won't work. */
	if (!x11_working())
		return clipboard ? clipboard_data : primary_data;

	/* If we currently own the selection that is requested, there is no need to go
	   through the X server. */
	if ((clipboard && clipboard_owner_since == CurrentTime) || (!clipboard && primary_owner_since == CurrentTime)) {
		action = clipboard ? CONVERT_CLIPBOARD : CONVERT_PRIMARY;
		XChangeProperty(display, window, XA_WM_NAME, XA_STRING, 8, PropModeAppend, NULL, 0);
		XFlush(display);
		write(wakeup_pipe[1], &wakeup_pipe, 1);
		if (pthread_cond_timedwait(&clipboard_signal, &clipboard_mutex, &timeout) != ETIMEDOUT &&
				conversion_succeeded)
			result = new string(retrieved_data);
		action = ACTION_NONE;
	} else {
		result = clipboard ? clipboard_data : primary_data;
	}
	return result;
}

static void claim_selection(bool clipboard, string *data) {
	struct timespec timeout = timeout_time(1000000);

	if (!x11_working()) {
		if (clipboard)
			clipboard_data = data;
		else
			primary_data = data;
		return;
	}

	pthread_mutex_lock(&clipboard_mutex);

	if (clipboard) {
		/* If we don't own the selection, reseting is a no-op. */
		if (clipboard_owner_since == CurrentTime && data == NULL) {
			pthread_mutex_unlock(&clipboard_mutex);
			return;
		}
		action = CLAIM_CLIPBOARD;
		clipboard_data = data;
	} else {
		/* If we don't own the selection, reseting is a no-op. */
		if (primary_owner_since == CurrentTime && data == NULL) {
			pthread_mutex_unlock(&clipboard_mutex);
			return;
		}
		action = CLAIM_PRIMARY;
		primary_data = data;
	}

	if (data != NULL)
		XChangeProperty(display, window, XA_WM_NAME, XA_STRING, 8, PropModeAppend, NULL, 0);
	else
		XSetSelectionOwner(display, atoms[clipboard ? CLIPBOARD : PRIMARY], None, CurrentTime);

	XFlush(display);
	write(wakeup_pipe[1], &wakeup_pipe, 1);
	pthread_cond_timedwait(&clipboard_signal, &clipboard_mutex, &timeout);
	action = ACTION_NONE;
	pthread_mutex_unlock(&clipboard_mutex);
}

static void release_selections(void) {
	struct timespec timeout = timeout_time(1000000);

	if (!x11_working())
		return;

	pthread_mutex_lock(&clipboard_mutex);
	if (clipboard_owner_since == CurrentTime && primary_owner_since == CurrentTime) {
		pthread_mutex_unlock(&clipboard_mutex);
		return;
	}

	action = RELEASE_SELECTIONS;
	if (clipboard_owner_since != CurrentTime)
		XSetSelectionOwner(display, atoms[CLIPBOARD], None, CurrentTime);
	if (primary_owner_since != CurrentTime)
		XSetSelectionOwner(display, atoms[PRIMARY], None, CurrentTime);
	XFlush(display);
	write(wakeup_pipe[1], &wakeup_pipe, 1);
	pthread_cond_timedwait(&clipboard_signal, &clipboard_mutex, &timeout);
	action = ACTION_NONE;
	pthread_mutex_unlock(&clipboard_mutex);
}

static void lock(void) {
	pthread_mutex_lock(&clipboard_mutex);
}

static void unlock(void) {
	pthread_mutex_unlock(&clipboard_mutex);
}

extern "C" {
T3_WIDGET_API extclipboard_interface_t _t3_widget_extclipboard_calls = {
	EXTCLIPBOARD_VERSION,
	init_x11,
	release_selections,
	get_selection,
	claim_selection,
	lock,
	unlock,
	stop_x11
};
};

}; // namespace

