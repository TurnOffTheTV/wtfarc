#include <cstring>
#include <endianness.hpp>
#include <hex.hpp>
#include <iostream>
#include <Map.hpp>
#include <wtfarc_error.hpp>

using namespace WTFarc;

Map::Map(){
	_loaded=false;
}

Map::Map(char* buffer,unsigned int length):Map(){
	load(buffer,length);
}

Map::Map(std::fstream* stream):Map(){
	load(stream);
}

void Map::load(char* buffer,unsigned int length){

	//Check if already loaded
	if(_loaded) throw wtfarc_error("MAP already loaded");

	//Read revision
	memcpy(&_revision,buffer,4);
	_revision=bigToSystem(_revision);
	bool isLBP3 = (_revision>>16)>327;

	//Read number of entries
	unsigned int numEntries;
	memcpy(&numEntries,buffer+4,4);
	numEntries=bigToSystem(numEntries);

	unsigned int offset = 8;

	for(unsigned int i=0;i<numEntries;i++){

		MapEntry newEntry;

		//Read path
		unsigned int pathLength = 0;
		if(isLBP3){
			memcpy(&pathLength,buffer+offset,2);
			pathLength<<=16;
			offset+=2;
		}else{
			memcpy(&pathLength,buffer+offset,4);
			offset+=4;
		}
		pathLength=bigToSystem(pathLength);
		newEntry.path=std::string(buffer+offset,pathLength);
		offset+=pathLength;

		//Read timestamp
		newEntry.timestamp=0;
		if(isLBP3){
			memcpy(&newEntry.timestamp,buffer+offset,4);
			newEntry.timestamp<<=32;
			offset+=4;
		}else{
			memcpy(&newEntry.timestamp,buffer+offset,8);
			offset+=8;
		}
		newEntry.timestamp=bigToSystem(newEntry.timestamp);

		//Read size
		memcpy(&newEntry.size,buffer+offset,4);
		newEntry.size=bigToSystem(newEntry.size);

		//Read hash
		char hash[20];
		memcpy(hash,buffer+offset+4,20);
		newEntry.hash=bufferToHexString(hash,20);

		//Read GUID
		memcpy(&newEntry.guid,buffer+offset+24,4);
		newEntry.guid=bigToSystem(newEntry.guid);

		//Set in all the places
		_guidIndices[newEntry.guid]=_entries.size();
		_hashIndices[newEntry.hash]=_entries.size();
		_entries.push_back(newEntry);
		offset+=28;

	}

	_loaded=true;

}

void Map::load(std::fstream* stream){

	//Check if already loaded
	if(_loaded) throw wtfarc_error("MAP already loaded");

	//Read revision
	stream->read((char*)&_revision,4);
	_revision=bigToSystem(_revision);
	bool isLBP3 = (_revision>>16)>327;

	//Read number of entries
	unsigned int numEntries;
	stream->read((char*)&numEntries,4);
	numEntries=bigToSystem(numEntries);

	for(unsigned int i=0;i<numEntries;i++){

		MapEntry newEntry;

		//Read path
		unsigned int pathLength = 0;
		if(isLBP3){
			stream->read((char*)&pathLength,2);
			pathLength<<=16;
		}else{
			stream->read((char*)&pathLength,4);
		}
		pathLength=bigToSystem(pathLength);

		char pathBuffer[pathLength];
		stream->read(pathBuffer,pathLength);
		newEntry.path=std::string(pathBuffer,pathLength);

		//Read timestamp
		newEntry.timestamp=0;
		if(isLBP3){
			stream->read((char*)&newEntry.timestamp,4);
			newEntry.timestamp<<=32;
		}else{
			stream->read((char*)&newEntry.timestamp,8);
		}
		newEntry.timestamp=bigToSystem(newEntry.timestamp);

		//Read size
		stream->read((char*)&newEntry.size,4);
		newEntry.size=bigToSystem(newEntry.size);

		//Read hash
		char hash[20];
		stream->read(hash,20);
		newEntry.hash=bufferToHexString(hash,20);

		//Read GUID
		stream->read((char*)&newEntry.guid,4);
		newEntry.guid=bigToSystem(newEntry.guid);

		//Set in all the places
		_guidIndices[newEntry.guid]=_entries.size();
		_hashIndices[newEntry.hash]=_entries.size();
		_entries.push_back(newEntry);

	}

	_loaded=true;

}

char* Map::writeToBuffer(unsigned int& length){

	bool isLBP3 = (_revision>>16)>327;

	//Calculate length
	length=8;
	for(unsigned int i=0;i<_entries.size();i++){
		length+=_entries[i].path.string().length();
		if(isLBP3) length+=34; else length+=40;
	}

	//Allocate buffer
	unsigned int offset = 8;
	char* buffer = new char[length];

	//Write revision
	unsigned int revision = bigToSystem(_revision);
	memcpy(buffer,&revision,4);

	//Write entry count
	unsigned int numEntries = _entries.size();
	numEntries = bigToSystem(numEntries);
	memcpy(buffer+4,&numEntries,4);

	for(unsigned int i=0;i<_entries.size();i++){

		//Write path
		unsigned int pathLength = _entries[i].path.string().length();
		pathLength = bigToSystem(pathLength);
		if(isLBP3){
			pathLength>>=16;
			memcpy(buffer+offset,&pathLength,2);
			offset+=2;
		}else{
			memcpy(buffer+offset,&pathLength,4);
			offset+=4;
		}
		memcpy(buffer+offset,_entries[i].path.c_str(),_entries[i].path.string().length());
		offset+=_entries[i].path.string().length();

		//Write timestamp
		unsigned long timestamp = bigToSystem(_entries[i].timestamp);
		if(isLBP3){
			timestamp>>=32;
			memcpy(buffer+offset,&timestamp,4);
			offset+=4;
		}else{
			memcpy(buffer+offset,&timestamp,8);
			offset+=8;
		}

		//Write size
		unsigned int size = bigToSystem(_entries[i].size);
		memcpy(buffer+offset,&size,4);

		//Write hash
		unsigned int hashLength;
		char* hashBuffer = hexStringToBuffer(_entries[i].hash,hashLength);
		memcpy(buffer+offset+4,hashBuffer,20);

		//Write GUID
		unsigned int guid = bigToSystem(_entries[i].guid);
		memcpy(buffer+offset+24,&guid,4);

		offset+=28;

	}

	return buffer;

}

void Map::writeToFile(std::fstream* stream){

	bool isLBP3 = (_revision>>16)>327;

	//Write revision
	unsigned int revision = bigToSystem(_revision);
	stream->write((char*)&revision,4);

	//Write entry count
	unsigned int numEntries = _entries.size();
	numEntries = bigToSystem(numEntries);
	stream->write((char*)&numEntries,4);

	for(unsigned int i=0;i<_entries.size();i++){

		//Write path
		unsigned int pathLength = _entries[i].path.string().length();
		pathLength = bigToSystem(pathLength);
		if(isLBP3){
			pathLength>>=16;
			stream->write((char*)&pathLength,2);
		}else{
			stream->write((char*)&pathLength,4);
		}
		stream->write(_entries[i].path.c_str(),_entries[i].path.string().length());

		//Write timestamp
		unsigned long timestamp = bigToSystem(_entries[i].timestamp);
		if(isLBP3){
			timestamp>>=32;
			stream->write((char*)&timestamp,4);
		}else{
			stream->write((char*)&timestamp,8);
		}

		//Write size
		unsigned int size = bigToSystem(_entries[i].size);
		stream->write((char*)&size,4);

		//Write hash
		unsigned int hashLength;
		char* hashBuffer = hexStringToBuffer(_entries[i].hash,hashLength);
		stream->write((char*)hashBuffer,20);

		//Write GUID
		unsigned int guid = bigToSystem(_entries[i].guid);
		stream->write((char*)&guid,4);

	}

}

MapEntry Map::getEntry(unsigned int index){

	if(index>=_entries.size()) throw wtfarc_error("Map entry index out of range");

	return _entries[index];

}

void Map::removeEntry(unsigned int index){

	if(index>=_entries.size()) throw wtfarc_error("Map entry index out of range");

	_entries.erase(_entries.begin()+index);

}

unsigned int Map::getEntryIndexByGuid(unsigned int guid){

	if(!_guidIndices.contains(guid)) throw wtfarc_error("GUID not found");
	return _guidIndices[guid];

}

unsigned int Map::getEntryIndexByHash(std::string hash){

	if(!_hashIndices.contains(hash)) throw wtfarc_error("Hash not found");
	return _hashIndices[hash];

}