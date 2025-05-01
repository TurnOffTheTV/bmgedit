#ifndef TERMINAL_HPP
#define TERMINAL_HPP

//Width (# of columns) of terminal
extern unsigned int terminalWidth;
//Height (# of rows) of terminal
extern unsigned int terminalHeight;

//Update terminalWidth and terminalHeight vars
void updateTerminalSize();
//Clear screen and set cursor to 0,0
void clearTerminalScreen();

//Print the command help
void printHelp();

//Hide the cursor
void hideCursor();
//Show the cursor
void showCursor();
//Set the cursor's position
void moveCursor(unsigned int x,unsigned int y);

enum TerminalColor {
	BLACK=30,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	DEFAULT,
	BRIGHT_BLACK=90,
	BRIGHT_RED,
	BRIGHT_GREEN,
	BRIGHT_YELLOW,
	BRIGHT_BLUE,
	BRIGHT_MAGENTA,
	BRIGHT_CYAN,
	BRIGHT_WHITE
};

//Set color of text using color codes
void setTextColor(enum TerminalColor color);
//Set color of text using rgb
void setTextColor(unsigned char r,unsigned char g,unsigned char b);
//Set color of background using color codes
void setBackgroundColor(enum TerminalColor color);
//Set color of background using rgb
void setBackgroundColor(unsigned char r,unsigned char g,unsigned char b);

#endif