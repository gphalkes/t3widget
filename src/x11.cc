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
#include <stdio.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdbool.h>
#include <cstring>
#include <string>
#include <pthread.h>
#include <errno.h>

using namespace std;

namespace t3_widget {

enum clipboard_action_t {
	ACTION_NONE,
	CONVERT_CLIPBOARD,
	CONVERT_PRIMARY,
	CLAIM_CLIPBOARD,
	CLAIM_PRIMARY
};

static clipboard_action_t action;
/* Use CurrentTime as "Invalid" value, as it will never be returned by anything. */
static Time clipboard_owner_since = CurrentTime,
	primary_owner_since = CurrentTime;
static string clipboard_data, primary_data;
static Time conversion_started_at;

static Display *display;
static Window root, window;

static long maxRequest;

static bool receive_incr = false;

static pthread_t x11_event_thread;
static pthread_mutex_t clipboard_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t clipboard_signal = PTHREAD_COND_INITIALIZER;

static int self_pipe[2];

static bool x11error = false;

static const char *atomNames[] = {
	"CLIPBOARD",
	"PRIMARY",
	"TARGETS",
	"UTF8_STRING",
	"GDK_SELECTION", // Use same name as GDK to save on atoms
	"TIMESTAMP",
	"MULTIPLE",
	"INCR",
	"ATOM"
};
enum {
	CLIPBOARD,
	PRIMARY,
	TARGETS,
	UTF8_STRING,
	GDK_SELECTION,
	TIMESTAMP,
	MULTIPLE,
	INCR,
	ATOM
};
#define ATOM_COUNT (sizeof(atomNames) / sizeof(atomNames[0]))
static Atom atoms[ATOM_COUNT];


#define DATA_BLOCK_SIZE 4000

static string retrievedData;

static long retrieve_data(void) {
	Atom actualType;
	int actualFormat;
	unsigned long nitems, bytesAfter;
	unsigned char *prop = NULL;
	unsigned long offset = 0;

	do {
		if (XGetWindowProperty(display, window, atoms[GDK_SELECTION], offset,
			DATA_BLOCK_SIZE, False, AnyPropertyType, &actualType, &actualFormat,
			&nitems, &bytesAfter, &prop) != Success)
		{
			retrievedData.clear();
			return -1;
		} else {
			retrievedData.append((char *) prop, nitems);
			offset += nitems;
			XFree(prop);
		}
		prop = NULL;
	} while (bytesAfter);
	return offset;
}

static void claim(Time *since, Atom selection) {
	XSetSelectionOwner(display, selection, window, *since);
	if (XGetSelectionOwner(display, selection) != window)
		*since = CurrentTime;
	pthread_cond_signal(&clipboard_signal);
}

static void send_selection(Window requestor, Atom target, Atom property, string *data) {
	if (target == atoms[TARGETS]) {
		XChangeProperty(display, requestor, property, atoms[ATOM], 32, PropModeReplace, (unsigned char *) &atoms[TARGETS], 2);
	} else if (target == atoms[UTF8_STRING]) {
		//FIXME: use incremental sending for large transfers
		XChangeProperty(display, requestor, property, atoms[UTF8_STRING], 8, PropModeReplace,
			(unsigned char *) data->data(), data->size());
	}
}

static void *processEvents(void *arg) {
	XEvent event;
	fd_set read_set;
	int x11fd = ConnectionNumber(display);
	int max_fd = x11fd > self_pipe[0] ? x11fd : self_pipe[0];

	(void) arg;

	pthread_mutex_lock(&clipboard_lock);
	while (1) {
		XFlush(display);
		while (!XPending(display)) {
			FD_ZERO(&read_set);
			FD_SET(x11fd, &read_set);
			FD_SET(self_pipe[0], &read_set);
			pthread_mutex_unlock(&clipboard_lock);
			select(max_fd + 1, &read_set, NULL, NULL, NULL);
			pthread_mutex_lock(&clipboard_lock);
			if (FD_ISSET(self_pipe[0], &read_set)) {
				char c;
				if (read(self_pipe[0], &c, 1) == 1)
					XChangeProperty(display, window, XA_WM_NAME, XA_STRING, 8, PropModeAppend, NULL, 0);
			}
			XFlush(display);
		}
		XNextEvent(display, &event);

		switch (event.type) {
			case PropertyNotify:
				/*
					If the notify was for our window:
						If property XA_WM_NAME:
						- save the timestamp
							If timestamp request was for obtaining selection:
							- call XConvertSelection
							If timestamp was for claiming selection ownership:
							- call XSetSelectionOwner and verify with XGetSelectionOwner
						If property GDK_SELECTION:
							If not currently interested (previous request timed out)
							- delete property
							Else
								If type is INCR and not already receiving
								- switch state to INCR reception
								- delete property
								Else
								- retrieve data
								- delete property
									If state is INCR reception
										If data length is 0
										- reset state
										- return collected data
										Else
										- append data
										- continue loop
									Else
										- reset state
										- return data
					Else
						If INCR sending and window is remote
							If more data available
							- send next piece of data
							Else
							- reset property notify events on remote
							- send zero length data
				*/
				if (event.xproperty.window == window) {
					if (event.xproperty.atom == XA_WM_NAME) {
						switch (action) {
							case CONVERT_CLIPBOARD:
							case CONVERT_PRIMARY:
								retrievedData.clear();
								conversion_started_at = event.xproperty.time;
								/* Make sure that the target property does not exist */
								XDeleteProperty(display, window, atoms[GDK_SELECTION]);
								XConvertSelection(display, atoms[action == CONVERT_CLIPBOARD ? CLIPBOARD : PRIMARY],
									atoms[UTF8_STRING], atoms[GDK_SELECTION], window, conversion_started_at);
								break;
							case CLAIM_CLIPBOARD:
								clipboard_owner_since = event.xproperty.time;
								claim(&clipboard_owner_since, atoms[CLIPBOARD]);
								break;
							case CLAIM_PRIMARY:
								primary_owner_since = event.xproperty.time;
								claim(&primary_owner_since, atoms[PRIMARY]);
								break;
							default:
								break;
						}
					} else if (event.xproperty.atom == atoms[GDK_SELECTION]) {
						if (receive_incr && event.xproperty.state == PropertyNewValue) {
							if (retrieve_data() <= 0) {
								receive_incr = false;
								fprintf(stderr, "Incremental transfer done\n");
								pthread_cond_signal(&clipboard_signal);
							}
							XDeleteProperty(display, window, atoms[GDK_SELECTION]);
						}
					}
				} else {
					/*FIXME: we may get this for incremental sends, so we should check
					  if any of the running sends are incremental. */
				}
				break;

			case SelectionNotify: { //FIXME: should we use throw to exit on error?
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
					receive_incr = true;
				} else if (event.xselection.target == atoms[UTF8_STRING]) {
					retrieve_data();
					pthread_cond_signal(&clipboard_signal);
				} else {
					pthread_cond_signal(&clipboard_signal);
				}
				XDeleteProperty(display, window, atoms[GDK_SELECTION]);
				break;
			}
			case SelectionClear:
				if (event.xselectionclear.selection == atoms[CLIPBOARD])
					clipboard_owner_since = CurrentTime;
				else if (event.xselectionclear.selection == atoms[PRIMARY])
					primary_owner_since = CurrentTime;
				break;

			case SelectionRequest:
				XEvent reply;
				reply.type = SelectionNotify;
				reply.xselection.requestor = event.xselectionrequest.requestor;
				reply.xselection.selection = event.xselectionrequest.selection;
				reply.xselection.target = event.xselectionrequest.target;
				reply.xselection.property = event.xselectionrequest.property == None ? event.xselectionrequest.target:
					event.xselectionrequest.property;

				if (event.xselectionrequest.selection == atoms[CLIPBOARD] && clipboard_owner_since != CurrentTime) {
					send_selection(event.xselectionrequest.requestor, event.xselectionrequest.target,
						reply.xselection.property, &clipboard_data);
				} else if (event.xselectionrequest.selection == atoms[PRIMARY] && primary_owner_since != CurrentTime) {
					send_selection(event.xselectionrequest.requestor, event.xselectionrequest.target,
						reply.xselection.property, &primary_data);
				} else {
					reply.xselection.property = None;
				}
				XSendEvent(display, event.xselectionrequest.requestor, False, 0, &reply);
				break;
		}
	}
}

static int errorHandler(Display *_display, XErrorEvent *error) {
	(void) _display;
	(void) error;
	//FIXME: Alloc errors should not disable further processing
	x11error = true;
	return 0;
}

static int ioErrorHandler(Display *_display) {
	(void) _display;
	x11error = true;
	return 0;
}

bool init_x11(void) {
	XEvent event;
	int black;
	char **atomNamesLocal = NULL;
	size_t i;

	if (pipe(self_pipe) < 0)
		return false;

	if ((atomNamesLocal = (char **) malloc(sizeof(char *) * ATOM_COUNT)) == NULL)
		return false;

	for (i = 0; i < ATOM_COUNT; i++) atomNamesLocal[i] = NULL;

	for (i = 0; i < ATOM_COUNT; i++) {
		if ((atomNamesLocal[i] = strdup(atomNames[i])) == NULL)
			goto error_exit;
	}

	// Make sure X11 errors don't abort the program
	XSetErrorHandler(errorHandler);
	XSetIOErrorHandler(ioErrorHandler);

	if ((display = XOpenDisplay(NULL)) == NULL)
		goto error_exit;

	maxRequest = XMaxRequestSize(display);

	root = XDefaultRootWindow(display);
	black = BlackPixel(display, DefaultScreen(display));
	window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, black, black);

	while (XPending(display))
		XNextEvent(display, &event);
	if (x11error)
		goto error_exit;

	if (!XInternAtoms(display, atomNamesLocal, ATOM_COUNT, False, atoms))
		goto error_exit;

	XSelectInput(display, window, PropertyChangeMask);

	while (XPending(display))
		XNextEvent(display, &event);
	if (x11error)
		goto error_exit;

	pthread_create(&x11_event_thread, NULL, processEvents, NULL);
	return true;

error_exit:
	if (atomNamesLocal != NULL) {
		for (i = 0; i < ATOM_COUNT; i++)
			free(atomNamesLocal[i]);
		free(atomNamesLocal);
	}
	if (display != NULL)
		XCloseDisplay(display);
	return false;
}

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

string *get_x11_selection(bool clipboard) {
	struct timespec timeout = timeout_time(1000000);
	string *result;
	char c;

	pthread_mutex_lock(&clipboard_lock);
	action = clipboard ? CONVERT_CLIPBOARD : CONVERT_PRIMARY;
	while (write(self_pipe[1], &c, 1) != 1) {}
	if (pthread_cond_timedwait(&clipboard_signal, &clipboard_lock, &timeout) != ETIMEDOUT)
		result = new string(retrievedData);
	action = ACTION_NONE;
	pthread_mutex_unlock(&clipboard_lock);
	return result;
}

static void set_selection_data(string *selection_data, const string *data) {
	selection_data->clear();
	selection_data->append(*data);
}

void claim_selection(bool clipboard, const string *data) {
	struct timespec timeout = timeout_time(1000000);
	char c;

	pthread_mutex_lock(&clipboard_lock);
	if (clipboard) {
		action = CLAIM_CLIPBOARD;
		set_selection_data(&clipboard_data, data);
	} else {
		action = CLAIM_PRIMARY;
		set_selection_data(&primary_data, data);
	}
	while (write(self_pipe[1], &c, 1) != 1) {}
	pthread_cond_timedwait(&clipboard_signal, &clipboard_lock, &timeout);
	action = ACTION_NONE;
	pthread_mutex_unlock(&clipboard_lock);
}

#if 0
int main(int argc, char *argv[]) {
	string *result;
	init_x11();
	result = get_x11_selection(true);
	printf("Retrieved data: %.*s\n", (int) result->size(), result->data());
	fprintf(stderr, "Data retrieved\n");
	result->clear();
	result->append("Test text for X11 Clipboard action");
	claim_selection(true, result);
	fprintf(stderr, "Clipboard claimed\n");
	return 0;
}
#endif
}; // namespace
