#define REMOVE_MENU_ID 7

class main_window_t : public main_window_base_t {
  menu_bar_t *menu;
  menu_panel_t *panel, *empty_panel;
  menu_item_base_t *remove;

 public:
  main_window_t(void) {
    menu = emplace_back<menu_bar_t>();
    menu->connect_activate([this](int id) { menu_activated(id); });

    panel = menu->insert_menu(nullptr, "_File");

    panel->insert_item(nullptr, "_Open", "C-o", 0);
    panel->insert_item(nullptr, "_Close", "C-w", 1);

    empty_panel = menu->insert_menu(nullptr, "E_mpty");
    empty_panel->insert_item(nullptr, "Remove", "", REMOVE_MENU_ID);

    panel = menu->insert_menu(nullptr, "_Edit");

    panel->insert_item(nullptr, "_Copy", "C-c", 2);
    panel->insert_separator();
    remove = panel->insert_item(nullptr, "_Paste", "C-v", 3);
    panel->insert_item(nullptr, "_Foo", "", REMOVE_MENU_ID);
  }

  void menu_activated(int id) {
    if (id == REMOVE_MENU_ID) {
      // Note: this causes a memory leak, but deleting the panel while its handler is running causes
      // a segmentation fault.
      menu->remove_menu(empty_panel).release();
      panel->remove_item(remove);
    }
  }
};
