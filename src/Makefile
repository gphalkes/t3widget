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


SOURCES.libt3widget.la := \
	colorscheme.cc \
	findcontext.cc \
	key.cc \
	log.cc \
	main.cc \
	stringmatcher.cc \
	subline.cc \
	textbuffer.cc \
	textline.cc \
	undo.cc \
	util.cc \
	dialogs/dialog.cc \
	dialogs/filedialog.cc \
	dialogs/gotodialog.cc \
	dialogs/insertchardialog.cc \
	dialogs/mainwindow.cc \
	dialogs/menupanel.cc \
	dialogs/messagedialog.cc \
	widgets/bullet.cc \
	widgets/button.cc \
	widgets/checkbox.cc \
	widgets/contentlist.cc \
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
	widgets/textfield.cc \
	widgets/widget.cc

#~ dialogs/finddialog.cc \

CXXLTTARGETS := libt3widget.la

#================================================#
# NO RULES SHOULD BE DEFINED BEFORE THIS INCLUDE #
#================================================#
include ../../makesys/rules.mk
#================================================#
T3NAME := widget
include ../../t3shared/rules.mk
#================================================#

CXXFLAGS += -D__STDC_LIMIT_MACROS
CXXFLAGS += -D_T3_WIDGET_DEBUG
CXXFLAGS += -I$(CURDIR)
CXXFLAGS += -I$(CURDIR)/window
CXXFLAGS += -I$(CURDIR)/unicode
CXXFLAGS += -I$(CURDIR)/key
CXXFLAGS += -I$(CURDIR)/charconv
CXXFLAGS += -pthread -DTHREADED_KEYS
CXXFLAGS += `pkg-config --cflags sigc++-2.0`

LDLIBS += -L$(CURDIR)/../src/window/.libs -lt3window
LDLIBS += -L$(CURDIR)/../src/unicode/.libs -lt3unicode
LDLIBS += -L$(CURDIR)/../src/charconv/.libs -lcharconv
LDLIBS += -L$(CURDIR)/../src/key/.libs -lt3key
LDLIBS += -lpcre
LDLIBS += `pkg-config --libs sigc++-2.0`

CXX := clang++
CXXFLAGS += -fno-caret-diagnostics

links:
	@[ -L unicode ] || ln -s ../../unicode/src unicode
	@[ -L charconv ] || ln -s ../../../charconv/src charconv
	@[ -L window ] || ln -s ../../window/src window
	@[ -L key ] || ln -s ../../key/src key

.PHONY: links
