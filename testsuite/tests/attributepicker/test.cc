void attribute_selected(t3_attr_t attr) {
	lprintf("attribute: %08x\n", (int) attr);
	exit_main_loop(0);
}

class main_window_t : public main_window_base_t {
	attribute_picker_dialog_t *dialog;

	public:
		main_window_t(void) {
			dialog = new attribute_picker_dialog_t();
			dialog->center_over(this);
			dialog->show();
			dialog->connect_closed(signals::bind(signals::ptr_fun(exit_main_loop), 1));
			dialog->connect_attribute_selected(signals::ptr_fun(attribute_selected));
			//~ dialog->set_base_attributes(T3_ATTR_BG_BLUE | T3_ATTR_BOLD);
		}
};

