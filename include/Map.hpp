#ifndef MAP_HPP
#define MAP_HPP

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace WTFarc {

	struct MapEntry {

		std::filesystem::path path;
		unsigned long timestamp;
		unsigned int size;
		std::string hash;
		unsigned int guid;

	};

	class Map {

		public:

			Map();
			Map(char* buffer,unsigned int length);
			Map(std::fstream* stream);

			bool isLoaded(){return _loaded;}

			void load(char* buffer,unsigned int length);
			void load(std::fstream* stream);

			char* writeToBuffer(unsigned int& length);
			void writeToFile(std::fstream* stream);

			unsigned int getNumEntries(){return _entries.size();}
			MapEntry getEntry(unsigned int index);
			void addEntry(MapEntry entry){_entries.push_back(entry);_loaded=true;}
			void removeEntry(unsigned int index);

			unsigned int getEntryIndexByGuid(unsigned int guid);
			unsigned int getEntryIndexByHash(std::string hash);

		private:

			bool _loaded;
			unsigned int _revision;

			std::vector<MapEntry> _entries;
			std::map<unsigned int,unsigned int> _guidIndices;
			std::map<std::string,unsigned int> _hashIndices;

	};

}

#endif