void button_activated(void) {
  lprintf("Button activated\n");
  exit_main_loop(0);
}

class main_window_t : public main_window_base_t {
  expander_t *expander;

 public:
  main_window_t(void) {
    expander = emplace_back<expander_t>("_Test");
    button_t *button = new button_t("T_est");
    button->connect_activate(button_activated);
    expander->set_child(button);
    expander->set_size(None, 10);
  }
};
