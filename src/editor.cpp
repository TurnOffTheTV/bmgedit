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

const std::string errorMessage = "!!!ERROR!!!\nMessage could not\nbe loaded.";

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
		std::cout << " | Press 'q' to quit";
		moveCursor(1,2);
		for(unsigned int i=0;i<terminalWidth;i++) std::cout << '=';

		//DEBUG: print entries
		moveCursor(1,3);
		std::cout << "Entry " << entryIndex << ": ";
		bmgPrintMessage(entries[entryIndex].message,1,4);

		//Handle key presses
		switch(getchar()){
			case 'q':
				runProgram=false;
			break;
			case '\x1b':
				if(getchar()=='\x5b'){
					switch(getchar()){
						case '\x41':
							if(entryIndex>0) entryIndex--;
						break;
						case '\x42':
							if(entryIndex<numEntries-1) entryIndex++;
						break;
					}
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

void bmgPrintMessage(std::string msg,unsigned int x,unsigned int y){
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


	for(unsigned int i=0;i<msg.length();i++){
		if(msg[i]=='\x1a'){
			unsigned char escLength = msg[i+1];

			switch(msg[i+2]){
				case '\x00':
					//Message controls?
					if(msg[i+4]=='\x01') std::cout << "{auto-close message}";
				break;
				case '\x01':
					//Select settings
					useOption=true;
					if(msg[i+4]=='\x00') yesOption=msg.substr(i+5,escLength-5);
					if(msg[i+4]=='\x01') noOption=msg.substr(i+5,escLength-5);
				break;
				case '\x02':
					//Save record
					if(msg[i+4]=='\x02') std::cout << "{box game record time}";
					if(msg[i+4]=='\x03') std::cout << "{# of shines for blue coins}";
					if(msg[i+4]=='\x04'){
						if(msg[i+5]=='\x00') std::cout << "{# of bananas}";
						if(msg[i+5]=='\x01') std::cout << "{# of coconuts}";
						if(msg[i+5]=='\x02') std::cout << "{# of pineapples}";
						if(msg[i+5]=='\x03') std::cout << "{# of durians}";
					}
				break;
				case '\xff':
					//Change text color
					switch(msg[i+5]){
						case '\x00':
							if(useColorCodes){
								setTextColor(BRIGHT_WHITE);
							}else{
								setTextColor(255,255,255);
							}
						break;
						case '\x01':
							if(useColorCodes){
								setTextColor(WHITE);
							}else{
								setTextColor(200,200,200);
							}
						break;
						case '\x02':
							if(useColorCodes){
								setTextColor(BRIGHT_RED);
							}else{
								setTextColor(255,0,0);
							}
						break;
						case '\x03':
							if(useColorCodes){
								setTextColor(BRIGHT_BLUE);
							}else{
								setTextColor(100,100,255);
							}
						break;
						case '\x04':
							if(useColorCodes){
								setTextColor(BRIGHT_YELLOW);
							}else{
								setTextColor(255,255,0);
							}
						break;
						case '\x05':
							if(useColorCodes){
								setTextColor(BRIGHT_GREEN);
							}else{
								setTextColor(0,255,0);
							}
						break;
					}
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