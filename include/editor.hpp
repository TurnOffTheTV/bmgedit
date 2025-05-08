#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <filesystem>
#include <BmgEntry.hpp>

//Wether to open editor as read-only
extern bool readOnly;
//Wether to create new file in editor
extern bool newFile;
//Wether to use color codes
extern bool useColorCodes;
//Wether to print debug info
extern bool developerMode;

enum EditorScreen {
	PICK_ENTRY,
	EDIT_ENTRY,
	ENTRY_MENU,
	PICK_CONTROL,
	CONTROL_DETAIL
};

//Run the actual BMG editor
int runEditor(std::filesystem::path filename);

//Save the file
void saveBmgFile(std::filesystem::path path,std::vector<BmgEntry> entries,unsigned int numEntries,unsigned short entryLength);

//Get length of BMG message
unsigned int bmgMessageLength(char* msgBuffer);
//Print first line of BMG message at cursor position
void bmgPrintLine(std::string msg);
//Print full BMG message to console, reading escaped sequences
void bmgPrintMessage(std::string msg,unsigned int x,unsigned int y,unsigned int entryCharIndex);

//Get row # of character in BMG
unsigned int bmgGetRow(std::string msg,unsigned int index);
//Get column # of character in BMG
unsigned int bmgGetColumn(std::string msg,unsigned int index);
//Get the position in the string that the cursor is at
unsigned int bmgCursorIndex(std::string msg,unsigned int cursorPos);
#endif