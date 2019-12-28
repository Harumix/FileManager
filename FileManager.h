#pragma once

#include <string>
#include <array>
#include <bitset>
#include <vector>
#include <unordered_map>
#include <map>

#define FILE_OPEN_R_MODE  1 
#define FILE_OPEN_W_MODE  2 
#define _CRT_SECURE_NO_WARNINGS
#define FILE_ERROR_NONE				0
#define FILE_ERROR_EMPTY_NAME		1
#define FILE_ERROR_NAME_USED		2
#define FILE_ERROR_NO_INODES_LEFT	3
#define FILE_ERROR_DATA_TOO_BIG		4
#define FILE_ERROR_NOT_FOUND		5
#define FILE_ERROR_NOT_OPENED		6
#define FILE_ERROR_OPENED			7
#define FILE_ERROR_SYNC				8
#define FILE_ERROR_NOT_R_MODE		9
#define FILE_ERROR_NOT_W_MODE		10
#define FILE_SYNC_WAITING 30

class FileManager {
private:
	using u_int = unsigned int;
	using u_short_int = unsigned short int;

	//Do edycji 
	static const uint8_t BLOCK_SIZE = 32;	   		//Rozmiar bloku (bajty)
	static const u_short_int DISK_CAPACITY = 1024;	//Pojemnoœæ dysku (bajty)
	static const uint8_t BLOCK_INDEX_NUMBER = 3;	//Wartoœæ oznaczaj¹ca d³ugoœæ pola blockDirect
	static const uint8_t INODE_NUMBER_LIMIT = 32;	//Maksymalna iloœæ elementów w katalogu
	static const bool BLOCK_FREE = false;           //Wartoœæ oznaczaj¹ca wolny blok
	static const bool BLOCK_OCCUPIED = !BLOCK_FREE; //Wartoœæ oznaczaj¹ca zajêty blok

	//Maksymalny rozmiar danych
	static const u_short_int MAX_DATA_SIZE = (BLOCK_INDEX_NUMBER + BLOCK_SIZE / 2)*BLOCK_SIZE;
	//Maksymalny rozmiar pliku (wliczony blok indeksowy)
	static const u_short_int MAX_FILE_SIZE = MAX_DATA_SIZE + BLOCK_SIZE;

	struct Inode {
		uint8_t blocksOccupied = 0;  //Iloœæ zajmowanych bloków
		u_short_int realSize = 0;    //Rzeczywisty rozmiar pliku (rozmiar danych)
		std::array<u_int, BLOCK_INDEX_NUMBER> directBlocks{};	//Bezpoœrednie indeksy
		u_int singleIndirectBlocks; //Indeks bloku indeksowego, zpisywanego na dysku

		Inode();

		virtual ~Inode() = default;

		void clear();
	};

	struct Disk {
		//Tablica reprezentuj¹ca przestrzeñ dyskow¹ (jeden indeks - jeden bajt)
		std::array<char, DISK_CAPACITY> space{};

		Disk();

		void write(const u_short_int& begin, const std::string& data);
		void write(const u_short_int& begin, const std::array<u_int, BLOCK_SIZE / 2>& data);

		const std::string read_str(const u_int& begin) const;
		const std::array<u_int, BLOCK_SIZE / 2> read_arr(const u_int& begin) const;
	} disk; 
	
	//Struktura dysku
	struct FileSystem {
		u_int freeSpace{ DISK_CAPACITY }; //Zawiera informacje o iloœci wolnego miejsca na dysku (bajty)

		//Wektor bitowy bloków (domyœlnie: 0 - wolny blok, 1 - zajêty blok)
		std::bitset<DISK_CAPACITY / BLOCK_SIZE> bitVector;

		
		//Tablica i-wêz³ów
		std::array<Inode, INODE_NUMBER_LIMIT> inodeTable;
		//Pomocnicza tablica 'zajêtoœci' i-wêz³ów (1 - zajêty, 0 - wolny).
		std::bitset<INODE_NUMBER_LIMIT> inodeBitVector;
		std::unordered_map<std::string, u_int> rootDirectory;

		FileSystem();

		u_int get_free_inode_id();

		void reset();
	} fileSystem; 

	//Klasa odczytu/zapisu
	class FileIO {
	private:
#define READ_FLAG 0
#define WRITE_FLAG 1

		std::string buffer;
		u_short_int readPos = 0;
		Disk* disk;
		Inode* file;

		bool readFlag;
		bool writeFlag;

	public:
		FileIO() : disk(nullptr), file(nullptr), readFlag(false), writeFlag(false) {}
		FileIO(Disk* disk, Inode* inode, const std::bitset<2>& mode) : disk(disk), file(inode),
			readFlag(mode[READ_FLAG]), writeFlag(mode[WRITE_FLAG]) {}

		void buffer_update(const int8_t& blockNumber);

		std::string read(const u_short_int& byteNumber);
		std::string read_all();
		void reset_read_pos() { readPos = 0; }

		void write(const std::vector<std::string>& dataFragments, const int8_t& startIndex) const;

		const std::bitset<2> get_flags() const;
	};

	//Mapa dostêpu dla poszczególnych plików i procesów
	//Klucz   - para nazwa pliku, nazwa procesu
	std::map<std::pair<std::string, std::string>, FileIO> accessedFiles;

public:
	explicit FileManager() = default;

	//Tworzy plik o podanej nazwie w obecnym katalogu. Po stworzeniu plik jest otwarty w trybie do zapisu.
	int file_create(const std::string& fileName, const std::string& procName);

	//Zapisuje podane dane w danym pliku usuwaj¹c poprzedni¹ zawartoœæ.
	int file_write(const std::string& fileName, const std::string& procName, const std::string& data);

	//Dopisuje podane dane na koniec pliku.
	int file_append(const std::string& fileName, const std::string& procName, const std::string& data);

	//Odczytuje podan¹ liczbê bajtów z pliku. Po odczycie przesuwa siê wskaŸnik odczytu.
	int file_read(const std::string& fileName, const std::string& procName, const u_short_int& byteNumber, std::string& result);

	//Odczytuje ca³e dane z pliku.
	int file_read_all(const std::string& fileName, const std::string& procName, std::string& result);

	//Usuwa plik o podanej nazwie znajduj¹cy siê w obecnym katalogu.\n
	int file_delete(const std::string& fileName, const std::string& procName);

	//Otwiera plik z podanym trybem dostêpu:
	int file_open(const std::string& fileName, const std::string& procName, const unsigned int& mode);

	//Zamyka plik o podanej nazwie.
	int file_close(const std::string& fileName, const std::string& procName);

	//Sprawdza czy plik istnieje.
	bool file_exists(const std::string& fileName);

	//Zamyka wszystkie pliki dla danego procesu.
	int file_close_all(const std::string& procName);

	//Zamyka wszystkie pliki.
	int file_close_all();


	//Wyœwietla parametry systemu plików.
	static void display_file_system_params();

	//Wyœwietla informacje o wybranym katalogu.
	void display_root_directory_info();

	//Wyœwietla informacje o pliku.
	int display_file_info(const std::string& name);

	//Wyœwietla strukturê katalogów.
	void display_root_directory();

	//Wyœwietla zawartoœæ dysku jako znaki.
	void display_disk_content_char();

	//Wyœwietla wektor bitowy.
	void display_bit_vector();

	void display_block_char(const unsigned int& block);

private:

	bool check_if_name_used(const std::string& name);
	
	bool check_if_enough_space(const u_int& dataSize) const;

	static u_int calculate_needed_blocks(const size_t& dataSize);

	size_t calculate_directory_size();

	size_t calculate_directory_size_on_disk();

	void file_truncate(Inode* file, const u_int& neededBlocks);

	void file_add_indexes(Inode* file, const std::vector<u_int>& blocks);

	void file_deallocate(Inode* file);

	void file_allocate_blocks(Inode* file, const std::vector<u_int>& blocks);

	void file_allocation_increase(Inode* file, const u_int& neededBlocks);

	const std::vector<u_int> find_unallocated_blocks_fragmented(u_int blockNumber);

	const std::vector<u_int> find_unallocated_blocks_best_fit(const u_int& blockNumber);

	const std::vector<u_int> find_unallocated_blocks(const u_int& blockNumber);

	bool is_file_opened_write(const std::string& fileName);

	int file_accessing_proc_count(const std::string& fileName);

	std::string get_file_data_block(Inode* file, const int8_t& indexNumber) const;

	void file_write(Inode* file, FileIO* IO, const std::string& data);

	void file_append(Inode* file, FileIO* IO, const std::string& data);

	static const tm get_current_time_and_date();

	void change_bit_vector_value(const u_int& block, const bool& value);

	static const std::vector<std::string> fragmentate_data(const std::string& data);
};

extern FileManager fm;
