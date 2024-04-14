#include <stdio.h>
 
// Library where getch() is stored
#include <curses.h>
 

int main() {
    initscr();          // Initialize the window
    noecho();           // Turn off echo for character inputs
    cbreak();           // Disable line buffering, making input available immediately

    printw("Press any key: ");
    int ch = getch();   // Read a character from the keyboard

    // Optional: Do something with the input
    printw("You pressed: %c", ch);
    // while(1) {}
    endwin();           // End curses mode
    return 0;
}