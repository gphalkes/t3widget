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


SOURCES.libt3widget.la := widgets/menu.cc widgets/scrollbar.cc widgets/label.cc \
	widgets/listpane.cc widgets/multiwidget.cc widgets/bullet.cc \
	widgets/textfield.cc widgets/filepane.cc widgets/button.cc widgets/checkbox.cc \
	widgets/smartlabel.cc widgets/widget.cc widgets/dummywidget.cc widgets/menuitem.cc \
	widgets/contentlist.cc main.cc keys.cc undo.cc textline.cc stringmatcher.cc util.cc \
	colorscheme.cc textbuffer.cc subline.cc widgets/editwindow.cc \
	dialogs/dialog.cc dialogs/filedialog.cc dialogs/menupanel.cc dialogs/messagedialog.cc \
	dialogs/mainwindow.cc log.cc widgets/frame.cc
	#~ dialogs/finddialog.cc dialogs/insertchardialog.cc \
	#~ dialogs/closeconfirmdialog.cc dialogs/altmessagedialog.cc dialogs/gotodialog.cc \
	#~ dialogs/overwriteconfirmdialog.cc dialogs/openrecentdialog.cc dialogs/selectbufferdialog.cc \
	#~ dialogs/encodingdialog.cc

CXXLTTARGETS := libt3widget.la

#================================================#
# NO RULES SHOULD BE DEFINED BEFORE THIS INCLUDE #
#================================================#
include ../../makesys/rules.mk
#================================================#
T3NAME := widget
include ../../t3shared/rules.mk
#================================================#

CXXFLAGS += -I$(CURDIR) -D__STDC_LIMIT_MACROS -D_T3_WIDGET_DEBUG
CXXFLAGS += -I$(CURDIR)/window -I$(CURDIR)/unicode -I$(CURDIR)/key
CXXFLAGS += -pthread -DTHREADED_KEYS
CXXFLAGS += `pkg-config --cflags sigc++-2.0`
LDLIBS += `pkg-config --libs sigc++-2.0`
CXX := clang++
CXXFLAGS += -fno-caret-diagnostics

links:
	@[ -L unicode ] || ln -s ../../unicode/src unicode
	@[ -L charconv ] || ln -s ../../../charconv/src charconv
	@[ -L window ] || ln -s ../../window/src window
	@[ -L key ] || ln -s ../../key/src key

.PHONY: links
