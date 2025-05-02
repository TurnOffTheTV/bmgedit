#include <iostream>
#include <cstring>

#include <terminal.hpp>
#include <editor.hpp>

//Version string
const std::string version = "1.0.0";

int main(int argc, char** argv){
	updateTerminalSize();

	if(argc==1) printHelp();

	if(argc==2){
		if(!strcmp(argv[1],"-h")){
			printHelp();
			return 0;
		}
		if(!strcmp(argv[1],"-v")){
			std::cout << version << std::endl;
			return 0;
		}
		return runEditor(argv[1]);
	}

	if(argc>2){
		for(unsigned char i=1;i<argc-1;i++){
			if(strcmp(argv[i],"-h")==0){
				printHelp();
				return 0;
			}
			if(strcmp(argv[i],"-v")==0){
				std::cout << version << std::endl;
				return 0;
			}
			if(strcmp(argv[i],"-r")==0){
				readOnly=true;

				continue;
			}
			if(strcmp(argv[i],"-c")==0){
				newFile=true;

				continue;
			}
			if(strcmp(argv[i],"-a")==0){
				useColorCodes=true;

				continue;
			}
			if(strcmp(argv[i],"-d")==0){
				developerMode=true;

				continue;
			}
			std::cout << "Unknown argument: " << argv[i] << '.' << std::endl;
			return 1;
		}

		return runEditor(argv[argc-1]);
	}

	return 0;
}