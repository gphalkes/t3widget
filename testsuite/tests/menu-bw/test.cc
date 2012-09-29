#define REMOVE_MENU_ID 7

class main_window_t : public main_window_base_t {
	menu_bar_t *menu;
	menu_panel_t *panel, *empty_panel;
	menu_item_base_t *remove;

	public:
		main_window_t(void) {
			menu = new menu_bar_t();
			push_back(menu);
			menu->connect_activate(sigc::mem_fun(this, &main_window_t::menu_activated));

			panel = new menu_panel_t("_File", menu);

			panel->add_item("_Open", "C-o", 0);
			panel->add_item("_Close", "C-w", 1);

			empty_panel = new menu_panel_t("E_mpty", menu);
			empty_panel->add_item("Remove", NULL, 7);

			panel = new menu_panel_t("_Edit", menu);

			panel->add_item("_Copy", "C-c", 2);
			panel->add_separator();
			remove = panel->add_item("_Paste", "C-v", 3);
			panel->add_item("_Foo", NULL, REMOVE_MENU_ID);
		}

		void menu_activated(int id) {
			if (id == REMOVE_MENU_ID) {
				menu->remove_menu(empty_panel);
				panel->remove_item(remove);
				delete remove;
			}
		}
};

#define CALL_INIT init();
void init(void) {
	set_color_mode(false);
}
