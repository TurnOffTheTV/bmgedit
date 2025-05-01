#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <filesystem>

//Wether to open editor as read-only
extern bool readOnly;
//Wether to create new file in editor
extern bool newFile;
//Wether to use color codes
extern bool useColorCodes;
//Wether to print debug info
extern bool developerMode;

//Run the actual BMG editor
int runEditor(std::filesystem::path filename);

//Get length of BMG message
unsigned int bmgMessageLength(char* msgBuffer);
//Print BMG message to console, reading escaped sequences
void bmgPrintMessage(std::string msg,unsigned int x,unsigned int y);

#endif