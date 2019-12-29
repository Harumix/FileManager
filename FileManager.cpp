#include "FileManager.h"
#include <iomanip>
#include <iostream>

using namespace std;

FileManager fm;

//Aliasy
using u_int = unsigned int;
using u_short_int = unsigned short int;
using u_char = unsigned char;

//Operatory
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

//----------------------------------Czêœæ publiczna-------------------------------------------

//G³ówne metody

/**
Tworzy plik o podanej nazwie w obecnym katalogu. Po stworzeniu plik jest otwarty w trybie do zapisu.

fileName Nazwa pliku.
procName Nazwa procesu tworz¹cego.
*/
int FileManager::file_create(const string& fileName, const string& procName) {
	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nie zosta³a podana pusta nazwa pliku
		if (fileName.empty()) { 
		return FILE_ERROR_EMPTY_NAME; 
		}

		//Sprawdzenie czy nie istnieje ju¿ plik o podanej nazwie
		if (check_if_name_used(fileName)) {
		return FILE_ERROR_NAME_USED; 
		}
		
		//Sprawdzenie czy nie zosta³a przekroczona maksymalna liczba plików
		if (!check_if_name_used(fileName)) {
			if (fileSystem.rootDirectory.size() >= INODE_NUMBER_LIMIT) {
				return FILE_ERROR_NO_INODES_LEFT;
			}
		}
	}

	//Czêœæ dzia³aj¹ca
	{
		if (!check_if_name_used(fileName)) {
			//Uzyskanie wolnego id dla wêzla
			const u_int inodeId = fileSystem.get_free_inode_id();
			//Dodanie pliku do katalogu g³ównego
			fileSystem.rootDirectory[fileName] = inodeId;
			//Zablokowanie id
			fileSystem.inodeBitVector[inodeId] = true;
		}

		//Zapisanie pliku w tablicy wezlow
		Inode* file = &fileSystem.inodeTable[fileSystem.rootDirectory[fileName]];
		//Przypisanie do pliku daty
		file->creationTime = get_current_time_and_date();
		file->modificationTime = get_current_time_and_date();
	}
}

/**
Zapisuje podane dane w danym pliku usuwaj¹c poprzedni¹ zawartoœæ.

fileName Nazwa pliku.
procName Nazwa procesu, który chce zapisaæ do pliku.
data Dane do zapisu.
*/
int FileManager::file_write(const string& fileName, const string& procName, const string& data) {
	Inode* inode;

	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nie zosta³a podana pusta nazwa pliku
		if (fileName.empty()) { 
		return FILE_ERROR_EMPTY_NAME; 
		}

		//Sprawdzenie czy dane nie przekraczaj¹ maksymalnego rozmiaru danych
		if (data.size() > MAX_DATA_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG; 
			}

		//Sprawdzenie czy dane zmieszcz¹ siê na dysku
		if (data.size() > BLOCK_INDEX_NUMBER * BLOCK_SIZE &&
			fileSystem.freeSpace < calculate_needed_blocks(data.size())*BLOCK_SIZE + BLOCK_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG;
		}

		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = fileSystem.rootDirectory.find(fileName);

		//Sprawdzenie czy plik znajdujê siê w katalogu
		if (fileIterator == fileSystem.rootDirectory.end()) { 
		return FILE_ERROR_NOT_FOUND; 
		}
		inode = &fileSystem.inodeTable[fileIterator->second];

		//Sprawdzenie czy plik zosta³ otworzony
		if (!inode->opened) { 
		return FILE_ERROR_NOT_OPENED; 
		}

		//Sprawdzenie czy istnieje proces odpowiedzialny za otwarcie pliku
		if (accessedFiles.find(pair(fileName, procName)) == accessedFiles.end()) {
			return FILE_ERROR_NOT_OPENED;
		}

		//Sprawdzenie czy plik otwarty jest w trybie do zapisu
		if (!accessedFiles[pair(fileName, procName)].get_flags()[WRITE_FLAG]) {
			return FILE_ERROR_NOT_W_MODE; 
			}

		//Sprawdzenie czy na dysku jest wystarczaj¹ca ilosc blokow do zapisania danych
		if (data.size() > fileSystem.freeSpace - inode->blocksOccupied*BLOCK_SIZE) { 
		return FILE_ERROR_DATA_TOO_BIG; 
		}
	}

	//Czêœæ dzia³aj¹ca
	file_deallocate(inode);
	file_write(inode, &accessedFiles[pair(fileName, procName)], data);
	return FILE_ERROR_NONE;
}

/**
Dopisuje podane dane na koniec pliku.

fileName Nazwa pliku.
procName Nazwa procesu, który chce dopisaæ do pliku.
data Dane do zapisu.
*/
int FileManager::file_append(const string& fileName, const string& procName, const string& data) {
	Inode* inode;

	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nazwa nie jest pusta
		if (fileName.empty()) { 
		return FILE_ERROR_EMPTY_NAME; 
		}

		//Sprawdzenie czy dane nie s¹ za duze
		if (data.size() > BLOCK_INDEX_NUMBER * BLOCK_SIZE && fileSystem.freeSpace < calculate_needed_blocks(data.size())*BLOCK_SIZE + BLOCK_SIZE) {
			return FILE_ERROR_DATA_TOO_BIG;
		}

		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = fileSystem.rootDirectory.find(fileName);

		//Sprawdzenie czy plik istnieje
		if (fileIterator == fileSystem.rootDirectory.end()) {
			return FILE_ERROR_NOT_FOUND; 
			}
		inode = &fileSystem.inodeTable[fileIterator->second];

		//Sprawdzenie czy plik jest otwarty
		if (!inode->opened) { 
		return FILE_ERROR_NOT_OPENED; 
		}

		//Sprawdzenie czy plik jest otwarty w trybie zapisu
		if (!accessedFiles[pair(fileName, procName)].get_flags()[WRITE_FLAG]) { 
		return FILE_ERROR_NOT_W_MODE; 
		}

		//Sprawdzenie czy po dopisaniu danych rozmiar nie bedzie za duzy
		if (inode->realSize + data.size() > MAX_DATA_SIZE) { 
		return FILE_ERROR_DATA_TOO_BIG; 
		}

		//Sprawdzenie czy rozmiar danych nie jest za duzy
		if (data.size() > fileSystem.freeSpace) { 
		return FILE_ERROR_DATA_TOO_BIG; 
		}
	}

	//Czêœæ dzia³aj¹ca
	file_write(inode, &accessedFiles[pair(fileName, procName)], data);
	return FILE_ERROR_NONE;
}

/**
Odczytuje podan¹ liczbê bajtów z pliku. Po odczycie przesuwa siê wskaŸnik odczytu.

fileName Nazwa pliku.
procName Nazwa procesu, który chce odczytaæ z pliku.
byteNumber Iloœæ bajtów do odczytu.
result Miejsce do zapisania odczytanych danych.
*/
int FileManager::file_read(const string& fileName, const string& procName, const u_short_int& byteNumber, string& result) {
	//Iterator zwracany podczas przeszukiwania za plikiem o podanej nazwie
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nazwa nie jest pusta
		if (fileName.empty()) { 
		return FILE_ERROR_EMPTY_NAME; 
		}

		//Sprawdzenie czy plik istnieje
		if (fileIterator == fileSystem.rootDirectory.end()) { 
		return FILE_ERROR_NOT_FOUND; 
		}

		//Sprawdzenie czy plik jest otwarty
		if (accessedFiles.find(pair(fileName, procName)) == accessedFiles.end()) { 
		return FILE_ERROR_NOT_OPENED; 
		}

		//Sprawdzenie czy plik jest otwarty w trybie do odczytu
		if (!accessedFiles[pair(fileName, procName)].get_flags()[READ_FLAG]) { 
		return FILE_ERROR_NOT_R_MODE; 
		}
	}

	//Czêœæ dzia³aj¹ca ----------------------
	result = accessedFiles[pair(fileName, procName)].read(byteNumber);
	return FILE_ERROR_NONE;
}

/**
Odczytuje ca³e dane z pliku.

fileName Nazwa pliku.
procName Nazwa procesu, który chce odczytaæ z pliku.
result Miejsca do zapisania odczytanych danych.
*/
	
int FileManager::file_read_all(const string& fileName, const string& procName, string& result) {
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nazwa nie jest pusta
		if (fileName.empty()) { 
		return FILE_ERROR_EMPTY_NAME;
		}

		//Sprawdzenie czy plik istnieje
		if (fileIterator == fileSystem.rootDirectory.end()) { 
		return FILE_ERROR_NOT_FOUND;
		}
		
		//Sprawdzenie czy plik jest otwarty
		if (accessedFiles.find(pair(fileName, procName)) == accessedFiles.end()) { 
		return FILE_ERROR_NOT_OPENED; 
		}

		//Sprawdzenie czy plik jest otwarty w trybie do odczytu
		if (!accessedFiles[pair(fileName, procName)].get_flags()[READ_FLAG]) { 
		return FILE_ERROR_NOT_R_MODE; 
		}
	}

	//Czêœæ dzia³aj¹ca
	return file_read(fileName, procName, fileSystem.inodeTable[fileSystem.rootDirectory[fileName]].realSize, result);
}

/**
Usuwa plik o podanej nazwie znajduj¹cy siê w obecnym katalogu.

fileName Nazwa pliku.
procName Nazwa procesu, który chce usun¹æ pliku.
*/
int FileManager::file_delete(const string& fileName, const string& procName) {
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nazwa pliku nie jest pusta
		if (fileName.empty()) { 
		return FILE_ERROR_EMPTY_NAME; 
		}

		//Sprawdzenie czy plik istnieje
		if (fileIterator == fileSystem.rootDirectory.end()) { 
		return FILE_ERROR_NOT_FOUND; 
		}
		
		//Sprawdzenie czy plik jest otwarty
		if (accessedFiles.find(pair(fileName, procName)) != accessedFiles.end()) { 
		return FILE_ERROR_OPENED;
		}
	}

	//Czêœæ dzia0³aj¹ca
	{
		if (fileSystem.rootDirectory.find(fileName) != fileSystem.rootDirectory.end()) {
			Inode* inode = &fileSystem.inodeTable[fileIterator->second];

			file_deallocate(inode);
			//Usuniecie wezla odpowiedzialnego za plik
			fileSystem.inodeTable[fileIterator->second].clear();

			//Usuñ wpis o pliku z obecnego katalogu
			fileSystem.rootDirectory.erase(fileName);

		}
		return FILE_ERROR_NONE;
	}
}

/**
Otwiera plik z podanym trybem dostêpu:
- R (read) - do odczytu
- W (write) - do zapisu
- RW (read/write) - do odczytu i zapisu

fileName Nazwa pliku.
procName Nazwa procesu tworz¹cego.
mode Tryb dostêpu do pliku.
*/
int FileManager::file_open(const string& fileName, const string& procName, const unsigned int& mode) {
	Inode* file;
	bitset<2>mode_(mode);

	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nazwa nie jest pusta
		if (fileName.empty()) {
			return FILE_ERROR_EMPTY_NAME; 
			}

		//Iterator zwracany podczas przeszukiwania obecnego katalogu za plikiem o podanej nazwie
		const auto fileIterator = fileSystem.rootDirectory.find(fileName);

		//Sprawdzenie czy plik istnieje
		if (fileIterator == fileSystem.rootDirectory.end()) { 
		return FILE_ERROR_NOT_FOUND; 
		}
		file = &fileSystem.inodeTable[fileIterator->second];

	}

	//Czêœæ dzia³aj¹ca
	{
		if (accessedFiles.find(pair(fileName, procName)) != accessedFiles.end()) {
			accessedFiles.erase(pair(fileName, procName));
		}

		if (file_accessing_proc_count(fileName) == 0) {
			file->opened = false;
		}

		file->opened = true;

		accessedFiles[pair(fileName, procName)] = FileIO(&disk, file, mode_);

		return FILE_ERROR_NONE;
	}
}

/**
Zamyka plik o podanej nazwie.

fileName Nazwa pliku.
procName Nazwa procesu, który zamkn¹æ pliku.
Kod b³êdu. 0 oznacza brak b³êdu.
*/
int FileManager::file_close(const string& fileName, const string& procName) {
	const auto fileIterator = fileSystem.rootDirectory.find(fileName);

	//Czêœæ sprawdzaj¹ca
	{
		//Sprawdzenie czy nazwa nie jest pusta
		if (fileName.empty()) { 
		return FILE_ERROR_EMPTY_NAME; 
		}

		//Sprawdzenie czy plik istnieje
		if (fileIterator == fileSystem.rootDirectory.end()) { 
		return FILE_ERROR_NOT_FOUND; 
		}
		
		//Sprawdzenie czy plik jest otwarty
		if (accessedFiles.find(pair(fileName, procName)) == accessedFiles.end()) { 
		return FILE_ERROR_NOT_OPENED; 
		}
	}

	//Czêœæ dzia³aj¹ca
	{
		accessedFiles.erase(pair(fileName, procName));
		Inode* file = &fileSystem.inodeTable[fileIterator->second];

		if (file_accessing_proc_count(fileName) == 0) {
			file->opened = false;
		}
		
		return FILE_ERROR_NONE;
	}
}


/**
Sprawdza czy plik istnieje.

fileName Nazwa pliku do znalezienia.
*/
bool FileManager::file_exists(const std::string& fileName) {
	if (fileSyste
	m.rootDirectory.find(fileName) != fileSystem.rootDirectory.end()) { 
	return true; }
	else { 
	return false; 
	}
}


//Metody do wyœwietlania
/**
Wyœwietla parametry systemu plików.
*/
void FileManager::display_file_system_params() {
	cout << " |       Pojemnosc dysku : " << DISK_CAPACITY << " B\n";
	cout << " |         Rozmiar bloku : " << static_cast<int>(BLOCK_SIZE) << " B\n";
	cout << " |    Maks rozmiar pliku : " << MAX_FILE_SIZE << " B\n";
	cout << " |   Maks rozmiar danych : " << MAX_DATA_SIZE << " B\n";
	cout << " | Maks indeksow w bloku : " << static_cast<int>(BLOCK_INDEX_NUMBER) << " Indeksow\n";
	cout << " |     Maks ilosc plikow : " << static_cast<int>(INODE_NUMBER_LIMIT) << " Plikow\n";
}

/**
Wyœwietla informacje o wybranym katalogu.
*/
void FileManager::display_root_directory_info() {
	cout << " |          Nazwa : " << "root" << '\n';
	cout << " | Rozmiar dancyh : " << calculate_directory_size() << " B\n";
	cout << " |        Rozmiar : " << calculate_directory_size_on_disk() << " B\n";
	cout << " |   Ilosc plikow : " << fileSystem.rootDirectory.size() << " Plikow\n";
}
/**
Wyœwietla informacje o pliku.
*/
int FileManager::display_file_info(const string& name) {
	//Sprawdzenie czy nazwa nie jest pusta
	if (name.empty()) { 
	return FILE_ERROR_EMPTY_NAME; 
	}

	const auto fileIterator = fileSystem.rootDirectory.find(name);

	//Sprawdzenie czy plik istnieje
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
		cout << " |       Stworzono : " << file->creationTime << '\n';
		cout << " |   Zmodyfikowano : " << file->modificationTime << "\n";
		cout << " |\n";
		cout << " | Indeksy w pliku : ";
		for (const auto& elem : file->directBlocks) {
			if (elem != -1) { 
			cout << elem << ' '; 
			}
			else { 
			cout << -1 << ' ';
			}
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

/**
Wyœwietla strukturê katalogów.
*/
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

/**
Wyœwietla zawartoœæ dysku jako znaki.
*/
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

/**
Wyœwietla wektor bitowy.
*/
void FileManager::display_bit_vector() {
	u_int index = 0;
	for (u_int i = 0; i < fileSystem.bitVector.size(); i++) {
		if (i % 8 == 0) { cout << setfill('0') << setw(2) << (index / 8) + 1 << ". "; }
		cout << fileSystem.bitVector[i] << (index % 8 == 7 ? "\n" : " ");
		index++;
	}
	cout << '\n';
}

/**
Wyœwietla zawartoœc bloku indeksowego
*/
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
//----------------------------------Czêœæ prywatna-------------------------------------------
