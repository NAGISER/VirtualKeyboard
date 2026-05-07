#include "Keyboard.h"

int main(int argc, char* argv[]) {
	setlocale(LC_ALL,"RUS");
	auto app = Gtk::Application::create("org.example.keyboard");
	Keyboard keyboard;
	keyboard.run(app);
	return 0;
}
