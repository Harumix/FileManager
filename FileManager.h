#pragma once
#include <string>
#include <vector>
#include <array>
#include <sstream>  

class FileManager {
	public:
	using u_int = unsigned int;
	using u_short_int = unsigned short int;

	static const uint8_t BLOCK_SIZE = 16;
	static const uint8_t FILE_LIMIT = 16;
	static const u_short_int DISK_CAPACITY = 1024;

	struct File {
		public:
		std::string name;
		int indexBlockNumber;
		int size = 0;
		int readPointer;
		int writePointer;

		File() {
			this->readPointer = 0;
			this->writePointer = 0;
		}

		File(std::string _name) {
			this->name = _name;
			this->readPointer = 0;
			this->writePointer = 0;
		}
	};

	std::array<char, DISK_CAPACITY> disk;
	std::vector<File> mainCatalog;
	std::array<int, DISK_CAPACITY / BLOCK_SIZE> openFiles;
	std::array<bool, DISK_CAPACITY / BLOCK_SIZE> freeIndexes;

	
	
	int searchFreeBlock();

	int searchFileId(std::string name);

	int indexBlockFillZero(int ind);

	int searchIndexBlock(int i);

	bool isNameUsed(std::string name);

	int createFile(std::string name);

	int deleteFile(std::string name);

	int openFile(std::string name);

	int closeFile(std::string name);

	int writeToFile(std::string name, std::string data);

	int readFileByte(std::string name, int howMuch);

	int sendFileByte(std::string name, int howMuch);

	std::string readFileAll(std::string name);

	int renameFile(std::string name, std::string newName);
	
	int longFile(std::string name);

	std::stringstream displayFileSystemParams();

	std::stringstream displayFileInfo(const std::string& name);

	std::stringstream displayDiskContentChar();
	
	FileManager() = default;
};
