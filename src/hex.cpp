#include <iomanip>
#include <ios>
#include <sstream>
#include <hex.hpp>

std::string WTFarc::bufferToHexString(char* buffer,unsigned int length){
	std::ostringstream hexStream;
	hexStream << std::hex << std::setfill('0'); 
	for (unsigned int i = 0;i<length;i++){
		unsigned char cleanByte = static_cast<unsigned char>(buffer[i]);
		hexStream << std::setw(2) << static_cast<int>(cleanByte);
	}
	return hexStream.str();
}

char* WTFarc::hexStringToBuffer(std::string hex,unsigned int& length){

	length=hex.length()/2;
	char* buffer = new char[length];

	for(unsigned int i=0;i<length;i++){

		unsigned char lowNybble;
		if (hex[i*2] >= '0' && hex[i*2] <= '9') lowNybble=hex[i*2]-'0'; else if(hex[i*2]>='a' && hex[i*2]<='f') lowNybble=hex[i*2]-'a'+10;
		unsigned char highNybble;
		if (hex[i*2+1] >= '0' && hex[i*2+1] <= '9') highNybble=hex[i*2+1]-'0'; else if(hex[i*2+1]>='a' && hex[i*2+1]<='f') highNybble=hex[i*2+1]-'a'+10;

		buffer[i] = (lowNybble << 4)|highNybble;

	}

	return buffer;
}