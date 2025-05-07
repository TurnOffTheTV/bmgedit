#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <editor.hpp>
#include <terminal.hpp>
#include <isLittleEndian.hpp>
#include "empty_bmg.h"

bool readOnly = false;
bool newFile = false;
bool useColorCodes = false;
bool developerMode = false;

//Message to display when there is an error
const std::string errorMessage = "!!!ERROR!!!\nMessage could not\nbe loaded.";

//Control code representation
const char* ctrl_unknown = "[unknown control code]";
const char* ctrl_slow = "[slow display]";
const char* ctrl_speed = "[speed:%i]";
const char* ctrl_autoClose = "[auto-close]";
const char* ctrl_selectYes = "[yes option:%s]";
const char* ctrl_selectNo = "[no option:%s]";
const char* ctrl_numBananas = "[# of bananas]";
const char* ctrl_numCoconuts = "[# of coconuts]";
const char* ctrl_numPineapples = "[# of pineapples]";
const char* ctrl_numDurians = "[# of durians]";
const char* ctrl_piantaRecord = "[pianta village piantissimo record]";
const char* ctrl_gelatoRecord = "[gelato beach piantissimo record]";
const char* ctrl_boxRecord = "[box game record]";
const char* ctrl_numBlueShines = "[# of blue coin shines]";
const char* ctrl_nokiRecord = "[noki bay piantissimo record]";
const char* ctrl_color = "[text color:%s]";
const char* ctrl_white = "white";
const char* ctrl_grey = "grey";
const char* ctrl_red = "red";
const char* ctrl_blue = "blue";
const char* ctrl_yellow = "yellow";
const char* ctrl_green = "green";
const char* ctrl_garbage = "garbage";

//Current screen in editor
enum EditorScreen screen = PICK_ENTRY;

//Wether the editor cursor is on a control sequence
bool cursorOnCtrl = false;

int runEditor(std::filesystem::path filename){
	//Error if file doesn't exist
	if(!newFile && !std::filesystem::exists(filename)){
		std::cout << filename << " does not exist." << std::endl;
		return 1;
	}

	//Open stream
	std::fstream bmgFile;
	if(newFile){
		bmgFile.open(filename,std::fstream::trunc | std::fstream::in | std::fstream::out);
	}else{
		bmgFile.open(filename,std::fstream::in);
	}

	//Check for errors opening file
	if(!bmgFile.is_open()){
		std::cout << "Failed to open file at " << filename << '.' << std::endl;
		return 1;
	}

	if(newFile){
		bmgFile.write((const char*)empty_bmg_data,empty_bmg_size);
		bmgFile.flush();
		bmgFile.seekp(std::fstream::beg);
	}

	//Check file magic
	char* magic = (char*)malloc(4);
	bmgFile.read(magic,4);
	if(strcmp(magic,"MESG")){
		std::cout << "File is not a BMG file." << std::endl;
		return 1;
	}

	//Check bmg version
	char* bmgType = (char*)malloc(4);
	bmgFile.read(bmgType,4);
	if(strcmp(bmgType,"bmg1")){
		std::cout << "This type of BMG file is not supported. (V)" << std::endl;
		return 1;
	}

	//Get number of sections
	bmgFile.seekp(12);
	unsigned int numSections;
	bmgFile.read(reinterpret_cast<char *>(&numSections),4);
	if(isLittleEndian()){
		unsigned int temp = numSections;
		numSections=std::byteswap(temp);
	}
	if(numSections!=2){
		//Error out if sections aren't as expected
		std::cout << "This type of BMG file is not supported. (S)" << std::endl;
		return 1;
	}

	//Get length of entry info section
	bmgFile.seekp(36);
	unsigned int infLength;
	bmgFile.read(reinterpret_cast<char *>(&infLength),4);
	if(isLittleEndian()){
		unsigned int temp = infLength;
		infLength=std::byteswap(temp);
	}

	//Save message section to buffer
	bmgFile.seekp(infLength+32+8);
	char* messagesBuffer = (char*)malloc(std::filesystem::file_size(filename)-infLength-32-8);
	bmgFile.read(messagesBuffer,std::filesystem::file_size(filename)-infLength-32-8);

	//Get entry info
	bmgFile.seekp(40);
	unsigned short numEntries;
	bmgFile.read(reinterpret_cast<char *>(&numEntries),2);
	if(isLittleEndian()){
		unsigned short temp = numEntries;
		numEntries=std::byteswap(temp);
	}
	unsigned short entryLength;
	bmgFile.read(reinterpret_cast<char *>(&entryLength),2);
	if(isLittleEndian()){
		unsigned short temp = entryLength;
		entryLength=std::byteswap(temp);
	}

	//Create entry list
	bmgFile.seekp(48);
	//BmgEntry entries[numEntries];

	std::vector<BmgEntry> entries;

	for(unsigned int i=0;i<numEntries;i++){
		BmgEntry newEntry;

		//Save entry pointer
		unsigned int entryStart = bmgFile.tellp();
		//Get message offset
		unsigned int msgOffset;
		bmgFile.read(reinterpret_cast<char *>(&msgOffset),4);
		if(isLittleEndian()){
			unsigned int temp = msgOffset;
			msgOffset=std::byteswap(temp);
		}

		//Copy message
		newEntry.message = std::string(messagesBuffer+msgOffset,bmgMessageLength(messagesBuffer+msgOffset));

		newEntry.charLength=0;

		for(unsigned int j=0;j<newEntry.message.length();j++){
			newEntry.charLength++;
			if(newEntry.message[j]=='\x1a'){
				unsigned char escLength = newEntry.message[j+1];
				j+=escLength-1;
				continue;
			}
		}
		
		if(entryLength>4){

			//Start frame
			unsigned short startFrame;
			bmgFile.read(reinterpret_cast<char *>(&startFrame),2);
			if(isLittleEndian()){
				unsigned short temp = startFrame;
				startFrame=std::byteswap(temp);
			}
			newEntry.startFrame=startFrame;

			//End frame
			unsigned short endFrame;
			bmgFile.read(reinterpret_cast<char *>(&endFrame),2);
			if(isLittleEndian()){
				unsigned short temp = endFrame;
				endFrame=std::byteswap(temp);
			}
			newEntry.endFrame=endFrame;

			//Sound ID
			bmgFile.read(reinterpret_cast<char *>(&newEntry.soundID),1);

		}

		entries.push_back(newEntry);

		//Skip to next entry
		bmgFile.seekp(entryStart+entryLength);
	}

	bmgFile.close();

	//Run UI
	system("stty raw");
	bool runProgram = true;
	unsigned int entryIndex = 0;
	unsigned int entryPickPage = 0;
	unsigned int entryCharIndex = 0;
	unsigned int menuIndex = 0;
	unsigned int menuPage = 0;
	unsigned int controlIndex = 0;
	unsigned int controlPickPage = 0;
	while(runProgram){
		//Set up terminal
		setBackgroundColor(BLACK);
		clearTerminalScreen();
		updateTerminalSize();
		hideCursor();
		setTextColor(WHITE);
		
		//Draw header
		moveCursor(1,1);
		std::cout << "\x1b[1mBMGEdit\x1b[22m";
		if(readOnly) std::cout << " | Read-only";
		if(screen==PICK_ENTRY) std::cout << " | Press 'q' to quit";
		if(screen==EDIT_ENTRY && !readOnly) std::cout << " | Press ^e for menu";
		if(screen==ENTRY_MENU || screen==PICK_CONTROL || screen==CONTROL_DETAIL || screen==EDIT_ENTRY && readOnly) std::cout << " | Press 'q' to return to entry list";
		moveCursor(1,2);
		for(unsigned int i=0;i<terminalWidth;i++) std::cout << '=';

		//Draw screens
		if(screen!=ENTRY_MENU) menuIndex=0;
		if(screen!=PICK_CONTROL) controlIndex=0;
		switch(screen){
			case PICK_ENTRY:{
				bool lastPage = false;
				for(unsigned int i=entryPickPage*(terminalHeight-2);i<(entryPickPage+1)*(terminalHeight-2);i++){
					if(i>numEntries) break;
					moveCursor(1,i-entryPickPage*(terminalHeight-2)+3);
					if(i==numEntries){
						std::cout << "  New Entry";
						lastPage=true;
					}else{
						std::cout << "  Entry " << i << ": ";
						bmgPrintLine(entries[i].message);
						std::cout << "...";
					}
				}

				moveCursor(1,entryIndex-entryPickPage*(terminalHeight-2)+3);
				std::cout << '>';

				switch(getchar()){
					case 'q':
						runProgram=false;
					break;
					case '\x0d':
						entryCharIndex=0;
						screen=EDIT_ENTRY;
						if(entryIndex==numEntries){
							BmgEntry newEntry;
							newEntry.message="";
							if(entryLength>4){
								newEntry.startFrame=0;
								newEntry.endFrame=0;
								newEntry.charLength=0;
							}
							entries.push_back(newEntry);
							numEntries++;
						}
					break;
					case '\x1b':
						if(getchar()=='\x5b'){
							switch(getchar()){
								case '\x41':
									if(entryIndex>0){
										entryIndex--;
										if(entryIndex-entryPickPage*(terminalHeight-2)+3<3) entryPickPage--;
									}
								break;
								case '\x42':
									if(lastPage){
										if(entryIndex<numEntries){
											entryIndex++;
											if(entryIndex-entryPickPage*(terminalHeight-2)>terminalHeight-3) entryPickPage++;
										}
									}else{
										if(entryIndex<numEntries-1){
											entryIndex++;
											if(entryIndex-entryPickPage*(terminalHeight-2)>terminalHeight-3) entryPickPage++;
										}
									}
								break;
							}
						}
					break;
				}
			break;}
			case EDIT_ENTRY:{
				bmgPrintMessage(entries[entryIndex].message,1,3,entryCharIndex);

				if(!readOnly){
					showCursor();
					moveCursor(bmgGetColumn(entries[entryIndex].message,entryCharIndex)+1,bmgGetRow(entries[entryIndex].message,entryCharIndex)+3);
				}

				if(developerMode) std::cout << (int)getchar();

				char keyChar = getchar();

				if(keyChar>=32 && keyChar!=35 && keyChar!=36 && keyChar!=37 && keyChar!=43 && keyChar!=60 && keyChar!=62 && keyChar!=64 && keyChar!=127/* && keyChar!=165*/){
					entries[entryIndex].message.insert(entryCharIndex,{keyChar});
					entryCharIndex++;
				}

				if(keyChar==1){
					entries[entryIndex].message.insert(entryCharIndex,"@");
					entryCharIndex++;
				}
				if(keyChar==2){
					entries[entryIndex].message.insert(entryCharIndex,"#");
					entryCharIndex++;
				}
				if(keyChar==3){
					entries[entryIndex].message.insert(entryCharIndex,"%");
					entryCharIndex++;
				}
				if(keyChar==12){
					entries[entryIndex].message.insert(entryCharIndex,"<");
					entryCharIndex++;
				}
				if(keyChar==18){
					entries[entryIndex].message.insert(entryCharIndex,">");
					entryCharIndex++;
				}
				if(keyChar==24){
					entries[entryIndex].message.insert(entryCharIndex,"+");
					entryCharIndex++;
				}
				if(keyChar==25){
					entries[entryIndex].message.insert(entryCharIndex,"¥");
					entryCharIndex++;
				}
				if(keyChar==26){
					entries[entryIndex].message.insert(entryCharIndex,"$");
					entryCharIndex++;
				}

				if(keyChar==13){
					entries[entryIndex].message.insert(entryCharIndex,"\x0a");
					entryCharIndex++;
				}

				switch(keyChar){
					case 127:
						if(entryCharIndex>0 && !cursorOnCtrl){
							entries[entryIndex].message.erase(entryCharIndex-1,1);
							entryCharIndex--;
						}
					break;
					case 5:
						if(readOnly) break;
						hideCursor();
						screen=ENTRY_MENU;
					break;
					case 'q':
						if(!readOnly) break;
						screen=PICK_ENTRY;
					break;
					case '\x1b':
						if(getchar()=='\x5b'){
							switch(getchar()){
								case '\x44':
									if(entryCharIndex>0) entryCharIndex--;
								break;
								case '\x43':
									if(entryCharIndex<entries[entryIndex].charLength) entryCharIndex++;
								break;
							}
						}
					break;
				}
			break;}
			case ENTRY_MENU:
				for(unsigned int i=menuPage*(terminalHeight-2);i<(menuPage+1)*(terminalHeight-2);i++){
					if(i>=2) break;
					moveCursor(1,i-menuPage*(terminalHeight-2)+3);
					switch(i){
						case 0:
							std::cout << " Back to entry edit";
						break;
						case 1:
							std::cout << " Insert escape code";
						break;
					}
				}

				moveCursor(1,menuIndex-menuPage*(terminalHeight-2)+3);
				std::cout << '>';

				switch(getchar()){
					case 'q':
						screen=PICK_ENTRY;
						saveBmgFile(filename,entries,numEntries,entryLength);
					break;
					case '\x0d':
						switch(menuIndex){
							case 0:
								screen=EDIT_ENTRY;
							break;
							case 1:
								screen=PICK_CONTROL;
							break;
						}
					break;
					case '\x1b':
						if(getchar()=='\x5b'){
							switch(getchar()){
								case '\x41':
									if(menuIndex>0){
										menuIndex--;
										if(menuIndex-menuPage*(terminalHeight-2)+3<3) menuPage--;
									}
								break;
								case '\x42':
									if(menuIndex<1){
										menuIndex++;
										if(menuIndex-menuPage*(terminalHeight-2)>terminalHeight-3) menuPage++;
									}
								break;
							}
						}
					break;
				}
			break;
			case PICK_CONTROL:
				for(unsigned int i=controlPickPage*(terminalHeight-2);i<(controlPickPage+1)*(terminalHeight-2);i++){
					if(i>=4) break;
					moveCursor(1,i-controlPickPage*(terminalHeight-2)+3);
					switch(i){
						case 0:
							std::cout << " Delivery control";
						break;
						case 1:
							std::cout << " Option box";
						break;
						case 2:
							std::cout << " Read value";
						break;
						case 3:
							std::cout << " Text color";
						break;
					}
				}

				moveCursor(1,controlIndex-controlPickPage*(terminalHeight-2)+3);
				std::cout << '>';

				switch(getchar()){
					case 'q':
						screen=PICK_ENTRY;
					break;
					case '\x1b':
						if(getchar()=='\x5b'){
							switch(getchar()){
								case '\x41':
									if(controlIndex>0){
										controlIndex--;
										if(controlIndex-controlPickPage*(terminalHeight-2)+3<3) controlPickPage--;
									}
								break;
								case '\x42':
									if(controlIndex<3){
										controlIndex++;
										if(controlIndex-controlPickPage*(terminalHeight-2)>terminalHeight-3) controlPickPage++;
									}
								break;
							}
						}
					break;
				}
			break;
			case CONTROL_DETAIL:
				switch(getchar()){
					case 'q':
						screen=EDIT_ENTRY;
					break;
				}
			break;
		}
	}

	//Clean up
	system("stty cooked");
	showCursor();
	setTextColor(DEFAULT);
	setBackgroundColor(DEFAULT);
	clearTerminalScreen();

	//Return OK
	return 0;
}

void saveBmgFile(std::filesystem::path path,std::vector<BmgEntry> entries,unsigned int numEntries,unsigned short entryLength){
	std::fstream file;
	file.open(path,std::fstream::trunc | std::fstream::out);

	file.flush();

	std::vector<std::streampos> entryOffsets;

	unsigned int fileLength = 1;

	//Header
	file.write("MESGbmg1",8);//magic
	file.write("\x00\x00\x00\x00",4);//file length (will be filled later)
	file.write("\x00\x00\x00\x02",4);//section number
	for(int i=0;i<16;i++) file.write("\x00",1);//padding

	//Info section
	file.write("INF1",4);//magic

	unsigned char extraBytes = 0;
	unsigned int infLength = 16+numEntries*entryLength;
	while(infLength%32!=0){
		infLength++;
		extraBytes++;
	}
	fileLength+=infLength/32;
	if(isLittleEndian()){
		unsigned int temp = infLength;
		infLength=std::byteswap(temp);
	}
	file.write((const char*)&infLength,4);//section length

	unsigned int tNumEntries = numEntries;
	if(isLittleEndian()){
		unsigned short temp = tNumEntries;
		tNumEntries=std::byteswap(temp);
	}
	file.write((const char*)&tNumEntries,2);//entry number

	if(isLittleEndian()){
		unsigned short temp = entryLength;
		entryLength=std::byteswap(temp);
	}
	file.write((const char*)&entryLength,2);//entry length
	file.write("\x00\x00\x00\x00",4);

	unsigned int datLength = 9;
	unsigned int msgDataOffset = 1;
	for(unsigned int i=0;i<numEntries;i++){
		unsigned int thisOffset = msgDataOffset;
		if(isLittleEndian()){
			unsigned int temp = thisOffset;
			thisOffset=std::byteswap(temp);
		}
		file.write((const char*)&thisOffset,4);//entry length
		msgDataOffset+=entries[i].message.length()+1;
		datLength+=entries[i].message.length()+1;
		if(entryLength==4) continue;

		unsigned short startFrame = entries[i].startFrame;
		if(isLittleEndian()){
			unsigned short temp = startFrame;
			startFrame=std::byteswap(temp);
		}
		file.write((const char*)&startFrame,2);//start frame

		unsigned short endFrame = entries[i].endFrame;
		if(isLittleEndian()){
			unsigned short temp = endFrame;
			endFrame=std::byteswap(temp);
		}
		file.write((const char*)&endFrame,2);//end frame

		file << (unsigned char)entries[i].soundID;
		file.write("\x00\x00\x00",3);
	}

	for(unsigned char i=0;i<extraBytes;i++) file.write("\x00",1);//padding

	//Data section
	file.write("DAT1",4);//magic
	extraBytes=0;
	while(datLength%32!=0){
		datLength++;
		extraBytes++;
	}
	fileLength+=datLength/32;
	if(isLittleEndian()){
		unsigned int temp = datLength;
		datLength=std::byteswap(temp);
	}
	file.write((const char*)&datLength,4);//section length
	file.write("\x00",1);//padding
	for(unsigned int i=0;i<numEntries;i++){
		file.write(entries[i].message.c_str(),entries[i].message.length());//message
		file.write("\x00",1);//padding
	}

	for(unsigned char i=0;i<extraBytes;i++) file.write("\x00",1);//padding

	file.seekp(std::fstream::beg+8);
	if(isLittleEndian()){
		unsigned int temp = fileLength;
		fileLength=std::byteswap(temp);
	}
	file.write((const char*)&fileLength,4);//file length
}

unsigned int bmgMessageLength(char* msgBuffer){
	unsigned int length = 0;

	while(true){
		if(msgBuffer[length]=='\x00'){
			break;
		}
		if(msgBuffer[length]=='\x1a'){
			unsigned char escLength = msgBuffer[length+1];
			length+=escLength;
			continue;
		}
		length++;
	}

	return length;
}

void bmgPrintLine(std::string msg){
	for(unsigned int i=0;i<msg.length();i++){
		if(msg[i]=='\x1a'){
			unsigned char escLength = msg[i+1];
			i+=escLength-1;
			continue;
		}
		if(msg[i]=='\x0a'){
			break;
		}
		std::cout << msg[i];
	}
}

void bmgPrintMessage(std::string msg,unsigned int x,unsigned int y,unsigned int entryCharIndex){
	unsigned int line = y;
	bool useOption = false;
	std::string yesOption = "";
	std::string noOption = "";
	moveCursor(x,y);
	
	if(useColorCodes){
		setTextColor(BRIGHT_WHITE);
	}else{
		setTextColor(255,255,255);
	}
	if(useColorCodes){
		setBackgroundColor(BLUE);
	}else{
		setBackgroundColor(0,0,255);
	}


	unsigned int charLength = 0;
	for(unsigned int i=0;i<msg.length();i++){
		charLength++;
		if(msg[i]=='\x1a'){
			//Sure wish this part of the decomp was easier to read
			unsigned char escLength = msg[i+1];

			if(!readOnly){
				std::cout << "§";
				if(entryCharIndex==charLength) std::cout << " ";
			}

			switch(msg[i+2]){
				case '\x00':
					if(!readOnly && entryCharIndex!=charLength) break;
					//Message controls?
					if(escLength==5){
						if(msg[i+4]=='\x01') std::cout << ctrl_autoClose;
						if(msg[i+4]=='\x00') std::cout << ctrl_slow;
					}else if (escLength==6){
						if(msg[i+4]=='\x00') printf(ctrl_speed,(int)msg[i+5]);
					}else{
						std::cout << ctrl_unknown;
					}
				break;
				case '\x01':
					//Select settings
					if(readOnly){
						useOption=true;
						if(msg[i+4]=='\x00') yesOption=msg.substr(i+5,escLength-5);
						if(msg[i+4]=='\x01') noOption=msg.substr(i+5,escLength-5);
					}else if(entryCharIndex==charLength){
						if(msg[i+4]=='\x00') printf(ctrl_selectYes,msg.substr(i+5,escLength-5).c_str());
						if(msg[i+4]=='\x01') printf(ctrl_selectNo,msg.substr(i+5,escLength-5).c_str());
					}
				break;
				case '\x02':
					if(!readOnly && entryCharIndex!=charLength) break;
					//Get vars
					switch(msg[i+4]){
						case '\x00':
							std::cout << ctrl_piantaRecord;
						break;
						case '\x01':
							std::cout << ctrl_gelatoRecord;
						break;
						case '\x02':
							std::cout << ctrl_boxRecord;
						break;
						case '\x03':
							std::cout << ctrl_numBlueShines;
						break;
						case '\x04':
							if(escLength==6){
								if(msg[i+5]=='\x00') std::cout << ctrl_numBananas;
								if(msg[i+5]=='\x01') std::cout << ctrl_numCoconuts;
								if(msg[i+5]=='\x02') std::cout << ctrl_numPineapples;
								if(msg[i+5]=='\x03') std::cout << ctrl_numDurians;
							}else{
								std::cout << ctrl_unknown;
							}
						break;
						case '\x06':
							std::cout << ctrl_nokiRecord;
						break;
						default:
							std::cout << ctrl_unknown;
						break;
					}
				break;
				case '\xff':
					//Change text color
					switch(msg[i+5]){
						case '\x00':
							if(!readOnly && entryCharIndex==charLength) printf(ctrl_color,ctrl_white);
							if(useColorCodes){
								setTextColor(BRIGHT_WHITE);
							}else{
								setTextColor(255,255,255);
							}
						break;
						case '\x01':
							if(!readOnly && entryCharIndex==charLength) printf(ctrl_color,ctrl_grey);
							if(useColorCodes){
								setTextColor(WHITE);
							}else{
								setTextColor(200,200,200);
							}
						break;
						case '\x02':
							if(!readOnly && entryCharIndex==charLength) printf(ctrl_color,ctrl_red);
							if(useColorCodes){
								setTextColor(BRIGHT_RED);
							}else{
								setTextColor(255,0,0);
							}
						break;
						case '\x03':
							if(!readOnly && entryCharIndex==charLength) printf(ctrl_color,ctrl_blue);
							if(useColorCodes){
								setTextColor(BRIGHT_BLUE);
							}else{
								setTextColor(100,100,255);
							}
						break;
						case '\x04':
							if(!readOnly && entryCharIndex==charLength) printf(ctrl_color,ctrl_yellow);
							if(useColorCodes){
								setTextColor(BRIGHT_YELLOW);
							}else{
								setTextColor(255,255,0);
							}
						break;
						case '\x05':
							if(!readOnly && entryCharIndex==charLength) printf(ctrl_color,ctrl_green);
							if(useColorCodes){
								setTextColor(BRIGHT_GREEN);
							}else{
								setTextColor(0,255,0);
							}
						break;
						default:
							if(!readOnly && entryCharIndex==charLength) printf(ctrl_color,ctrl_garbage);
						break;
					}
				break;
				default:
					std::cout << ctrl_unknown;
				break;
			}

			i+=escLength-1;
			continue;
		}
		if(msg[i]=='\x0a'){
			line++;
			moveCursor(1,line);
			continue;
		}
		std::cout << msg[i];
	}

	if(useOption){
		line+=2;
		moveCursor(1,line);
		std::cout << '>' << yesOption;
		moveCursor(1,line+1);
		std::cout << ' ' << noOption;
	}

	setTextColor(WHITE);
	setBackgroundColor(BLACK);
}

unsigned int bmgGetRow(std::string msg,unsigned int index){
	unsigned int row = 0;
	unsigned int charLength = 0;

	for(unsigned int i=0;i<msg.length();i++){
		charLength++;
		if(msg[i]=='\x1a') i+=msg[i+1]-1;
		if(msg[i]=='\x0a') row++;
		if(charLength>=index) break;
	}

	return row;
}

unsigned int bmgGetColumn(std::string msg,unsigned int index){
	unsigned int col = 0;
	unsigned int charLength = 0;

	if(index==0) return 0;

	for(unsigned int i=0;i<msg.length();i++){
		cursorOnCtrl=false;
		charLength++;
		col++;
		if(msg[i]=='\x1a'){
			i+=msg[i+1]-1;
			cursorOnCtrl=true;
		}
		if(msg[i]=='\x0a') col=0;
		if(charLength>=index) break;
	}

	return col;
}