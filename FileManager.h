#pragma once
#include <string>
#include <vector>
#include <array>
#include <sstream>  

class FileManager {
	using u_int = unsigned int;
	using u_short_int = unsigned short int;

	static const uint8_t BLOCK_SIZE = 16;
	static const uint8_t FILE_LIMIT = 16;
	static const u_short_int DISK_CAPACITY = 1024;

	struct File {
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

	int SearchFreeBlock();

	int SearchFileId(std::string name);

	int IndexBlockFillZero(int ind);

	int SearchIndexBlock(int i);

	bool IsNameUsed(std::string name);

	int CreateFile(std::string name);

	int DeleteFile(std::string name);

	int OpenFile(std::string name);

	int CloseFile(std::string name);

	int WriteToFile(std::string name, std::string data);

	int ReadFileByte(std::string name, int howMuch);

	int SendFileByte(std::string name, int howMuch);

	std::string ReadFileAll(std::string name);

	int RenameFile(std::string name, std::string newName);

	std::stringstream display_file_system_params();

	std::stringstream display_file_info(const std::string& name);

	std::stringstream display_disk_content_char();
	
	FileManager() = default;
};
