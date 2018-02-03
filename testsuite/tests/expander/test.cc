void button_activated(void) {
	lprintf("Button activated\n");
	exit_main_loop(0);
}

class main_window_t : public main_window_base_t {
	expander_t *expander;
	public:
		main_window_t(void) {
			expander = new expander_t("_Test");
			button_t *button = new button_t("T_est");
			button->connect_activate(signals::ptr_fun(button_activated));
			expander->set_child(button);
			expander->set_size(None, 10);
			push_back(expander);
		}
};

