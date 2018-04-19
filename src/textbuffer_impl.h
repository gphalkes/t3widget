#ifndef T3_WIDGET_TEXTBUFFER_IMPL_H
#define T3_WIDGET_TEXTBUFFER_IMPL_H

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#include <t3widget/textbuffer.h>

namespace t3_widget {

struct text_buffer_t::implementation_t {
  lines_t lines;
  text_coordinate_t selection_start;
  text_coordinate_t selection_end;
  selection_mode_t selection_mode;

  undo_list_t undo_list;
  text_coordinate_t last_undo_position;
  undo_type_t last_undo_type;
  undo_t *last_undo;

  text_line_factory_t *line_factory;

  implementation_t(text_line_factory_t *_line_factory)
      : selection_start(-1, 0),
        selection_end(-1, 0),
        selection_mode(selection_mode_t::NONE),
        last_undo_type(UNDO_NONE),
        last_undo(nullptr),
        line_factory(_line_factory == nullptr ? &default_text_line_factory : _line_factory) {}
};

}  // namespace t3_widget

#endif
