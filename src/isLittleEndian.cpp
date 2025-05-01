#include <isLittleEndian.hpp>

//https://stackoverflow.com/a/3877387/25918506
bool isLittleEndian(){
	short int number = 0x1;
	char *numPtr = (char*)&number;
	return (numPtr[0] == 1);
}