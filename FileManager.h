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

//G³ówna klasa
class FileManager {
private:
	using u_int = unsigned int;
	using u_short_int = unsigned short int;

	//Definicje sta³ych wartoœci
	static const uint8_t BLOCK_SIZE = 32;	   		//Rozmiar bloku w bajtach
	static const u_short_int DISK_CAPACITY = 1024;	//Pojemnoœæ dysku w bajtach
	static const uint8_t BLOCK_INDEX_NUMBER = 3;	//Wielkoœc bloku indeksowego (dlugosc pola blockDirect)
	static const uint8_t INODE_NUMBER_LIMIT = 32;	//Maksymalna iloœæ elementów w katalogu
	static const bool BLOCK_FREE = false;           //Wartoœæ oznaczaj¹ca wolny blok
	static const bool BLOCK_OCCUPIED = !BLOCK_FREE; //Wartoœæ oznaczaj¹ca zajêty blok
	static const u_short_int MAX_DATA_SIZE = (BLOCK_INDEX_NUMBER + BLOCK_SIZE / 2)*BLOCK_SIZE; //Maksymalny rozmiar danych
	static const u_short_int MAX_FILE_SIZE = MAX_DATA_SIZE + BLOCK_SIZE; 					   //Maksymalny rozmiar pliku 

	//Klasa zawierajaca podstawowe informacje o pliku
	struct Inode {
		uint8_t blocksOccupied = 0;  //Iloœæ zajmowanych bloków
		u_short_int realSize = 0;    //Rozmiar danych
		std::array<u_int, BLOCK_INDEX_NUMBER> directBlocks{};	//Bezpoœrednie indeksy
		u_int singleIndirectBlocks; //Numer bloku indeksowego
		tm creationTime = tm();		//Czas i data utworzenia
		tm modificationTime = tm(); //Czas i data ostatniej modyfikacji pliku

		bool opened = false;

		Inode();

		virtual ~Inode() = default;

		void clear();
	};

	//Klasa odpowiedzialna za dysk
	struct Disk {
		std::array<char, DISK_CAPACITY> space{}; //Tablica reprezentuj¹ca przestrzeñ dyskow¹, jednemu indeksowi przypada jeden bajt

		Disk();

		void write(const u_short_int& begin, const std::string& data);
		void write(const u_short_int& begin, const std::array<u_int, BLOCK_SIZE / 2>& data);
		const std::string read_str(const u_int& begin) const;
		const std::array<u_int, BLOCK_SIZE / 2> read_arr(const u_int& begin) const;
		
	} disk;
	//Klasa odpowiedzialna za system plikow system plików
	struct FileSystem {
		u_int freeSpace{ DISK_CAPACITY }; //Zawiera informacje o iloœci wolnego miejsca na dysku (bajty)
		std::bitset<DISK_CAPACITY / BLOCK_SIZE> bitVector; //Wektor bitowy bloków
		std::array<Inode, INODE_NUMBER_LIMIT> inodeTable; //Tablica Inode'ów
		std::bitset<INODE_NUMBER_LIMIT> inodeBitVector; //Tablica 'zajêtoœci' wêz³ów
		std::unordered_map<std::string, u_int> rootDirectory; //Tablica przechowuj¹ca pliki w katalogu

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
	//Wartoœæ - semafor przypisany danemu procesowi
	std::map<std::pair<std::string, std::string>, FileIO> accessedFiles;



public:
	explicit FileManager() = default;

	//G³ówne metody
	int file_create(const std::string& fileName, const std::string& procName);

	int file_write(const std::string& fileName, const std::string& procName, const std::string& data);

	int file_append(const std::string& fileName, const std::string& procName, const std::string& data);

	int file_read(const std::string& fileName, const std::string& procName, const u_short_int& byteNumber, std::string& result);

	int file_read_all(const std::string& fileName, const std::string& procName, std::string& result);

	int file_delete(const std::string& fileName, const std::string& procName);

	int file_open(const std::string& fileName, const std::string& procName, const unsigned int& mode);

	int file_close(const std::string& fileName, const std::string& procName);

	bool file_exists(const std::string& fileName);


	//Metody odpowiedzialne za wyswietlanie informacji

	static void display_file_system_params();

	void display_root_directory_info();

	int display_file_info(const std::string& name);

	void display_root_directory();
	
	void display_disk_content_char();

	void display_bit_vector();

	void display_block_char(const unsigned int& block);

private:
	//Metody sluzace do sprawdzania
	bool check_if_name_used(const std::string& name);

	bool check_if_enough_space(const u_int& dataSize) const;

	//Metody sluzace do obliczej
	static u_int calculate_needed_blocks(const size_t& dataSize);

	size_t calculate_directory_size();

	size_t calculate_directory_size_on_disk();

	//Metody odpowiedzialne za alokacje
	void file_truncate(Inode* file, const u_int& neededBlocks);

	void file_add_indexes(Inode* file, const std::vector<u_int>& blocks);

	void file_deallocate(Inode* file);

	void file_allocate_blocks(Inode* file, const std::vector<u_int>& blocks);

	void file_allocation_increase(Inode* file, const u_int& neededBlocks);

	const std::vector<u_int> find_unallocated_blocks_fragmented(u_int blockNumber);

	const std::vector<u_int> find_unallocated_blocks_best_fit(const u_int& blockNumber);

	const std::vector<u_int> find_unallocated_blocks(const u_int& blockNumber);

	//Metody dodatkowe
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
