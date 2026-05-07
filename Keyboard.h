#pragma once
#include <iostream>
#include <gtkmm.h>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <dev/evdev/uinput.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#if defined(X11) || (!defined(__Wayland__) && !defined(WAYLAND))
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <dlfcn.h>
#include <X11/extensions/XKBrules.h> 
#endif
class Keyboard{
private:
    Gtk::Window* pDialog = nullptr;
    std::vector<Gtk::Button*> buttons;
    std::vector<Gtk::Button*> ru_buttons;
    std::vector<Gtk::Button*> punct_buttons;
    std::vector<Gtk::Button*> management_buttons;
    std::string config_path = "config.ini";
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Grid* grid = nullptr;
    bool flag_alt = false;
    bool flag_caps = false;
    bool flag_transparent = false;
    bool flag_punctuation = false;
    bool is_wayland;
    std::map<Glib::ustring, Glib::ustring> ru_map = {
        {"q", "й"}, {"w", "ц"}, {"e", "у"}, {"r", "к"}, {"t", "е"}, {"y", "н"}, {"u", "г"}, {"i", "ш"}, {"o", "щ"}, {"p", "з"},
        {"a", "ф"}, {"s", "ы"}, {"d", "в"}, {"f", "а"}, {"g", "п"}, {"h", "р"}, {"j", "о"}, {"k", "л"}, {"l", "д"},
        {"z", "я"}, {"x", "ч"}, {"c", "с"}, {"v", "м"}, {"b", "и"}, {"n", "т"}, {"m", "ь"}
    };
    std::map<Glib::ustring, std::vector<int>> punct_map ={
        {"q", {(int)'=', KEY_EQUAL, 0}}, {"w", {(int)'+', KEY_EQUAL, 1}}, {"e", {(int)'\\', KEY_BACKSLASH, 0}}, {"r", {(int)'"', KEY_APOSTROPHE, 1}}, 
        {"t", {(int)'1', KEY_1, 0}}, {"y", {(int)'2', KEY_2, 0}}, 
        {"u", {(int)'3', KEY_3, 0}}, {"i", {(int)'4', KEY_4, 0}}, {"o", {(int)'5', KEY_5, 0}}, {"p", {(int)'0', KEY_0, 0}},
        {"a", {(int)'`', KEY_GRAVE, 0}}, {"s", {(int)'~',KEY_GRAVE, 1}}, {"d", {(int)'!', KEY_1, 1}}, {"f", {(int)'[', KEY_LEFTBRACE, 0}}, {"g", {(int)']', KEY_RIGHTBRACE, 0}}, 
        {"h", {(int)'{', KEY_LEFTBRACE, 1}}, {"j", {(int)'}', KEY_RIGHTBRACE, 1}}, {"k", {(int)'/', KEY_SLASH, 0}}, {"l", {(int)'?', KEY_SLASH, 1}},
        {"z", {(int)',', KEY_COMMA, 0}}, {"x", {(int)'.', KEY_DOT, 0}}, {"c", {(int)'<', KEY_COMMA, 1}}, {"v", {(int)'>', KEY_DOT, 1}}, {"b", {(int)';', KEY_SEMICOLON, 0}}, 
        {"n", {(int)':', KEY_SEMICOLON, 1}}, {"m", {(int)'@', KEY_2, 1}},
        {"1", {(int)'#', KEY_3, 1}}, {"2", {(int)'$', KEY_4, 1}}, {"3", {(int)'%', KEY_5, 1}}, {"4", {(int)'^', KEY_6, 1}}, {"5", {(int)'&', KEY_7, 1}}, {"6", {(int)'*', KEY_8, 1}},
        {"7", {(int)'(', KEY_9, 1}}, {"8", {(int)')', KEY_0, 1}}, {"9", {(int)'-', KEY_MINUS, 0}}, {"0", {(int)'_', KEY_MINUS, 1}}
    };
    std::map<Glib::ustring,int> letters = { {"х", KEY_LEFTBRACE}, {"ъ", KEY_RIGHTBRACE}, {"ж", KEY_SEMICOLON}, {"э", KEY_APOSTROPHE}, {"б", KEY_COMMA}, {"ю", KEY_DOT}, {"ё", KEY_GRAVE},
        {"\\", KEY_BACKSLASH}, {"-", KEY_MINUS}, {"=", KEY_EQUAL}};
    std::map<Glib::ustring,std::vector<int>> letters_punct = { {"№", {KEY_3, 1}}, {"\'", {KEY_APOSTROPHE, 0}}, {"|", {KEY_BACKSLASH, 1}}};
    bool dragging = false;
    int drag_start_x, drag_start_y;
    int window_start_x, window_start_y;
    
    int fd;
    std::map<Glib::ustring,int> keymap = { {"a", KEY_A}, {"b", KEY_B}, {"c", KEY_C}, {"d", KEY_D}, {"e", KEY_E}, 
    {"f", KEY_F}, {"g", KEY_G}, {"h", KEY_H}, {"i", KEY_I}, {"j", KEY_J}, 
    {"k", KEY_K}, {"l", KEY_L}, {"m", KEY_M}, {"n", KEY_N}, {"o", KEY_O}, 
    {"p", KEY_P}, {"q", KEY_Q}, {"r", KEY_R}, {"s", KEY_S}, {"t", KEY_T}, 
    {"u", KEY_U}, {"v", KEY_V}, {"w", KEY_W}, {"x", KEY_X}, {"y", KEY_Y}, {"z", KEY_Z},
    {"1", KEY_1}, {"2", KEY_2}, {"3", KEY_3}, {"4", KEY_4}, {"5", KEY_5}, {"6", KEY_6}, {"7", KEY_7}, {"8", KEY_8}, {"9", KEY_9}, {"0", KEY_0}};
    bool flag_shift;
    bool flag_nomer;

public:
    Keyboard();
    void run(Glib::RefPtr<Gtk::Application> app);
    void on_button_clicked(Gtk::Button* btn);
    void on_button_clicked_alt();
    void on_button_clicked_caps();
    void on_button_clicked_transparent();
    void on_button_clicked_punctuation();
    void on_window_close();
    bool on_window_button_press(GdkEventButton* event);
    bool on_window_motion(GdkEventMotion* event);
    bool on_window_button_release(GdkEventButton* event);
    void setup_russian_names();
    void setup_punct_names();
    void load_settings();
    
    void uinput_init();
    void emit(int code, int val);
    int get_keycode(Glib::ustring name);
    
    void on_button_clicked_backspace();
    void on_button_release_backspace();
    
    bool emulate_caps_lock();
};

#if defined(X11) || (!defined(__Wayland__) && !defined(WAYLAND))
void change_layout_x11(std::string layout, std::string options);
#else
inline void change_layout_x11(std::string, std::string = "") {
}
#endif
