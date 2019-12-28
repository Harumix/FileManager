#include "pch.h"
#include "FileManager.h"
#include <iomanip>
#include <iostream>

using namespace std;

FileManager fm;

using u_int = unsigned int;
using u_short_int = unsigned short int;
using u_char = unsigned char;

ostream& operator << (ostream& os, const vector<string>& strVec) {
	for (const string& str : strVec) {
		os << str << '\n';
	}
	return os;
}

ostream& operator << (ostream& os, const tm& time) {
	os << time.tm_hour << ':' << setfill('0') << setw(2) << time.tm_min
		<< ' ' << setfill('0') << setw(2) << time.tm_mday << '.'
		<< setfill('0') << setw(2) << time.tm_mon << '.' << time.tm_year;
	return os;
}

bool operator == (const tm& time1, const tm& time2) {
	return time1.tm_hour == time2.tm_hour && time1.tm_isdst == time2.tm_isdst && time1.tm_mday == time2.tm_mday && time1
		.tm_min == time2.tm_min && time1.tm_mon == time2.tm_mon && time1.tm_sec == time2.tm_sec && time1.tm_wday ==
		time2.tm_wday && time1.tm_yday == time2.tm_yday && time1.tm_year == time2.tm_year;
}

int FileManager::file_create(const string& fileName, const string& procName) {

	//Sprawdzenie poprawnosci
	{
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME;
		}

		if (!check_if_name_used(fileName)) {
			return FILE_ERROR_NAME_USED;
		}
	}

	//Jêzeli dzia³a
	{
		if (!check_if_name_used(fileName)) {
			const u_int inodeId = fileSystem.get_free_inode_id();
			fileSystem.rootDirectory[fileName] = inodeId;
			fileSystem.inodeBitVector[inodeId] = true;
		}

		return file_open(fileName, procName, FILE_OPEN_W_MODE);
	}
}

int FileManager::file_write(const string& fileName, const string& procName, const string& data) {

	Inode* inode;

	//Czêœæ sprawdzaj¹ca
	{
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME;
		}

		if (data.size() > MAX_DATA_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG;
		}

		if (data.size() > BLOCK_INDEX_NUMBER * BLOCK_SIZE &&
			fileSystem.freeSpace < calculate_needed_blocks(data.size())*BLOCK_SIZE + BLOCK_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG;
		}

		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = fileSystem.rootDirectory.find(fileName);
		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND;
		}
		inode = &fileSystem.inodeTable[fileIterator->second];

		if (data.size() > fileSystem.freeSpace - inode->blocksOccupied*BLOCK_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG;
		}
	}

	//Czêœæ dzia³aj¹ca
	file_deallocate(inode);
	return FILE_ERROR_NONE;
}

int FileManager::file_append(const string& fileName, const string& procName, const string& data) {
	Inode* inode;

	//Czêœæ sprawdzaj¹ca
	{
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME;
		}

		if (data.size() > BLOCK_INDEX_NUMBER * BLOCK_SIZE && fileSystem.freeSpace < calculate_needed_blocks(data.size())*BLOCK_SIZE + BLOCK_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG;
		}

		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = fileSystem.rootDirectory.find(fileName);

		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND;
		}
		inode = &fileSystem.inodeTable[fileIterator->second];

		if (inode->realSize + data.size() > MAX_DATA_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG;
		}

		if (data.size() > fileSystem.freeSpace) {
			return FILE_ERROR_DATA_TOO_BIG;
		}
	}

	return FILE_ERROR_NONE;
}


int FileManager::file_read(const string& fileName, const string& procName, const u_short_int& byteNumber, string& result) {
	//Iterator zwracany podczas przeszukiwania za plikiem o podanej nazwie
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME;
		}

		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND;
		}

	}

	return FILE_ERROR_NONE;
}

int FileManager::file_read_all(const string& fileName, const string& procName, string& result) {
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME;
		}

		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND;
		}
	}

	//Czêœæ dzia³aj¹ca
	return file_read(fileName, procName, fileSystem.inodeTable[fileSystem.rootDirectory[fileName]].realSize, result);
}

int FileManager::file_delete(const string& fileName, const string& procName) {
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME;
		}

		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND;
		}

		else if (file_accessing_proc_count(fileName) == 0)
		{
		}
	}

	//Czêœæ dzia0³aj¹ca
	{
		if (fileSystem.rootDirectory.find(fileName) != fileSystem.rootDirectory.end()) {
			Inode* inode = &fileSystem.inodeTable[fileIterator->second];

			file_deallocate(inode);
			fileSystem.inodeTable[fileIterator->second].clear();
			fileSystem.rootDirectory.erase(fileName);
		}
		return FILE_ERROR_NONE;
	}
}

int FileManager::file_open(const string& fileName, const string& procName, const unsigned int& mode) {
	Inode* file;
	bitset<2>mode_(mode);

	//Czêœæ sprawdzaj¹ca
	{
		if (fileName.empty()) {
			return FILE_ERROR_DATA_TOO_BIG;
		}

		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = fileSystem.rootDirectory.find(fileName);

		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND;
		}
		file = &fileSystem.inodeTable[fileIterator->second];
	}

	//Czêœæ dzia³aj¹ca
	{

		if (file_accessing_proc_count(fileName) == 0) {
		}

		return FILE_ERROR_NONE;
	}
}

int FileManager::file_close(const string& fileName, const string& procName) {
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME;
		}

		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND;
		}
	}

	//Czêœæ dzia³aj¹ca
	{
	
		Inode* file = &fileSystem.inodeTable[fileIterator->second];

		if (file_accessing_proc_count(fileName) == 0) {
		}

		return FILE_ERROR_NONE;
	}
}

bool FileManager::file_exists(const std::string& fileName) {
	if (fileSystem.rootDirectory.find(fileName) != fileSystem.rootDirectory.end()) {
		return true;
	}
	else {
		return false;
	}
}


//Metody dodatkowe

int FileManager::file_close_all(const string& procName) {
	vector<pair<string, string>> fileNames;
	for (const auto& elem : accessedFiles) {
		fileNames.push_back(elem.first);
	}
	for (const pair<string, string>& fileName : fileNames) {
		if (fileName.second == procName) {
			if (const int result = file_close(fileName.first, fileName.second) != 0) {
				return result;
			}
		}
	}
	return FILE_ERROR_NONE;
}

int FileManager::file_close_all() {
	vector<pair<string, string>> fileNames;
	for (const auto& elem : accessedFiles) {
		fileNames.push_back(elem.first);
	}
	for (const pair<string, string>& fileName : fileNames) {
		if (const int result = file_close(fileName.first, fileName.second) != 0) {
			return result;
		}
	}

	return FILE_ERROR_NONE;
}


//Wyswietlanie informacji

void FileManager::display_file_system_params() {
	cout << " |       Pojemnosc dysku : " << DISK_CAPACITY << " B\n";
	cout << " |         Rozmiar bloku : " << static_cast<int>(BLOCK_SIZE) << " B\n";
	cout << " |    Maks rozmiar pliku : " << MAX_FILE_SIZE << " B\n";
	cout << " |   Maks rozmiar danych : " << MAX_DATA_SIZE << " B\n";
	cout << " | Maks indeksow w bloku : " << static_cast<int>(BLOCK_INDEX_NUMBER) << " Indeksow\n";
	cout << " | Maks indeksow w pliku : " << static_cast<int>(BLOCK_INDEX_NUMBER) << " Indeksow\n";
	cout << " |     Maks ilosc plikow : " << static_cast<int>(INODE_NUMBER_LIMIT) << " Plikow\n";
}

void FileManager::display_root_directory_info() {
	cout << " |          Nazwa : " << "root" << '\n';
	cout << " | Rozmiar dancyh : " << calculate_directory_size() << " B\n";
	cout << " |        Rozmiar : " << calculate_directory_size_on_disk() << " B\n";
	cout << " |   Ilosc plikow : " << fileSystem.rootDirectory.size() << " Plikow\n";
}

int FileManager::display_file_info(const string& name) {

	if (name.empty()) {
		return FILE_ERROR_EMPTY_NAME;
	}

	const auto fileIterator = fileSystem.rootDirectory.find(name);
	if (fileIterator == fileSystem.rootDirectory.end()) {
		return FILE_ERROR_NOT_FOUND;
	}

	auto file = &fileSystem.inodeTable[fileIterator->second];

	//Wyœwietlanie
	{
		cout << " |           Nazwa : " << name << '\n';
		cout << " |   Numer I-wezla : " << fileIterator->second << '\n';
		cout << " |  Rozmiar danych : " << file->realSize << " Bytes\n";
		cout << " |         Rozmiar : " << file->blocksOccupied*BLOCK_SIZE << " Bytes\n";
		cout << " |\n";
		cout << " | Indeksy w pliku : ";
		for (const auto& elem : file->directBlocks) {
			if (elem != -1) { cout << elem << ' '; }
			else { cout << -1 << ' '; }
		}
		cout << '\n';

		cout << " |    Indeks bloku : ";
		if (file->singleIndirectBlocks != -1) {
			cout << file->singleIndirectBlocks << " \n";
			cout << " | Indeksy w bloku : ";
			array<u_int, BLOCK_SIZE / 2>blocks{};
			blocks.fill(-1);
			blocks = disk.read_arr(file->singleIndirectBlocks*BLOCK_SIZE);
			for (const u_int& block : blocks) {
				if (block != -1) {
					cout << block << ' ';
				}
				else { cout << -1 << ' '; }
			}
			cout << '\n';
		}
		else { cout << -1 << "\n"; }

		cout << " |   Zapisane dane : ";
		FileIO tempIO(&disk, file, bitset<2>(3));
		cout << tempIO.read_all() << '\n';
	}

	return FILE_ERROR_NONE;
}

void FileManager::display_root_directory() {
	cout << string(1, ' ') << "root" << "\n";

	for (auto it = fileSystem.rootDirectory.begin(); it != fileSystem.rootDirectory.end(); ++it) {
		++it;
		cout << ' ';
		if (it != fileSystem.rootDirectory.end()) { cout << static_cast<u_char>(195); }
		else { cout << static_cast<u_char>(192); }
		--it;
		cout << string(2, static_cast<u_char>(196)) << " " << it->first << '\n';
	}
}

void FileManager::display_disk_content_char() {
	for (u_int i = 0; i < DISK_CAPACITY / BLOCK_SIZE; i++) {
		cout << setfill('0') << setw(2) << i << ".  ";
		for (u_int j = 0; j < BLOCK_SIZE; j++) {
			if (disk.space[i*BLOCK_SIZE + j] >= 0 && disk.space[i*BLOCK_SIZE + j] <= 32) { cout << "."; }
			else { cout << disk.space[i*BLOCK_SIZE + j]; }
		}
		if (i % 2 == 1) { cout << '\n'; }
		else { cout << "  "; }
	}
	cout << '\n';
}

void FileManager::display_bit_vector() {
	u_int index = 0;
	for (u_int i = 0; i < fileSystem.bitVector.size(); i++) {
		if (i % 8 == 0) { cout << setfill('0') << setw(2) << (index / 8) + 1 << ". "; }
		cout << fileSystem.bitVector[i] << (index % 8 == 7 ? "\n" : " ");
		index++;
	}
	cout << '\n';
}

void FileManager::display_block_char(const unsigned int& block) {
	int i = block * BLOCK_SIZE;
	if (block < fileSystem.bitVector.size()) {
		cout << setfill('0') << setw(2) << block << ". ";
		while (true) {
			if (disk.space[i] >= 0 && disk.space[i] <= 32) { cout << "."; }
			else { cout << disk.space[i]; }
			i++;
			if (i % BLOCK_SIZE == 0) { break; }
		}
	}
	cout << '\n';
}
