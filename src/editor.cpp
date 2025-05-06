#include <iostream>
#include <fstream>
#include <cstring>
#include <editor.hpp>
#include <terminal.hpp>
#include <isLittleEndian.hpp>
#include <BmgEntry.hpp>

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

int runEditor(std::filesystem::path filename){
	//Error if file doesn't exist
	if(!newFile && !std::filesystem::exists(filename)){
		std::cout << filename << " does not exist." << std::endl;
		return 1;
	}

	//Open stream based on flags
	std::fstream bmgFile;
	if(newFile){
		if(readOnly){
			bmgFile.open(filename,std::fstream::trunc | std::fstream::in);
		}else{
			bmgFile.open(filename,std::fstream::trunc | std::fstream::in | std::fstream::out);
		}
	}else{
		if(readOnly){
			bmgFile.open(filename,std::fstream::in);
		}else{
			bmgFile.open(filename,std::fstream::in | std::fstream::out);
		}
	}

	//Check for errors opening file
	if(!bmgFile.is_open()){
		std::cout << "Failed to open file at " << filename << '.' << std::endl;
		return 1;
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
	BmgEntry entries[numEntries];

	for(unsigned int i=0;i<numEntries;i++){
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
		entries[i].message = std::string(messagesBuffer+msgOffset,bmgMessageLength(messagesBuffer+msgOffset));

		entries[i].charLength=0;

		for(unsigned int j=0;j<entries[i].message.length();j++){
			entries[i].charLength++;
			if(entries[i].message[j]=='\x1a'){
				unsigned char escLength = entries[i].message[j+1];
				j+=escLength-1;
				continue;
			}
		}
		
		if(entryLength==4) continue;

		//Start frame
		unsigned short startFrame;
		bmgFile.read(reinterpret_cast<char *>(&startFrame),2);
		if(isLittleEndian()){
			unsigned short temp = startFrame;
			startFrame=std::byteswap(temp);
		}
		entries[i].startFrame=startFrame;

		//End frame
		unsigned short endFrame;
		bmgFile.read(reinterpret_cast<char *>(&endFrame),2);
		if(isLittleEndian()){
			unsigned short temp = endFrame;
			endFrame=std::byteswap(temp);
		}
		entries[i].endFrame=endFrame;

		//Sound ID
		bmgFile.read(reinterpret_cast<char *>(&entries[i].soundID),1);

		//Skip to next entry
		bmgFile.seekp(entryStart+entryLength);
	}

	//Print UI to console
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
		if(screen==EDIT_ENTRY && !readOnly) std::cout << " | Press ^m for menu";
		if(screen==ENTRY_MENU || screen==PICK_CONTROL || screen==CONTROL_DETAIL || screen==EDIT_ENTRY && readOnly) std::cout << " | Press 'q' to return to entry list";
		moveCursor(1,2);
		for(unsigned int i=0;i<terminalWidth;i++) std::cout << '=';

		//Draw screens
		if(screen!=ENTRY_MENU) menuIndex=0;
		if(screen!=PICK_CONTROL) controlIndex=0;
		switch(screen){
			case PICK_ENTRY:
				for(unsigned int i=entryPickPage*(terminalHeight-2);i<(entryPickPage+1)*(terminalHeight-2);i++){
					if(i>=numEntries) break;
					moveCursor(1,i-entryPickPage*(terminalHeight-2)+3);
					std::cout << "  Entry " << i << ": ";
					bmgPrintLine(entries[i].message);
					std::cout << "...";
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
									if(entryIndex<numEntries-1){
										entryIndex++;
										if(entryIndex-entryPickPage*(terminalHeight-2)>terminalHeight-3) entryPickPage++;
									}
								break;
							}
						}
					break;
				}
			break;
			case EDIT_ENTRY:
				bmgPrintMessage(entries[entryIndex].message,1,3,entryCharIndex);

				if(!readOnly){
					showCursor();
					moveCursor(bmgGetColumn(entries[entryIndex].message,entryCharIndex)+1,bmgGetRow(entries[entryIndex].message,entryCharIndex)+3);
				}

				switch(getchar()){
					case 13:
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
			break;
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
	bmgFile.close();
	system("stty cooked");
	showCursor();
	setTextColor(DEFAULT);
	setBackgroundColor(DEFAULT);
	clearTerminalScreen();

	//Return OK
	return 0;
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
		charLength++;
		col++;
		if(msg[i]=='\x1a') i+=msg[i+1]-1;
		if(msg[i]=='\x0a') col=0;
		if(charLength>=index) break;
	}

	return col;
}
