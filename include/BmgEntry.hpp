#ifndef BMGENTRY_HPP
#define BMGENTRY_HPP

#include <string>

enum SoundId {

};

struct BmgEntry {
	std::string message;
	unsigned short startFrame;
	unsigned short endFrame;
	enum SoundId soundID;
	unsigned int charLength;
};

#endif