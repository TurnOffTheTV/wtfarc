#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>

namespace WTFarc {

	std::string bufferToHexString(char* buffer,unsigned int length);

	char* hexStringToBuffer(std::string hex,unsigned int& length);

};

#endif