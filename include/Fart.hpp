#ifndef FART_HPP
#define FART_HPP

#include <map>
#include <fstream>
#include <string>
#include <vector>

namespace WTFarc {

	enum FartType: char {

		FAR2='2',
		FAR3='3',
		FAR4='4',
		FAR5='5',
		FARC='C'

	};

	struct FartEntry {

			unsigned int offset;
			unsigned int length;

			bool loadFromBuffer;
			char* buffer;
			std::fstream* stream;

	};

	struct FartSaveKey {

			unsigned char revisionSize;
			char* revision;

			int userId;
			int copied;
			int saveType;
			char hash[20];

	};

	//Don't look at me, I didn't name it
	class Fart {

		public:

			Fart();
			Fart(char* buffer,unsigned int length);
			Fart(std::fstream* stream);

			bool isLoaded(){return _loaded;}

			void load(char* buffer,unsigned int length);
			void load(std::fstream* stream);

			char* writeToBuffer(unsigned int& length);
			void writeToFile(std::fstream* stream);

			FartType getType(){return _fartType;}

			bool isLittleEndian(){return _littleEndian;}
			void setLittleEndian(bool littleEndian){_littleEndian=littleEndian;}

			unsigned int getNumEntries(){return _numEntries;}
			std::string getHash(unsigned int index){return _hashIndices[index];}
			unsigned int getEntryLength(std::string hash);
			char* getEntryData(std::string hash);
			std::string addEntry(char* buffer,unsigned int length);
			std::string addEntry(std::fstream* stream,unsigned int length);
			void removeEntry(std::string hash);

		private:

			bool _loaded;
			FartType _fartType;

			bool _littleEndian;
			unsigned int _numFragments;
			int* _fragments;
			char _hash[20];
			FartSaveKey _saveKey;

			char* _buffer;
			std::fstream* _stream;

			unsigned int _numEntries;
			std::vector<std::string> _hashIndices;
			std::map<std::string,FartEntry> _entries;

			unsigned int _getHeaderSize();
			void _parseHeader(char header[12]);
			char* _createHeader();
			unsigned int _getFatSize();
			void _parseFat(char* fat,bool loadFromBuffer);
			char* _createFat();
			unsigned int _getSaveKeySize();
			void _parseSaveKey(char* saveKey);
			char* _createSaveKey();

	};

};

#endif