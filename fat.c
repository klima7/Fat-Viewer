#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <wchar.h>
#include "disc.h"
#include "fat.h"
#include "utils.h"

// Dane z boot sectora
struct fat_boot_sector_t bs;
// Adres katalogu głównego
static uint32_t root_dir_addr;
// Adres obszaru z klastrami
static uint32_t alloc_area_addr;

// Inicjowanie
int fat_init(void)
{
    // Odczytanie bootsectora
	int read_res = disc_readblock(&bs, 0, 1);
	if(read_res != 1) return 1;

	// Sprawdzenie poprawności sekwencji kończącej
	if(bs.boot_sector_end != FAT_END_SEQUENCE) return 1;

	// Obliczenie częstu używanych adresów
	root_dir_addr = (bs.bpb.reserved_sectors + bs.bpb.sectors_per_table * bs.bpb.tables_count) * SECTOR_SIZE;
	alloc_area_addr = root_dir_addr + bs.bpb.entries_in_root_directory * FAT_ENTRY_SIZE;

	return 0;
}

// Funkcja zwraca wartośc znajdującą się na danej pozycji danej tablicy fat
static uint16_t fat_read_table(int table, uint32_t cluster, int *err)
{
    // Zerowanie flagi błędu
    if(err != NULL) *err = 0;

    // Obliczenie adresu komórki talicy
	uint32_t table_start = (bs.bpb.reserved_sectors + bs.bpb.sectors_per_table * table) * SECTOR_SIZE;
	uint32_t entry_addr = table_start + cluster * 2;

	// Obliczenie numeru sektora i offsetu wewnątrz niego
	uint32_t sector_number = entry_addr / SECTOR_SIZE;
	uint32_t sector_offset = entry_addr % SECTOR_SIZE;

	// Odczytanie właściwego sektora
	uint8_t sector[SECTOR_SIZE];
	int res = disc_readblock(sector, sector_number, 1);
    if(res != 1)
    {
        if(err != NULL) *err = 1;
        return 0;
    }

    // Zwrócenie  komórki tablicy
	uint16_t *entry = (uint16_t *)(sector + sector_offset);
	return *entry;
}

// funkcja zwraca numer następnego klastra lub 0 jeśli kolejny klaster nie istnieje
uint16_t fat_get_next_cluster(uint16_t cluster, int *err)
{
    // Zerowanie flagi błędu
    if(err != NULL) *err = 0;

    // Sprawdzenie czy klaster nie jest ostatni
    if(FAT_CLUSTER_IS_END(cluster)) return 0;

    // Odczytanie numeru nastęþnego klastra z tablicy
    int err2 = 0;
    uint16_t value = fat_read_table(0, cluster, &err2);
    if(err2) { if(err != NULL) *err = 1; return 0; }

    // Zwrócenie numeru następnego klastra
    return value;
}

// Funkcja zwraca długość łańcucha klastrów rozpoczynającego się w klastrze number start
uint32_t fat_get_chain_length(uint16_t start, int *err)
{
    // Zerowanie flagi błędów
    if(err != NULL) *err = 0;

    // Długość łąńcucha klastrów i bieżący numer klastra
    uint32_t count = 0;
    uint16_t current = start;

    // Odczytywanie numberów kolejnych klastrów, aż do klastra kończącego
    while(!FAT_CLUSTER_IS_END(current))
    {
        count++;
        int err2 = 0;
        current = fat_get_next_cluster(current, &err2);
        if(err2) { *err = 1; return 0; }
    }

    // Zwrócenie liczby klastrów
    return count;
}

// Funkcja znajdująca wpis w katalogu rozpoczynającym się w klastrze dir_start odpowiadający za plik o nazwie name
int fat_get_entry_simple(uint16_t dir_start, const char *name, struct fat_directory_entry_t *res_entry)
{
    // Sprawdzenie argumentów
    if(name == NULL || res_entry == NULL) return 1;

    // Szukamy w katalogu głównym - zero ma specjalne znaczenie
    if(dir_start == 0)
    {
        // Inicjacja numeru sektora i przesunięcia
        uint32_t sector_number = root_dir_addr / SECTOR_SIZE;
        uint32_t sector_offset = 0;

        // Wczytanie zawartości pierwszego sektora
        uint8_t sector[SECTOR_SIZE];
        int res = disc_readblock(sector, sector_number, 1);
        if(res != 1) return 1;

        struct fat_directory_entry_t *entry = (struct fat_directory_entry_t *)sector;

        // Sprawdzanie wpisów do czasu sprawdzenia wszystkich
        for(uint32_t i=0; i<bs.bpb.entries_in_root_directory; i++)
        {
            // Połączenie nazwy pliku z rozszerzeniem
            char full_name[13];
            fat_join_filename((char *)entry->name, (char *)entry->ext, full_name);

            //Sprawdzenie czy nazwa pliku jest nazwą szukaną
            if(strcmp(name, full_name) == 0)
            {
                *res_entry = *entry;
                return 0;
            }

            // ------ Przejście do następnego wpisu ------
            entry++;
            sector_offset += FAT_ENTRY_SIZE;

            // Jeżeli wyszliśmy poza sektor to odczytujemy kolejny
            if(sector_offset >= SECTOR_SIZE)
            {
                // Znajdujemy numer kolejnego sektora
                sector_number++;

                // Odczytujemy kolejny sektor
                int res = disc_readblock(sector, sector_number, 1);
                if (res != 1) return 1;

                // Zmieniamy przesunięcie
                entry = (struct fat_directory_entry_t *)sector;
                sector_offset = 0;
            }
        }
        // Nie udało się znaleść wpisu w katalogu
        return 1;
    }

    // Nie szukamy w katalogu głównym, lecz w podkatalogu
    else
    {
        // Inicjowanie numeru klastra i przesunięcia
        uint32_t cluster_number = dir_start;
        uint32_t cluster_offset = 0;

        // Odczytanie pierwszego klastra
        uint8_t cluster[FAT_CLUSTER_SIZE];
        int res = disc_readblock(cluster, FAT_GET_DATA_ADDR(cluster_number)/SECTOR_SIZE, FAT_CLUSTER_SIZE/SECTOR_SIZE);
        if (res != FAT_CLUSTER_SIZE/SECTOR_SIZE) return 1;

        struct fat_directory_entry_t *entry = (struct fat_directory_entry_t *)cluster;

        // Sprawdzanie wpisów do czasu sprawdzenia wszystkich
        while(entry->name[0] != FAT_ENTRY_FREE)
        {
            // Połączenie nazwy pliku z rozszerzeniem
            char full_name[13];
            fat_join_filename((char *)entry->name, (char *)entry->ext, full_name);

            //Sprawdzenie czy nazwa pliku jest nazwą szukaną
            if(strcmp(name, full_name) == 0)
            {
                *res_entry = *entry;
                return 0;
            }

            // ------ Przejście do następnego wpisu ------
            entry++;
            cluster_offset += FAT_ENTRY_SIZE;

            // Jeżeli wyszliści poza klaster to odczytujemy kolejny
            if(cluster_offset >= FAT_CLUSTER_SIZE)
            {
                // Znajdujemy numer kolejnego klastra
                int err = 0;
                cluster_number = fat_get_next_cluster(cluster_number, &err);
                if(err) return 1;

                // Odczytujemy kolejny klaster
                int res = disc_readblock(cluster, FAT_GET_DATA_ADDR(cluster_number)/SECTOR_SIZE, FAT_CLUSTER_SIZE / SECTOR_SIZE);
                if (res != FAT_CLUSTER_SIZE / SECTOR_SIZE) return 1;

                // Zmieniamy przesunięcie
                entry = (struct fat_directory_entry_t *)cluster;
                cluster_offset = 0;
            }
        }
        // Nie udało się znaleść wpisu w katalogu
        return 1;
    }
}

// Funkcja zwracająca wpis w katalogu odpowiadający za plik wskazywany przez ścieżkę path
int fat_get_entry(const char *path, struct fat_directory_entry_t *res_entry)
{
    // Sprawdzenie poprawności argumentów
    if(path == NULL || res_entry == NULL) return 1;

    // Rozpoczęcie dzielenia ścieżki na tokeny
    char *tokens = strdup(path);
    char *token = strtok(tokens, "/");

    // Rozpoczęcie poszukiwanie od katalogu głównego
    uint16_t current_dir = 0;
    bool is_directory = true;

    struct fat_directory_entry_t entry;

    // Iterowanie po tokenach i przechodzenie do kolejnych katalogów
    while(token != NULL)
    {
        // Jeżeli w środku ścieżki znajduje się plik nie będący katalogiem to zgłoś błąd
        if(!is_directory) { free(tokens); return 1; }

        // Znalezienie wpisu w aktualnym katalogu
        int res = fat_get_entry_simple(current_dir, token, &entry);
        if(res != 0) { free(tokens); return 1; }

        // Znalezienie numeru startowego kolejnego katalogu/pliku i określenie czy jest katalogiem
        current_dir = entry.file_start;
        is_directory = entry.attr & FAT_ATTR_SUBDIRECTORY;

        // Znalezienie kolejnego tokena
        token = strtok(NULL, "/");
    }

    *res_entry = entry;
    free(tokens);
    return 0;
}

// Funkcja zwraca wpis w katalogu rozpoczynającym się w klastrze dir_start z pozycji dir_pos
int fat_get_entry_POS(uint16_t dir_start, uint32_t dir_pos, struct fat_directory_entry_t *res_entry)
{
    // Przeszukiwanym katalogiem jest katalog główny
    if(dir_start == 0)
    {
        // Znalezienie sektora z wpisem
        uint32_t entry_addr = root_dir_addr + dir_pos * FAT_ENTRY_SIZE;
        uint32_t sector_number = entry_addr / SECTOR_SIZE;
        uint32_t sector_offset = entry_addr % SECTOR_SIZE;

        // Wczytanie zawartości sektora
        uint8_t sector[SECTOR_SIZE];
        int res = disc_readblock(sector, sector_number, 1);
        if(res != 1) return 1;

        // Zwrócenie znalezionego wpisu
        struct fat_directory_entry_t *entry = (struct fat_directory_entry_t *) (sector + sector_offset);
        *res_entry = *entry;
        return 0;
    }

    // Przeszukiwanym katalogiem jest jakiś podkatalog
   else
    {
        // Obliczenie przesunięcia od początku katalogu
        uint32_t entry_offset = dir_pos * FAT_ENTRY_SIZE;

        // Znalezienie klastra z wpisem
        uint32_t current_cluster = dir_start;
        while (entry_offset >= FAT_CLUSTER_SIZE) {
            int err = 0;
            current_cluster = fat_get_next_cluster(current_cluster, &err);
            if (err) return 1;
            entry_offset -= FAT_CLUSTER_SIZE;
        }

        // Odczyt odpowiedniego klastra
        uint8_t cluster[FAT_CLUSTER_SIZE];
        int res = disc_readblock(cluster, FAT_GET_DATA_ADDR(current_cluster) / SECTOR_SIZE, FAT_CLUSTER_SIZE / SECTOR_SIZE);
        if (res != FAT_CLUSTER_SIZE / SECTOR_SIZE) return 1;

        // Zwrócenie znalezionego wpisu
        struct fat_directory_entry_t *entry = (struct fat_directory_entry_t *) (cluster + entry_offset);
        *res_entry = *entry;
        return 0;
    }
}

// Funkcja łączy nazwę z rozszerzeniem w ukłądzie 8.3 i zapisuje wynik do result
char *fat_join_filename(const char *name, const char *ext, char *result)
{
    // Wydobądź nazwe bez spacji
	char name2[9];
	memcpy(name2, name, 8);
	name2[8] = 0;
	for(int i=7; name2[i]==' ' && i>=0; i--)
		name2[i] = 0;

	// Wydobądź rozszerzenie bez spacji
	char ext2[4];
	memcpy(ext2, ext, 3);
	ext2[3] = 0;
	for(int i=2; ext2[i]==' ' && i>=0; i--)
		ext2[i] = 0;

	// Wpisz do wyniku nazwe
	*result = 0;
	strcat(result, name2);

	// Dopisz rozszerzenie jeśli istnieje
	if(*ext2 != 0)
	{
		strcat(result, ".");
		strcat(result, ext2);
	}
	
	return result;
}

// Funckcja mówi czy dany wpis odpowiada za widoczny plik (Może on być też usunięty/wolny/pomocniczy)
bool fat_is_entry_visible(FENTRY *entry)
{
	if(entry->name[0] == FAT_ENTRY_FREE || entry->name[0] == FAT_ENTRY_DEL1 || entry->name[0] == FAT_ENTRY_DEL2) 
		return false;

	if(entry->attr & FAT_ATTR_SYSTEM || entry->attr & FAT_ATTR_VOLUME)
		return false;
		
	return true;
}

// Funkcja zwraca liczbę wszystkich wpisów w katalogu (usuniętych, wolnych, zajętych, ...)
uint32_t fat_get_entries_count(uint16_t dir_start)
{
    // Zwrócenie liczby wpisów w katalogu głównym
	if(dir_start == 0)
		return bs.bpb.entries_in_root_directory;

	// Zwrócenie liczby wpisów w podkatalogu
    uint32_t count = 0;
    for(uint32_t i=0; ; i++)
    {
        struct fat_directory_entry_t entry;
        int err = fat_get_entry_POS(dir_start, i, &entry);
        if(err) return 0;
        if(entry.name[0] == 0x00) return count;
        count++;
    }
}

// Funkcja zwraca liczbę widocznym wpisów w katalogu
uint32_t fat_get_visible_entries_count(uint16_t dir_start)
{
    // Pobranie liczby wszystkich wpisów w katalogu
    uint32_t entries_count = fat_get_entries_count(dir_start);

    // Zliczanie wpisów widocznych
    uint32_t count = 0;
    for(uint32_t i=0; i<entries_count; i++)
    {
        struct fat_directory_entry_t entry;
        int err = fat_get_entry_POS(dir_start, i, &entry);
        if(err) return 0;
        if(fat_is_entry_visible(&entry)) count++;
    }

    return count;
}

// Odczytywanie zawartości pliku
uint32_t fat_read_file(void *buffer, uint32_t start_cluster, uint32_t offset, uint32_t size)
{
    // Znalezienie pierwszego klastra do odczytu
	uint32_t current_cluster = start_cluster;
	while(offset >= FAT_CLUSTER_SIZE)
	{
	    int err = 0;
        current_cluster = fat_get_next_cluster(current_cluster, &err);
        if(err) return 0;
		offset -= FAT_CLUSTER_SIZE;
	}

	// Liczba bajtów którą udało się odczytać
	uint32_t bytes_read = 0;

	// Aktualnie czytany klaster
	uint8_t cluster[FAT_CLUSTER_SIZE];
	int res = disc_readblock(cluster, FAT_GET_DATA_ADDR(current_cluster)/SECTOR_SIZE, FAT_CLUSTER_SIZE/SECTOR_SIZE);
    if(res != FAT_CLUSTER_SIZE/SECTOR_SIZE) return bytes_read;

    // Odczytywanie żądanej liczby bajtów
	for(uint32_t i=0; i<size; i++)
    {
        ((uint8_t *)buffer)[i] = cluster[offset];
        offset++;
        bytes_read++;

        // Gdy wyjdziemy poza aktualny klaster odczytujemy kolejny
        if(offset >= FAT_CLUSTER_SIZE)
        {
            offset = 0;
            int err = 0;
            current_cluster = fat_get_next_cluster(current_cluster, &err);
            if(err) return bytes_read;
            int res = disc_readblock(cluster, FAT_GET_DATA_ADDR(current_cluster)/SECTOR_SIZE, FAT_CLUSTER_SIZE/SECTOR_SIZE);
            if(res != FAT_CLUSTER_SIZE/SECTOR_SIZE) return bytes_read;
        }
    }

	// Zwrócenie liczby odczytanych bajtów
	return bytes_read;
}

// Funkcja zwraca informacje o liczbie klastrów różnego typu
void fat_get_cluster_summary(uint32_t *free, uint32_t *use, uint32_t *bad, uint32_t *end)
{
    // Zerowanie wszystkich zmiennych
    if(free) *free = 0;
    if(use) *use = 0;
    if(bad) *bad = 0;
    if(end) *end = 0;

    // Obliczenie liczby komórek w tablicy fat
    uint32_t cells_count = bs.bpb.sectors_per_table * bs.bpb.bytes_per_sector / 2;

    // Iterowanie po komórkach talicy fat i klasyfikowanie klastrów
    for(uint32_t i=0; i<cells_count; i++)
    {
        // Pobranie komórki z tablicy fat
        uint16_t val = fat_read_table(0, i, NULL);

        // Inkrementacja odpowiednich zmiennych
        if(FAT_CLUSTER_IS_END(val) && val != FAT_CLUSTER_FREE && end) (*end)++;
        if(val == FAT_CLUSTER_BAD && bad) (*bad)++;
        if(val == FAT_CLUSTER_FREE && free) (*free)++;
        if((FAT_CLUSTER_IS_DATA(val) || (FAT_CLUSTER_IS_END(val) && val != FAT_CLUSTER_FREE)) && use) (*use)++;
    }
}

// Zwraca liczbę zajętych wpisów w katalogu głównym (w tym takich jak VOLUME i SYSTEM)
int fat_get_root_summary(uint32_t *free, uint32_t *used)
{
    // Sprawdzenie poprawności argumentów
    if(!free || !used) return 1;

    // Zerowanie zmiennych
    *free = 0;
    *used = 0;

    // Liczba wszystkich wpisów w katalogu głóœnym
    uint32_t entries_count = fat_get_entries_count(0);

    // Pobieranie kolejnych wpisów
    for(int i=0; i<entries_count; i++)
    {
        struct fat_directory_entry_t entry;
        int res = fat_get_entry_POS(0, i, &entry);
        if(res) return 1;

        // Zliczanie wolnych wpisów
        if(entry.name[0] == FAT_ENTRY_FREE || entry.name[0] == FAT_ENTRY_DEL1 || entry.name[0] == FAT_ENTRY_DEL2) (*free)++;
    }

    *used = entries_count - *free;
    return 0;
}

// Funkcja zwracająca boot sector
void fat_get_boot_sector(struct fat_boot_sector_t *boot_sector)
{
    *boot_sector = bs;
}

// Funkcja przeprowadza rozkodowanie czasu
struct tm fat_convert_time(uint16_t date, uint16_t time)
{
	struct tm res;
	
	res.tm_sec = (time & 0x001F) * 2;
	res.tm_min = (time & 0x07E0) >> 5;
	res.tm_hour = (time & 0xF800) >> 11;
	
	res.tm_mday = (date & 0x001F);
	res.tm_mon = ((date & 0x01E0) >> 5) - 1;
	res.tm_year = ((date & 0xFE00) >> 9) + 80;
	
	return res;
}

void fat_display_info(void)
{
	unsigned int temp = 0;
	
	printf("%-31s", "OEM name");
	display_ascii_line(bs.oem_name, 8);
	printf("\n");
		
	temp = bs.bpb.bytes_per_sector;
	printf("%-30s %u\n", "Bytes per sector", temp);
	
	temp = bs.bpb.sectors_per_cluster;
	printf("%-30s %u\n", "Sectors per cluster", temp);
	
	temp = bs.bpb.reserved_sectors;
	printf("%-30s %u\n", "Reserved sectors", temp);
	
	temp = bs.bpb.tables_count;
	printf("%-30s %u\n", "FAT table count", temp);
	
	temp = bs.bpb.entries_in_root_directory;
	printf("%-30s %u\n", "Max entries in root dir", temp);
	
	temp = bs.bpb.all_sectors_1;
	printf("%-30s %u\n", "All sectors count", temp);
	
	temp = bs.bpb.media_type;
	printf("%-30s %u\n", "Media type", temp);
	
	temp = bs.bpb.sectors_per_table;
	printf("%-30s %u\n", "Sectors count per FAT table", temp);
	
	temp = bs.bpb.sectors_per_path;
	printf("%-30s %u\n", "Sectors count per path", temp);
	
	temp = bs.bpb.paths_per_cylinder;
	printf("%-30s %u\n", "Paths count per cylinder", temp);
	
	temp = bs.bpb.hidden_sectors;
	printf("%-30s %u\n", "Hidden sectors count", temp);
	
	temp = bs.bpb.all_sectors_2;
	printf("%-30s %u\n", "All sectors count", temp);
	
	printf("%-31s", "Drive number");
	uint8_t drive_number = bs.ebpb.drive_number;
	printf("%d ", drive_number & !0x80);
	if(drive_number & 0x80) printf("(fixed)\n");
	else printf("(not fixed)\n");
	
	// Jeżeli to pole jest inne to następne są nieważne
	if(bs.ebpb.boot_signature != FAT_BOOT_SIGNATURE)
		return;
		
	printf("%-31s", "Volume label");
	display_ascii_line(bs.ebpb.volume_label, 11);
	printf("\n");
	
	printf("%-31s", "File system type");
	display_ascii_line(bs.ebpb.fs_type, 8);
	printf("\n");
}