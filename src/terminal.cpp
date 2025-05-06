#include <terminal.hpp>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

unsigned int terminalWidth = 0;
unsigned int terminalHeight = 0;

void updateTerminalSize(){
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	terminalWidth=size.ws_col;
	terminalHeight=size.ws_row;
}

void clearTerminalScreen(){
	std::cout << "\x1b[2J\x1b[H";
}

void printHelp(){
	std::cout << "\x1b[7mBMGEdit\x1b[27m" << std::endl;
	std::cout << "\x1b[1mUsage:\x1b[22m" << std::endl;
	std::cout << "bmgedit <args> <filename.bmg>" << std::endl;
	std::cout << "\x1b[1mArgs:\x1b[22m" << std::endl;
	std::cout << "-h : Display this help page. This will prevent the editor from opening." << std::endl;
	std::cout << "-v : Print the version. This will prevent the editor from opening." << std::endl;
	std::cout << "-r : Open file as read-only." << std::endl;
	std::cout << "-c : Create a new BMG file with the filename specified. This will overwrite any existing file as well." << std::endl;
	std::cout << "-a : Use ANSI color codes instead of RGB values when rendering text. Use this if colors don't show up correctly in your terminal." << std::endl;
	std::cout << "-d : Enable debug information, and display more BMG info for advanced users." << std::endl;
}

void hideCursor(){
	std::cout << "\x1b[?25l";
}

void showCursor(){
	std::cout << "\x1b[?25h";
}

void moveCursor(unsigned int x,unsigned int y){
	printf("\x1b[%i;%if",y,x);
}

void setTextColor(enum TerminalColor color){
	std::cout << "\x1b[" << color << 'm';
}

void setTextColor(unsigned char r,unsigned char g,unsigned char b){
	printf("\x1b[38;2;%i;%i;%im",r,g,b);
}

void setBackgroundColor(enum TerminalColor color){
	std::cout << "\x1b[" << (color+10) << 'm';
}

void setBackgroundColor(unsigned char r,unsigned char g,unsigned char b){
	printf("\x1b[48;2;%i;%i;%im",r,g,b);
}