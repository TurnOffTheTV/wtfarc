#include <cstring>
#include <openssl/sha.h>
#include <endianness.hpp>
#include <Fart.hpp>
#include <hex.hpp>
#include <wtfarc_error.hpp>

using namespace WTFarc;

Fart::Fart(){
	_loaded=false;
	_littleEndian=false;
}

Fart::Fart(char* buffer,unsigned int length):Fart(){
	load(buffer,length);
}

Fart::Fart(std::fstream* stream):Fart(){
	load(stream);
}

void Fart::load(char* buffer,unsigned int length){

	//Check if already loaded
	if(_loaded) throw wtfarc_error("FART already loaded");

	_buffer=buffer;

	_parseHeader(buffer+(length-12));
	_parseFat(buffer+(length-(_getHeaderSize()+_getFatSize())),true);
	_parseSaveKey(buffer+(length-(_getHeaderSize()+_getFatSize()+_getSaveKeySize())));

	//Finish
	_loaded=true;

}

void Fart::load(std::fstream* stream){

	//Check if already loaded
	if(_loaded) throw wtfarc_error("FART already loaded");

	_stream=stream;

	char* header = new char[12];
	stream->seekg(-12,std::ios::end);
	stream->read(header,12);
	_parseHeader(header);
	delete header;

	char* fat = new char[_getFatSize()];
	stream->seekg(-(int)(_getHeaderSize()+_getFatSize()),std::ios::end);
	stream->read(fat,_getFatSize());
	_parseFat(fat,false);
	delete fat;

	char* saveKey = new char[_getSaveKeySize()];
	stream->seekg(-(int)(_getHeaderSize()+_getFatSize()+_getSaveKeySize()),std::ios::end);
	stream->read(saveKey,_getSaveKeySize());
	_parseSaveKey(saveKey);
	delete saveKey;

	//Finish
	_loaded=true;

}

char* Fart::writeToBuffer(unsigned int& length){

	//Calculate length
	length=0;
	for(unsigned int i=0;i<_numEntries;i++) length+=getEntryLength(_hashIndices[i]);
	length+=_getSaveKeySize()+_getFatSize()+_getHeaderSize();
	while(length%4) length++;

	//Set buffer
	char* buffer = new char[length];
	memset(buffer,0,length);
	unsigned int offset = 0;

	//Write entries
	for(unsigned int i=0;i<_numEntries;i++){
		char* entryData = getEntryData(_hashIndices[i]);
		memcpy(buffer+offset,entryData,getEntryLength(_hashIndices[i]));
		delete entryData;
		offset+=getEntryLength(_hashIndices[i]);
	}

	while(offset%4) offset++;
	
	if(_getSaveKeySize()){
		char* saveKey = _createSaveKey();
		memcpy(buffer+offset,saveKey,_getSaveKeySize());
		offset+=_getSaveKeySize();
		delete saveKey;
	}

	char* fat = _createFat();
	memcpy(buffer+offset,fat,_getFatSize());
	offset+=_getFatSize();
	delete fat;

	char* header = _createHeader();
	memcpy(buffer+offset,header,_getHeaderSize());
	offset+=_getHeaderSize();
	delete header;

	return buffer;

}

void Fart::writeToFile(std::fstream* stream){

	//Write files
	for(unsigned int i=0;i<_numEntries;i++){
		char* entry = getEntryData(_hashIndices[i]);
		stream->write(entry,getEntryLength(_hashIndices[i]));
		delete entry;
	}

	while(stream->tellp()%4) stream->write("\x00",1);

	//Write save key
	if(_getSaveKeySize()){
		char* saveKey = _createSaveKey();
		stream->write(saveKey,_getSaveKeySize());
		delete saveKey;
	}

	//Write fat
	char* fat = _createFat();
	stream->write(fat,_getFatSize());
	delete fat;

	//Write header
	char* header = _createHeader();
	stream->write(header,_getHeaderSize());
	delete header;

}

unsigned int Fart::getEntryLength(std::string hash){
	if(!_entries.contains(hash)) throw wtfarc_error("Hash not found in FART");
	return _entries[hash].length;
}

char* Fart::getEntryData(std::string hash){

	if(!_entries.contains(hash)) throw wtfarc_error("Hash not found in FART");

	//Allocate entry data buffer
	char* buffer = new char[_entries[hash].length];

	//Get data from FART source
	if(_entries[hash].loadFromBuffer){
		memcpy(buffer,_entries[hash].buffer+_entries[hash].offset,_entries[hash].length);
	}else{
		_entries[hash].stream->clear();
		_entries[hash].stream->seekg(_entries[hash].offset,std::ios::beg);
		_entries[hash].stream->read(buffer,_entries[hash].length);
	}

	return buffer;

}

std::string Fart::addEntry(char* buffer,unsigned int length){

	//Get hash of data
	unsigned char hashBuffer[20];
	SHA1(reinterpret_cast<const unsigned char*>(buffer),length,hashBuffer);
	std::string hash = bufferToHexString((char*)hashBuffer,20);

	//Create Fart entry
	FartEntry newEntry;
	newEntry.loadFromBuffer=true;
	newEntry.buffer=buffer;
	newEntry.offset=0;
	newEntry.length=length;

	//Set in hash indices and hash map
	_numEntries++;
	_hashIndices.push_back(hash);
	_entries.insert({hash,newEntry});

	_loaded=true;
	return hash;

}

std::string Fart::addEntry(std::fstream* stream,unsigned int length){

	//Create Fart entry
	FartEntry newEntry;
	newEntry.loadFromBuffer=false;
	newEntry.stream=stream;
	newEntry.offset=stream->tellg();
	newEntry.length=length;

	//Create buffer
	char* buffer = new char[length];
	stream->read(buffer,length);

	//Get hash of data
	unsigned char hashBuffer[20];
	SHA1(reinterpret_cast<const unsigned char*>(buffer),length,hashBuffer);
	std::string hash = bufferToHexString((char*)hashBuffer,20);

	//Set in hash indices and hash map
	_numEntries++;
	_hashIndices.push_back(hash);
	_entries.insert({hash,newEntry});

	_loaded=true;
	delete buffer;
	return hash;
	
}

void Fart::removeEntry(std::string hash){

	if(!_entries.contains(hash)) throw wtfarc_error("Hash not found in FART");

	_numEntries--;

	//Remove hash from indices
	for(unsigned int i=0;i<_hashIndices.size();i++){
		if(_hashIndices[i]==hash){
			_hashIndices.erase(_hashIndices.begin()+i);
			break;
		}
	}

	//Remove from hash map
	_entries.erase(hash);

}

unsigned int Fart::_getHeaderSize(){

	if(_fartType==FAR5) return 12;
	return 8;

}

void Fart::_parseHeader(char header[12]){

	//Read magic
	if(strncmp(header+8,"FAR",3)) throw wtfarc_error("Magic mismatch");
	_fartType=(FartType)header[11];

	//FAR5 is only for LBP Vita, always little-endian and has fragments
	if(_fartType==FAR5){
		_littleEndian=true;
		memcpy(&_numFragments,header,4);
		_numFragments=littleToSystem(_numFragments);
	}

	//Read number of entries
	memcpy(&_numEntries,header+4,4);
	_numEntries=bigToSystem(_numEntries);

	//Detect PS4 endianness by checking if first byte has content (indicates flipped byte order)
	if(!_littleEndian && _numEntries>0xFFFFFF) _littleEndian=true;
	if(_littleEndian) _numEntries=bigToSystem(_numEntries);

	//Check FART types
	//I've personally only seen types C and 4, but evidence suggests that types 2,3, and 5 exist as well
	if(_fartType!=FARC && _fartType!=FAR2 && _fartType!=FAR3 && _fartType!=FAR4 && _fartType!=FAR5) throw wtfarc_error("Invalid FART type");

}

char* Fart::_createHeader(){

	char* header;
	unsigned int offset=0;

	//Handle FAR5 fragment number
	if(_fartType==FAR5){
		header=new char[12];

		unsigned int fragments = littleToSystem(_numFragments);
		memcpy(header,&fragments,4);

		offset=4;
	}else{
		header=new char[8];
	}

	//Write entry count
	unsigned int numEntries;
	if(_littleEndian) numEntries = littleToSystem(_numEntries); else numEntries = bigToSystem(_numEntries);
	memcpy(header+offset,&numEntries,4);

	//Write magic
	memcpy(header+offset+4,"FAR",3);
	header[offset+7]=_fartType;

	return header;

}

unsigned int Fart::_getFatSize(){

	unsigned int fatSize = _numEntries*28;
	if(_fartType==FAR3 || _fartType==FAR4 || _fartType==FAR5) fatSize+=20;//Including file hash in FAT even though it's not really

	return fatSize;
}

void Fart::_parseFat(char* fat,bool loadFromBuffer){

	for(unsigned int i=0;i<_numEntries;i++){

		//Allocate variables
		FartEntry newEntry;
		char hashBuffer[20];

		//Set up entry loading
		newEntry.loadFromBuffer=loadFromBuffer;
		if(loadFromBuffer){
			newEntry.buffer=_buffer;
		}else{
			newEntry.stream=_stream;
		}

		//Copy hash
		memcpy(hashBuffer,fat+i*28,20);
		std::string hash = bufferToHexString(hashBuffer,20);

		//Copy data offset
		memcpy(&newEntry.offset,fat+i*28+20,4);
		if(_littleEndian){newEntry.offset=littleToSystem(newEntry.offset);}else{newEntry.offset=bigToSystem(newEntry.offset);}

		//Copy data length
		memcpy(&newEntry.length,fat+i*28+24,4);
		if(_littleEndian){newEntry.length=littleToSystem(newEntry.length);}else{newEntry.length=bigToSystem(newEntry.length);}

		//Set in hash indices and hash map
		_hashIndices.push_back(hash);
		_entries.insert({hash,newEntry});
		
	}

	//Read file hash if it exists (FAR3+)
	if(_fartType==FAR3 || _fartType==FAR4 || _fartType==FAR5) memcpy(_hash,fat+_numEntries*28,20);
	
}

char* Fart::_createFat(){

	char* fat = new char[_getFatSize()];
	unsigned int fileOffset = 0;
	unsigned int fatOffset = 0;

	for(unsigned int i=0;i<_numEntries;i++){

		std::string hash = _hashIndices[i];

		//Write hash
		unsigned int length;
		char* hashBytes = hexStringToBuffer(hash,length);
		memcpy(fat+fatOffset,hashBytes,20);
		delete hashBytes;

		//Write offset
		unsigned int writtenOffset;
		if(_littleEndian) writtenOffset = littleToSystem(fileOffset); else writtenOffset = bigToSystem(fileOffset);
		memcpy(fat+fatOffset+20,&writtenOffset,4);
		fileOffset+=_entries[hash].length;

		//Write length
		unsigned int writtenLength;
		if(_littleEndian) writtenLength = littleToSystem(_entries[hash].length); else writtenLength = bigToSystem(_entries[hash].length);
		memcpy(fat+fatOffset+24,&writtenLength,4);

		fatOffset+=28;

	}

	//Write file hash if FAR3+
	if(_fartType==FAR3 || _fartType==FAR4 || _fartType==FAR5) memcpy(fat+fatOffset,_hash,20);

	return fat;

}

unsigned int Fart::_getSaveKeySize(){

	//FARC is not a save, has no save key
	if(_fartType==FARC) return 0;

	unsigned int saveKeySize = 128;

	if(_fartType==FAR4) saveKeySize+=4;
	if(_fartType==FAR5) saveKeySize+=8+_numFragments*4;

	return saveKeySize;

}

void Fart::_parseSaveKey(char* saveKey){

	//FARC is not a save, has no save key
	if(_fartType==FARC) return;

	unsigned int offset;

	//Read game revision or fragments
	if(_fartType==FAR5){

		//Read revision
		_saveKey.revisionSize=8;
		_saveKey.revision = new char[8];
		memcpy(_saveKey.revision,saveKey,8);
		offset=8;

		//Read fragments
		_fragments = new int[_numFragments];
		for(unsigned int i=0;i<_numFragments;i++){
			memcpy(&_fragments[i],saveKey+offset,4);
			offset+=4;
		}

	}else if(_fartType==FAR4){

		//Read revision
		_saveKey.revisionSize=8;
		_saveKey.revision = new char[8];
		memcpy(_saveKey.revision,saveKey,8);
		offset=8;

	}else{

		//Read revision
		_saveKey.revisionSize=4;
		_saveKey.revision = new char[4];
		memcpy(_saveKey.revision,saveKey,4);
		offset=4;

	}

	//Read user id
	memcpy(&_saveKey.userId,saveKey+offset,4);

	//PS3/Vita games use user index integer; PS4 uses user hash so don't swap that
	if(_fartType==FAR5){
		_saveKey.userId=littleToSystem(_saveKey.userId);
	}else if(!_littleEndian){
		_saveKey.userId=bigToSystem(_saveKey.userId);
	}

	//User ID + 40 unused bytes
	offset+=44;

	//Read "copied"
	//No idea what this does (cwlib named it) and LBP3 seems to set something else here
	memcpy(&_saveKey.copied,saveKey+offset,4);

	//Read save type
	memcpy(&_saveKey.saveType,saveKey+offset+4,4);

	//copied + save type + 12 unused bytes
	offset+=20;

	//Read save hash
	memcpy(&_saveKey.hash,saveKey+offset,20);

}

char* Fart::_createSaveKey(){

	char* saveKey = new char[_getSaveKeySize()];
	memset(saveKey,0,_getSaveKeySize());
	unsigned int offset = 0;

	memcpy(saveKey,_saveKey.revision,_saveKey.revisionSize);
	offset+=_saveKey.revisionSize;

	int userId = _saveKey.userId;
	if(_fartType==FAR5){

		//Write FAR5 fragments
		unsigned int fragments = littleToSystem(_numFragments);
		memcpy(saveKey+offset,&fragments,4);
		memcpy(saveKey+offset+4,&_fragments,_numFragments*4);
		offset+=_numFragments*4+4;

		userId=littleToSystem(userId);

	}else if(!_littleEndian){
		userId=bigToSystem(userId);
	}

	//Write user id
	memcpy(saveKey+offset,&userId,4);
	offset+=44;

	//Write copied and save type
	memcpy(saveKey+offset,&_saveKey.copied,4);
	memcpy(saveKey+offset+4,&_saveKey.saveType,4);
	offset+=20;

	//Write save hash
	memcpy(saveKey+offset,&_saveKey.hash,20);

	return saveKey;
}