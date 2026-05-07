Compile the program using the command

clang++ main.cpp Keyboard.cpp -o main `pkg-config --cflags --libs gtkmm-3.0` -lX11 -lxkbfile

Run the program using administrator rights
