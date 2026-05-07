#include "Keyboard.h"
#if defined(X11) || (!defined(__Wayland__) && !defined(WAYLAND))
void init_xkb()
{
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;

    char *cur_rules = nullptr;
    XkbRF_VarDefsRec cur;
    memset(&cur, 0, sizeof(cur));
    if (!XkbRF_GetNamesProp(dpy, &cur_rules, &cur)) {
        XCloseDisplay(dpy);
        return;
    }

    char *rules_copy = cur_rules ? strdup(cur_rules) : strdup("evdev");
    char *model_copy = cur.model ? strdup(cur.model) : strdup("pc105");
    char *variant_copy = cur.variant ? strdup(cur.variant) : nullptr;
    if (cur_rules) XFree(cur_rules);

    XkbRF_VarDefsRec newset;
    memset(&newset, 0, sizeof(newset));
    newset.model = model_copy;
    newset.variant = variant_copy;
    newset.layout = strdup("us,ru");

    if (!XkbRF_SetNamesProp(dpy, rules_copy, &newset)) {
        std::cerr << "Ошибка инициализации XKB\n";
    } else {
        XkbGetKeyboardByName(dpy, XkbUseCoreKbd, NULL,
                             XkbGBN_AllComponentsMask,
                             XkbGBN_AllComponentsMask & (~XkbGBN_GeometryMask),
                             True);
    }

    free(rules_copy); free(model_copy); free(newset.layout);
    free(variant_copy);
    XCloseDisplay(dpy);
}
void change_layout_x11(std::string layout)
{
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;

    XkbRF_VarDefsRec cur;
    memset(&cur, 0, sizeof(cur));
    char *rules_str = nullptr;
    if (!XkbRF_GetNamesProp(dpy, &rules_str, &cur)) {
        XCloseDisplay(dpy);
        return;
    }
    if (rules_str) XFree(rules_str);

    int group = 0;
    if (cur.layout) {
        std::string layoutsStr(cur.layout);
        std::string target = layout;
        size_t commaPos = target.find(',');
        if (commaPos != std::string::npos)
            target = target.substr(0, commaPos);

        std::stringstream ss(layoutsStr);
        std::string item;
        int index = 0;
        while (std::getline(ss, item, ',')) {
            item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());
            if (item == target) {
                group = index;
                break;
            }
            ++index;
        }
    }

    XkbLockGroup(dpy, XkbUseCoreKbd, group);
    XFlush(dpy);
    XCloseDisplay(dpy);
}
#endif
void Keyboard::uinput_init(){
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for(int i=2; i<=83; i++){
        ioctl(fd,UI_SET_KEYBIT,i);
    }
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Keyboard device");
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
}

void Keyboard::emit(int code, int val){
    struct input_event ie;
    ie.type = EV_KEY;
    ie.code = code;
    ie.value = val;
    write(fd, &ie, sizeof(ie));
    struct input_event syn;
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;
    syn.value = 0;
    write(fd, &syn, sizeof(syn));
}

Keyboard::Keyboard(){
    std::string ui_file = "keyboard.ui";
    builder = Gtk::Builder::create_from_file(ui_file);
    builder->get_widget("main_window", pDialog);
    auto display = Gdk::Display::get_default();
    auto gtk_window = pDialog->get_window();
    auto monitor = display->get_monitor_at_window(gtk_window);
    is_wayland = (getenv("WAYLAND_DISPLAY") != nullptr);

    Gdk::Rectangle rect;
    monitor->get_geometry(rect);

	
    int width = rect.get_width();
    if (width <= 1024){
        ui_file = "keyboard_min.ui";
        builder = Gtk::Builder::create_from_file(ui_file);
        builder->get_widget("main_window", pDialog);
    }
    uinput_init();
    pDialog->signal_button_press_event().connect(sigc::mem_fun(*this, &Keyboard::on_window_button_press), false);
    pDialog->signal_motion_notify_event().connect(sigc::mem_fun(*this, &Keyboard::on_window_motion), false);
    pDialog->signal_button_release_event().connect(sigc::mem_fun(*this, &Keyboard::on_window_button_release), false);

    Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
    css_provider->load_from_path("custom_keyboard.css");
    pDialog->get_style_context()->add_provider_for_screen(Gdk::Screen::get_default(),css_provider,GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    pDialog->set_title(" ");
    
    builder->get_widget("grid", grid);
    auto button = grid->get_children();
    for(auto widget : button){
        Gtk::Button* btn = static_cast<Gtk::Button*>(widget);
        Glib::ustring text = btn->Gtk::Buildable::get_name();
        if (text == "button_alt"){
            btn->signal_clicked().connect(sigc::mem_fun(*this, &Keyboard::on_button_clicked_alt));
            management_buttons.push_back(btn);
        }
        else if (text == "button_caps"){
            btn->signal_clicked().connect(sigc::mem_fun(*this, &Keyboard::on_button_clicked_caps));
            management_buttons.push_back(btn);
        }
        else if (text == "button_transparent_mode"){
            btn->signal_clicked().connect(sigc::mem_fun(*this, &Keyboard::on_button_clicked_transparent));
            management_buttons.push_back(btn);
        }
        else if (text == "button_punctuation")
            btn->signal_clicked().connect(sigc::mem_fun(*this, &Keyboard::on_button_clicked_punctuation));
        else if (text == "button_BackSpace"){
            btn->signal_pressed().connect(sigc::mem_fun(*this, &Keyboard::on_button_clicked_backspace));
            btn->signal_released().connect(sigc::mem_fun(*this, &Keyboard::on_button_release_backspace));
        }
        else{
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &Keyboard::on_button_clicked), btn));
            buttons.push_back(btn);
        }
    }
    pDialog->signal_show().connect(sigc::mem_fun(*this, &Keyboard::load_settings));
    pDialog->set_keep_above(true);
    pDialog->set_accept_focus(false);
    pDialog->signal_hide().connect(sigc::mem_fun(*this, &Keyboard::on_window_close));
}

void Keyboard::on_button_clicked_backspace(){
    emit(KEY_BACKSPACE, 1);
}
void Keyboard::on_button_release_backspace(){
    emit(KEY_BACKSPACE, 0);
}

bool Keyboard::on_window_button_press(GdkEventButton* event) {
    if (event->type == GDK_BUTTON_PRESS) {
        dragging = true;
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        pDialog->get_position(window_start_x, window_start_y);
        return true;
    }
    return false;
}

bool Keyboard::on_window_motion(GdkEventMotion* event) {
    if (dragging) {
        int dx = event->x_root - drag_start_x;
        int dy = event->y_root - drag_start_y;
        pDialog->move(window_start_x + dx, window_start_y + dy);
        return true;
    }
    return false;
}

bool Keyboard::on_window_button_release(GdkEventButton* event) {
    if (event->button == 1) {
        dragging = false;
        return true;
    }
    return false;
}

void Keyboard::run(Glib::RefPtr<Gtk::Application> app){
    if (pDialog){
        app->run(*pDialog);
    }
}

void Keyboard::on_window_close(){
    Glib::KeyFile keyfile;
    keyfile.set_boolean("Settings", "alt", flag_alt);
    keyfile.set_boolean("Settings", "caps", flag_caps);
    keyfile.set_boolean("Settings", "transparent", flag_transparent);
    keyfile.set_boolean("Settings", "punctuation", flag_punctuation);
    try {
        keyfile.save_to_file(config_path);
    } catch (std::exception e) {
        std::cerr << "Ошибка сохранения настроек: " << e.what() << "\n";
    }
    if (!is_wayland)
		change_layout_x11("ru,us");
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
}

void Keyboard::load_settings(){
	init_xkb();
    Glib::KeyFile keyfile;
    try {
        keyfile.load_from_file(config_path);
    } catch (Glib::FileError e) {
        return;
    }
    flag_alt = keyfile.get_boolean("Settings", "alt");
    flag_caps = keyfile.get_boolean("Settings", "caps");
    flag_transparent = keyfile.get_boolean("Settings", "transparent");
    flag_punctuation = keyfile.get_boolean("Settings", "punctuation");
    change_layout_x11("us");
    if (flag_alt){
        flag_alt = false;
        on_button_clicked_alt();
        if (!is_wayland)
            change_layout_x11("ru");
    }
    flag_caps = !flag_caps;
    on_button_clicked_caps();
    if (flag_transparent){
        flag_transparent = false;
        on_button_clicked_transparent();
    }
    if (flag_punctuation){
        flag_punctuation = false;
        on_button_clicked_punctuation();
    }
}

int Keyboard::get_keycode(Glib::ustring name){
    Glib::ustring text = name.substr(7);
    flag_shift = false;
    if (text == "space")
        return KEY_SPACE;
    if (text == "Return")
        return KEY_ENTER;
    auto result = letters.find(text);
    if (result != end(letters)){
        return letters[text];
    }
    auto res = letters_punct.find(text);
    if (res != end(letters_punct)){
        std::vector<int> с = letters_punct[text];
        if (с[1])
            flag_shift = true;
        if (text == "№"){
            if (!is_wayland)
                change_layout_x11("ru");
            flag_nomer = true;
        }
        return с[0];
    }
    if (flag_punctuation){
        std::vector<int> res = punct_map[text];
        if (res[2])
            flag_shift = true;
        return res[1];
    }
    return keymap[text];
}

void Keyboard::on_button_clicked(Gtk::Button* btn){
    Glib::ustring name = btn->Gtk::Buildable::get_name();
    int key = get_keycode(name);
    if (flag_shift)
        emit(KEY_LEFTSHIFT, 1);
    emit(key, 1);
    emit(key, 0);
    if (flag_shift)
        emit(KEY_LEFTSHIFT, 0);
    if (flag_nomer){
        flag_nomer = false;
        if (!is_wayland)
            change_layout_x11("us");
    }
}

void Keyboard::on_button_clicked_alt(){
    Gtk::Button* btn = nullptr;
    builder->get_widget("button_alt", btn);
    if (!flag_alt){
        flag_alt = true;
        btn->set_label("EN");
        if (!is_wayland)
            change_layout_x11("ru");
        setup_russian_names();
    }
    else{
        flag_alt = false;
        btn->set_label("RU");
        for(Gtk::Button* btn : buttons){
            Glib::ustring text = btn->Gtk::Buildable::get_name();
            Glib::ustring s = text.substr(7);
            if (s != "space" && s != "Return"){
                if (flag_caps)
                    s = s.uppercase();
                btn->set_label(s);
            }
        }
        int col = 1;
        for (Gtk::Button* btn : ru_buttons) {
            btn->set_label(std::to_string(col));
            Glib::ustring text = "button_" + std::to_string(col);
            btn->Gtk::Buildable::set_name(text);
            col++;
            if (col == 10)
                col = 0;
        }
        ru_buttons.clear();
        if (!is_wayland)
            change_layout_x11("us");
    }
}

bool Keyboard::emulate_caps_lock(){
    GdkDisplay* display = gdk_display_get_default();
    GdkKeymap* keymap = gdk_keymap_get_for_display(display);
    return gdk_keymap_get_caps_lock_state(keymap);
}

void Keyboard::on_button_clicked_caps(){
    if (emulate_caps_lock() != flag_caps){
        emit(KEY_CAPSLOCK, 1);
        emit(KEY_CAPSLOCK, 0);
    }
    if (!flag_caps){
        flag_caps = true;
        for(Gtk::Button* btn : buttons){
            Glib::ustring text = btn->get_label();
            if (text != "" && text != "↵"){
                btn->set_label(text.uppercase());
            }
        }
        for(Gtk::Button* btn : ru_buttons)
            btn->set_label(btn->get_label().uppercase());
    }
    else{
        flag_caps = false;
        for(Gtk::Button* btn : buttons){
            Glib::ustring text = btn->get_label();
            if (text != "" && text != "↵"){
                btn->set_label(text.lowercase());
            }
        }
    }
    emit(KEY_CAPSLOCK, 1);
    emit(KEY_CAPSLOCK, 0);
}

void Keyboard::on_button_clicked_transparent(){
    if (!flag_transparent){
        pDialog->set_opacity(0.5);
        flag_transparent = true;
    }
    else{
        flag_transparent = false;
        pDialog->set_opacity(1);
    }
}

void Keyboard::on_button_clicked_punctuation(){
    if (!flag_punctuation){
        flag_punctuation = true;
        if (flag_alt){
            on_button_clicked_alt();
            flag_alt = true;
        }
        setup_punct_names();
        for (Gtk::Button* btn : management_buttons) {
            grid->remove(*btn);
        }
    }
    else{
        flag_punctuation = false;
        for (Gtk::Button* btn : punct_buttons) {
            grid->remove(*btn);
            delete btn;
        }
        if (flag_alt){
            flag_alt = false;
            on_button_clicked_alt();
        }
        else{
            flag_alt = true;
            on_button_clicked_alt();
        }
        punct_buttons.clear();
        grid->attach(*management_buttons[0], 1, 3, 1, 1);
        grid->attach(*management_buttons[1], 0, 3, 1, 1);
        grid->attach(*management_buttons[2], 0, 2, 1, 1);
    }
}

void Keyboard::setup_russian_names(){
    for(Gtk::Button* btn : buttons){
        Glib::ustring text = btn->Gtk::Buildable::get_name();
        Glib::ustring s = text.substr(7);
        if (s != "space" && s != "Return"){
            s = ru_map[s];
            if (flag_caps)
                s = s.uppercase();
            btn->set_label(s);
        }
    }
    int col = 0;
    for (auto par : letters){
        Glib::ustring letter = par.first;
        Glib::ustring text = "button_" + letter;
        if (flag_caps)
            letter = letter.uppercase();
        Gtk::Button* button = static_cast<Gtk::Button*>(grid->get_child_at(col, 0));
        button->Gtk::Buildable::set_name(text);
        button->set_label(letter);
        col++;
        ru_buttons.push_back(button);
    }
    grid->show_all();
}

void Keyboard::setup_punct_names(){
    for(Gtk::Button* btn : buttons){
        Glib::ustring text = btn->Gtk::Buildable::get_name();
        Glib::ustring s = text.substr(7);
        if (s != "space" && s != "Return"){
            std::vector<int> res = punct_map[s];
            char x = (char)res[0];
            s = x;
            btn->set_label(s);
        }
    }
    int col = 0, row = 2;
    for (auto par : letters_punct){
        Glib::ustring letter = par.first;
        Glib::ustring text = "button_" + letter;
        Gtk::Button* btn = new Gtk::Button(letter);
        btn->Gtk::Buildable::set_name(text);
        grid->attach(*btn, col, row, 1, 1);
        row++;
        if (row == 4){
            row--;
            col++;
        }
        btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &Keyboard::on_button_clicked), btn));
        punct_buttons.push_back(btn);
    }
    grid->show_all();
}
