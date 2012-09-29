# Copyright (C) 2010 G.P. Halkes
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as
# published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

COMPILER := gcc

SOURCES.libt3widget.la := \
	autocompleter.cc \
	clipboard.cc \
	colorscheme.cc \
	contentlist.cc \
	findcontext.cc \
	interfaces.cc \
	key.cc \
	log.cc \
	main.cc \
	stringmatcher.cc \
	textbuffer.cc \
	textline.cc \
	undo.cc \
	util.cc \
	wrapinfo.cc \
	dialogs/attributepickerdialog.cc \
	dialogs/dialog.cc \
	dialogs/filedialog.cc \
	dialogs/finddialog.cc \
	dialogs/gotodialog.cc \
	dialogs/inputselectiondialog.cc \
	dialogs/insertchardialog.cc \
	dialogs/mainwindow.cc \
	dialogs/menupanel.cc \
	dialogs/messagedialog.cc \
	widgets/bullet.cc \
	widgets/button.cc \
	widgets/checkbox.cc \
	widgets/dummywidget.cc \
	widgets/editwindow.cc \
	widgets/filepane.cc \
	widgets/frame.cc \
	widgets/label.cc \
	widgets/listpane.cc \
	widgets/menu.cc \
	widgets/menuitem.cc \
	widgets/multiwidget.cc \
	widgets/scrollbar.cc \
	widgets/smartlabel.cc \
	widgets/separator.cc \
	widgets/split.cc \
	widgets/textfield.cc \
	widgets/textwindow.cc \
	widgets/widget.cc

SOURCES.x11.la := x11.cc

CXXLTTARGETS := libt3widget.la x11.la
#================================================#
# NO RULES SHOULD BE DEFINED BEFORE THIS INCLUDE #
#================================================#
include ../../makesys/rules.mk
#================================================#
T3NAME := widget
include ../../shared/rules.mk
#================================================#

CXXFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CXXFLAGS += -D_T3_WIDGET_DEBUG
CXXFLAGS += -D_T3_WIDGET_INTERNAL
CXXFLAGS += -DHAS_STRDUP
CXXFLAGS += -I$(CURDIR)
CXXFLAGS += -pthread
CXXFLAGS += `pkg-config --cflags libpcre`
CXXFLAGS += `pkg-config --cflags sigc++-2.0`
# Set default visibility to hidden, so we find out when we forgot to export something
CXXFLAGS += -fvisibility=hidden
CXXFLAGS += -DWITH_X11
CXXFLAGS += -DX11_MOD_NAME=\"$(PWD)/.libs/x11.mod\"

LDLIBS.libt3widget.la += -L$(CURDIR)/t3window/.libs -lt3window
LDLIBS.libt3widget.la += -L$(CURDIR)/transcript/.libs -ltranscript
LDLIBS.libt3widget.la += -L$(CURDIR)/t3key/.libs -lt3key
LDLIBS.libt3widget.la += `pkg-config --libs libpcre`
LDLIBS.libt3widget.la += `pkg-config --libs sigc++-2.0`
LDLIBS.libt3widget.la += -lunistring

CXXFLAGS += -DHAS_DLFCN
LDLIBS.libt3widget.la += -ldl
#~ LDLIBS.libt3widget.la += -lltdl

#~ CXXFLAGS.x11 += `pkg-config --cflags x11` -DUSE_XLIB
#~ LDLIBS.x11.la += `pkg-config --libs x11`

CXXFLAGS.x11 += `pkg-config --cflags xcb`
LDLIBS.x11.la += `pkg-config --libs xcb`

LDLIBS.x11.la += -module -avoid-version -L.libs -lt3widget -shrext .mod

links:
	@[ -L transcript ] || ln -s ../../../transcript/src transcript
	@[ -L t3window ] || ln -s ../../window/src t3window
	@[ -L t3key ] || ln -s ../../key/src t3key
	@[ -L t3widget ] || ln -s . t3widget

x11.la: | libt3widget.la

.PHONY: links
