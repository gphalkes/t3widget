_T3_ACTION(COPY, "Copy", EKEY_CTRL | 'c', EKEY_CTRL | EKEY_INS)
_T3_ACTION(CUT, "Cut", EKEY_CTRL | 'x', EKEY_DEL | EKEY_SHIFT)
_T3_ACTION(PASTE, "Paste", EKEY_CTRL | 'v')
/* This key combination is typically caught by the terminal, and bound to paste-selection.
   Hence, we bind this to paste-selection as well, to provide the least surprise. */
_T3_ACTION(PASTE_SELECTION, "PasteSelection", EKEY_INS | EKEY_SHIFT)
_T3_ACTION(SELECT_ALL, "SelectAll", EKEY_CTRL | 'a')
_T3_ACTION(INSERT_SPECIAL, "InsertSpecial", EKEY_F9)
_T3_ACTION(MARK_SELECTION, "MarkSelection", EKEY_CTRL | 't')
