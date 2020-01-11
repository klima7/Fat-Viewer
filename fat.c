#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "disc.h"
#include "fat.h"
#include "utils.h"

int fat_version;                            // Wykorzystywana wersja FAT
struct fat_boot_sector_t bs;                // Dane z boot sectora
struct fat32_boot_sector_t *bs_view32;      // To samo co wyżej, ale nieco inna interpretacja pól
uint32_t *fat_table;                        // Tablica FAT

static uint32_t root_dir_addr;              // Adres katalogu głównego
static uint32_t alloc_area_addr;            // Adres obszaru z klastrami
static uint32_t fat_entries_count;          // Liczba wpisów w każdej tablicy fat
static uint32_t fat_cluster_size;

// Funkcja próbuje odczytać dysk za pomocą wszystkich obsługiwanych systemów plików
int fat_mount(void)
{
    int mount_res = 0;

    mount_res = fat_mount_version(FAT_32);
    if(mount_res == 0) return FAT_32;

    mount_res = fat_mount_version(FAT_16);
    if(mount_res == 0) return FAT_16;

    mount_res = fat_mount_version(FAT_12);
    if(mount_res == 0) return FAT_12;

    return 0;
}

// Montowanie
int fat_mount_version(int version)
{
    fat_version = version;

    // Odczytanie bootsectora
	int read_res = disc_readblock(&bs, 0, 1);
	if(read_res != 1) return 1;

	// ---------------Sprawdzenie poprawności bootsectora--------------------

    // Testy wspólne
    if (bs.boot_sector_end != FAT_END_SEQUENCE) return 1;
    if (!is_power_of_two(bs.bpb.bytes_per_sector)) return 1;
    if (!is_power_of_two(bs.bpb.sectors_per_cluster) || bs.bpb.sectors_per_cluster > 128) return 1;
    if (bs.bpb.reserved_sectors < 1) return 1;
    if (bs.bpb.tables_count != 1 && bs.bpb.tables_count != 2) return 1;

    // Testy charakterystyczne dla FAT12 i FAT16
	if(version != FAT_32)
	{
        if(bs.bpb.entries_in_root_directory == 0 || (bs.bpb.entries_in_root_directory * FAT_ENTRY_SIZE) % bs.bpb.bytes_per_sector != 0) return 1;
        if(bs.bpb.all_sectors_1 == 0 || bs.bpb.all_sectors_2 != 0) return 1;

        if(fat_version==FAT_12)
        {
            if(bs.bpb.sectors_per_table < 1 || bs.bpb.sectors_per_table > 12) return 1;
        }
        if(fat_version==FAT_16)
        {
            if(bs.bpb.sectors_per_table < 16 || bs.bpb.sectors_per_table > 256) return 1;
        }
    }

	// Testy charakterystyczne dla FAT32
    else if(version == FAT_32)
    {
        bs_view32 = (struct fat32_boot_sector_t*)&bs;
        if(bs.bpb.entries_in_root_directory != 0) return 1;
        if(bs.bpb.sectors_per_table != 0) return 1;
        if(bs.bpb.all_sectors_1 != 0 || bs.bpb.all_sectors_2 == 0) return 1;
        if(bs_view32->ebpb32.root_dir == 0) return 1;

        if(bs_view32->ebpb32.sectors_per_table < 512 || bs_view32->ebpb32.sectors_per_table > 2097152) return 1;
    }

    // -----------Obliczenie często wykorzystywanych wartości--------------
    fat_cluster_size = bs.bpb.sectors_per_cluster * bs.bpb.bytes_per_sector;

	if(version != FAT_32)
	{
        root_dir_addr =(bs.bpb.reserved_sectors + bs.bpb.sectors_per_table * bs.bpb.tables_count) * bs.bpb.bytes_per_sector;
        alloc_area_addr = root_dir_addr + bs.bpb.entries_in_root_directory * FAT_ENTRY_SIZE;
        if(version == FAT_16) fat_entries_count = bs.bpb.sectors_per_table * bs.bpb.bytes_per_sector / 2;
        else if(version == FAT_12) fat_entries_count = bs.bpb.sectors_per_table * bs.bpb.bytes_per_sector  / 3 * 2;
    }

    else if(version == FAT_32)
    {
        root_dir_addr = bs_view32->ebpb32.root_dir;
        alloc_area_addr = (bs.bpb.reserved_sectors + bs_view32->ebpb32.sectors_per_table * bs.bpb.tables_count) * bs.bpb.bytes_per_sector;
        fat_entries_count = bs_view32->ebpb32.sectors_per_table * bs.bpb.bytes_per_sector / 4;
    }

    // ------------------------Odczyt tablic FAT--------------------------
	uint32_t fat_size = 0;
	if(version == FAT_16) fat_size = fat_entries_count*2;
	else if(version == FAT_12) fat_size = fat_entries_count+fat_entries_count/2;
    else if(version == FAT_32) fat_size = fat_entries_count+fat_entries_count*4;

    uint8_t fat[bs.bpb.tables_count][fat_size];

	for(uint8_t i=0; i<bs.bpb.tables_count; i++)
    {
        uint32_t fat_start = (bs.bpb.reserved_sectors + bs.bpb.sectors_per_table * i) * bs.bpb.bytes_per_sector;

        // Obliczenie na których sektorach dyskowych znajduje się tablica fat
        uint32_t fat_start_disc_sector_number = fat_start / DISC_SECTOR_SIZE;
        uint32_t fat_start_disc_sector_offset = fat_start % DISC_SECTOR_SIZE;
        uint32_t fat_size_in_disc_sectors = (fat_size+fat_start_disc_sector_offset)/DISC_SECTOR_SIZE;
        if((fat_size+fat_start_disc_sector_offset)%DISC_SECTOR_SIZE != 0) fat_size_in_disc_sectors++;

        // Odczyt sektorów dyskowych z zawartościa tablicy fat
        uint8_t buffer[fat_size_in_disc_sectors*DISC_SECTOR_SIZE];
        read_res = disc_readblock(buffer, fat_start_disc_sector_number, fat_size_in_disc_sectors);
        if(read_res != fat_size_in_disc_sectors) return 1;

        memcpy(fat[i], buffer+fat_start_disc_sector_offset, fat_size);
    }

    // ------------------------Porównanie zawartości tablic FAT--------------------------
	if(bs.bpb.tables_count == 2)
	{
        for (uint32_t i = 0; i < fat_size; i++)
        {
            if (fat[0][i] != fat[1][i]) return 1;
        }
    }

    // ------------------------Zachowanie pierwszej tablicy FAT--------------------------
    fat_table = (uint32_t *)malloc(fat_entries_count*4);
	if(fat_table == NULL) return 1;

    if(version == FAT_16)
    {
        for(int i=0; i<fat_entries_count; i++)
        {
            fat_table[i] = ((uint16_t *) fat[0])[i];
        }
    }
    if(version == FAT_12)
    {
        for(int i=0; i<fat_entries_count; i++)
        {
            uint32_t group_addr = i/2*3;
            uint8_t three[3] = { fat[0][group_addr+0], fat[0][group_addr+1], fat[0][group_addr+2]};
            if(i%2==0)
                fat_table[i] = three[0] | (three[1] & 0x0F) << 8;
            else
                fat_table[i] = (three[1] & 0xF0) >> 4 | three[2] << 4;
        }
    }
    if(version == FAT_32)
    {
        for(int i=0; i<fat_entries_count; i++)
        {
            fat_table[i] = ((uint32_t *) fat[0])[i];
        }
    }

    // Wyświetlenie danych z bootsectora
    fat_display_info();
    printf("\n");

	return 0;
}

// Czy klaster jest kończący
int fat_is_end_cluster(uint32_t value)
{
    if(fat_version == FAT_16) return (value >= 0xFFF8 || value == 0);
    else if(fat_version == FAT_12) return (value >= 0xFF8 || value == 0);
    else if(fat_version == FAT_32) return (value >= 0x0FFFFFF8 || value == 0);
}

// Czy klaster jest wolny
int fat_is_free_cluster(uint32_t value)
{
    return value == 0;
}

// Czy klaster jest nieprawidłowy
int fat_is_bad_cluster(uint32_t value)
{
    if(fat_version == FAT_16) return value == 0xFFF7;
    else if(fat_version == FAT_12) return value == 0xFF7;
    else if(fat_version == FAT_32) return value & FAT32_MASK == 0x0FFFFFF7;
}

uint32_t fat_get_data_addr(uint32_t cluster)
{
    return alloc_area_addr + (cluster - 2) * fat_cluster_size;
}

// funkcja zwraca numer następnego klastra lub 0 jeśli kolejny klaster nie istnieje
uint16_t fat_get_next_cluster(uint16_t cluster)
{
    return fat_table[cluster];
}

// Funkcja zwraca długość łańcucha klastrów rozpoczynającego się w klastrze number start
uint32_t fat_get_chain_length(uint32_t start)
{
    // Długość łąńcucha klastrów i bieżący numer klastra
    uint32_t count = 0;
    uint32_t current = start;

    // Odczytywanie numberów kolejnych klastrów, aż do klastra kończącego
    while(!fat_is_end_cluster(current))
    {
        count++;
        current = fat_table[current];
    }

    // Zwrócenie liczby klastrów
    return count;
}

// Funkcja znajdująca wpis w katalogu rozpoczynającym się w klastrze dir_start odpowiadający za plik o nazwie name
int fat_get_entry_by_name(uint32_t dir_start, const char *name, struct fat_directory_entry_t *res_entry)
{
    // Znalezienie liczby wpisów w katalogu
    uint32_t entries_count = fat_get_entries_count(dir_start);

    // Przechodzenie wszystkich wpisów
    for(uint32_t i=0; i<entries_count; i++)
    {
        // Pobranie wpisu
        struct fat_directory_entry_t entry;
        int res = fat_get_entry_by_pos(dir_start, i, &entry);
        if(res || !fat_is_entry_visible(&entry)) continue;

        // Znalezienie nazwy
        char filename[257];
        res = fat_get_long_filename(dir_start, i, filename);
        if(res) fat_join_filename((char*)entry.name, (char*)entry.ext, filename);

        // Udało się znaleźć wpis
        if(strcmp(name, filename) == 0)
        {
            *res_entry = entry;
            return 0;
        }
    }
    // Nie udało się znaleźć wpisu
    return 1;
}

// Funkcja zwracająca wpis w katalogu odpowiadający za plik wskazywany przez ścieżkę path
int fat_get_entry_by_path(const char *path, struct fat_directory_entry_t *res_entry)
{
    // Rozpoczęcie dzielenia ścieżki na tokeny
    char *tokens = strdup(path);
    char *token = strtok(tokens, "/");
    if(token == NULL) return 1;

    // Rozpoczęcie poszukiwanie od katalogu głównego
    uint32_t current_dir = fat_version == FAT_32 ? bs_view32->ebpb32.root_dir : 0;
    bool is_directory = true;

    struct fat_directory_entry_t entry;

    // Iterowanie po tokenach i przechodzenie do kolejnych katalogów
    while(token != NULL)
    {
        // Jeżeli w środku ścieżki znajduje się plik nie będący katalogiem to zgłoś błąd
        if(!is_directory) { free(tokens); return 1; }

        // Znalezienie wpisu w aktualnym katalogu
        int res = fat_get_entry_by_name(current_dir, token, &entry);
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
int fat_get_entry_by_pos(uint32_t dir_start, uint32_t dir_pos, struct fat_directory_entry_t *res_entry)
{
    // Przeszukiwanym katalogiem jest katalog główny w fat12 lub fat16
    if(dir_start == 0)
    {
        // Znalezienie sektora z wpisem
        uint32_t entry_addr = root_dir_addr + dir_pos * FAT_ENTRY_SIZE;
        uint32_t sector_number = entry_addr / DISC_SECTOR_SIZE;
        uint32_t sector_offset = entry_addr % DISC_SECTOR_SIZE;

        // Wczytanie zawartości sektora
        uint8_t sector[DISC_SECTOR_SIZE];
        int res = disc_readblock(sector, sector_number, 1);
        if(res != 1) return 1;

        // Zwrócenie znalezionego wpisu
        struct fat_directory_entry_t *entry = (struct fat_directory_entry_t *) (sector + sector_offset);
        *res_entry = *entry;
        return 0;
    }

    // Przeszukiwanym katalogiem jest jakiś podkatalog (lub katalog gówny w fat32)
   else
    {
        // Obliczenie przesunięcia od początku katalogu
        uint32_t entry_offset = dir_pos * FAT_ENTRY_SIZE;

        // Znalezienie klastra z wpisem
        uint32_t current_cluster = dir_start;
        while (entry_offset >= fat_cluster_size) {
            current_cluster = fat_table[current_cluster];
            entry_offset -= fat_cluster_size;
        }

        // Odczyt odpowiedniego klastra
        uint8_t cluster[fat_cluster_size];
        int res = disc_readblock(cluster, fat_get_data_addr(current_cluster) / DISC_SECTOR_SIZE, fat_cluster_size / DISC_SECTOR_SIZE);
        if (res != fat_cluster_size / DISC_SECTOR_SIZE) return 1;

        // Zwrócenie znalezionego wpisu
        struct fat_directory_entry_t *entry = (struct fat_directory_entry_t *) (cluster + entry_offset);
        *res_entry = *entry;
        return 0;
    }
}

// Funkcja zwraca długą nazwę dla krótkiego wpisu znajdującego się na danej pozycji
int fat_get_long_filename(uint32_t dir_start, uint32_t dir_pos, char *long_filename)
{
    *long_filename = 0;

    // Zmienne wykorzystywane do składanie długiej nazwy
    char buffer[530];
    int pos = 0;

    if(dir_pos == 0) return 1;

    // Składanie długiej nazwy z maksymalnie dwudziestu fragmentów od ostatniego fizycznie do pierwszego fizycznie
    for(int32_t i=dir_pos-1; i>=0 && i+21>dir_pos; i--)
    {
        // Pobranie wpisu który może być fragmentem długiej nazwy
        struct fat_long_name_directory_entry_t long_entry;
        int res = fat_get_entry_by_pos(dir_start, i, (struct fat_directory_entry_t *) &long_entry);
        if(res != 0) return 1;

        // Sprawdzenie czy wpis jest wpisem długiej nazwy
        if(long_entry.attr != FAT_LONG_NAME_ATTR || long_entry.file_start != FAT_LONG_NAME_CLUSTER || long_entry.type != FAT_LONG_NAME_TYPE)
            return 1;

        // Wklejenie poszczególnych części do buforu
        memcpy(buffer+pos, long_entry.name1, 10);
        pos += 10;

        memcpy(buffer+pos, long_entry.name2, 12);
        pos += 12;

        memcpy(buffer+pos, long_entry.name3, 4);
        pos += 4;

        // Sprawdzenie czy wklejona część była ostatnia
        if(long_entry.number & FAT_LONG_NAME_LAST) break;
    }

    // Wybieranie co drugiego znaku by dostać napis ascii
    for(int i=0; i<pos && i/2<256; i+=2)
        long_filename[i/2] = buffer[i];
    long_filename[256] = 0;

    return 0;
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

// Funckcja mówi czy dany wpis odpowiada za widoczny plik (Może on być też usunięty/wolny/...)
bool fat_is_entry_visible(FENTRY *entry)
{
	if(entry->name[0] == FAT_ENTRY_FREE || entry->name[0] == FAT_ENTRY_DEL1 || entry->name[0] == FAT_ENTRY_DEL2)
		return false;

	if(entry->attr & FAT_ATTR_SYSTEM || entry->attr & FAT_ATTR_VOLUME)
		return false;

	return true;
}

// Funkcja zwraca liczbę wszystkich wpisów w katalogu (usuniętych, wolnych, zajętych, ...)
uint32_t fat_get_entries_count(uint32_t dir_start)
{
    // Zwrócenie liczby wpisów w katalogu głównym w fat12 i fat16
	if(dir_start == 0)
		return bs.bpb.entries_in_root_directory;

	// Zwrócenie liczby wpisów w podkataloguw fat12 i fat16 (lub w katalogu głównym w fat32)
    uint32_t count = 0;
    for(uint32_t i=0; ; i++)
    {
        struct fat_directory_entry_t entry;
        int err = fat_get_entry_by_pos(dir_start, i, &entry);
        if(err) return 0;
        if(entry.name[0] == 0x00) return count;
        count++;
    }
}

// Odczytywanie zawartości pliku
uint32_t fat_read_file(void *buffer, uint32_t start_cluster, uint32_t offset, uint32_t size)
{
    // Znalezienie pierwszego klastra do odczytu
	uint32_t current_cluster = start_cluster;
	while(offset >= fat_cluster_size)
	{
        current_cluster = fat_table[current_cluster];
		offset -= fat_cluster_size;
	}

	// Liczba bajtów którą udało się odczytać
	uint32_t bytes_read = 0;

	// Aktualnie czytany klaster
	uint8_t cluster[fat_cluster_size];
	int res = disc_readblock(cluster, fat_get_data_addr(current_cluster) / DISC_SECTOR_SIZE, fat_cluster_size / DISC_SECTOR_SIZE);
    if(res != fat_cluster_size / DISC_SECTOR_SIZE) return bytes_read;

    // Odczytywanie żądanej liczby bajtów
	for(uint32_t i=0; i<size; i++)
    {
        ((uint8_t *)buffer)[i] = cluster[offset];
        offset++;
        bytes_read++;

        // Gdy wyjdziemy poza aktualny klaster odczytujemy kolejny
        if(offset >= fat_cluster_size)
        {
            offset = 0;
            current_cluster = fat_table[current_cluster];
            int res = disc_readblock(cluster, fat_get_data_addr(current_cluster) / DISC_SECTOR_SIZE, fat_cluster_size / DISC_SECTOR_SIZE);
            if(res != fat_cluster_size / DISC_SECTOR_SIZE) return bytes_read;
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

    // Iterowanie po komórkach talicy fat i klasyfikowanie klastrów
    for(uint32_t i=0; i<fat_entries_count; i++)
    {
        // Pobranie komórki z tablicy fat
        uint32_t val = fat_table[i];

        // Inkrementacja odpowiednich zmiennych
        if(fat_is_end_cluster(val) && !fat_is_free_cluster(val) && end) (*end)++;
        if(fat_is_bad_cluster(val) && bad) (*bad)++;
        if(fat_is_free_cluster(val) && free) (*free)++;
        else (*use)++;
    }
}

// Zwraca liczbę zajętych wpisów w katalogu głównym (w tym takich jak VOLUME i SYSTEM)
int fat_get_root_summary(uint32_t *free, uint32_t *used)
{
    // Sprawdzenie poprawności argumentów
    if(!free || !used || fat_version == FAT_32) return 1;

    // Zerowanie zmiennych
    *free = 0;
    *used = 0;

    // Pobieranie kolejnych wpisów
    for(int i=0; i<bs.bpb.entries_in_root_directory; i++)
    {
        struct fat_directory_entry_t entry;
        int res = fat_get_entry_by_pos(0, i, &entry);
        if(res) return 1;

        // Zliczanie wolnych wpisów
        if(entry.name[0] == FAT_ENTRY_FREE || entry.name[0] == FAT_ENTRY_DEL1 || entry.name[0] == FAT_ENTRY_DEL2) (*free)++;
    }

    *used = bs.bpb.entries_in_root_directory - *free;
    return 0;
}

uint32_t fat_get_root_dir_addr(void)
{
    if(fat_version == FAT_32) return bs_view32->ebpb32.root_dir;
    else return 0;
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
    printf("=============BOOTSECTOR INFO=============\n");

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
    printf("%-30s %u\n", "All sectors count 1", temp);

    temp = bs.bpb.all_sectors_2;
    printf("%-30s %u\n", "All sectors count 2", temp);

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

    printf("%-31s", "Drive number");
    uint8_t drive_number = bs.ebpb.drive_number;
    printf("%d ", drive_number & !0x80);
    if(drive_number & 0x80) printf("(fixed)\n");
    else printf("(not fixed)\n");

    if(fat_version != FAT_32)
    {
        // Jeżeli to pole jest inne to następne są nieważne
        if (bs.ebpb.boot_signature != FAT_BOOT_SIGNATURE)
            return;

        printf("%-31s", "Volume label");
        display_ascii_line(bs.ebpb.volume_label, 11);
        printf("\n");

        printf("%-31s", "File system type");
        display_ascii_line(bs.ebpb.fs_type, 8);
        printf("\n");
    }

    if(fat_version == FAT_32)
    {
        temp = bs_view32->ebpb32.sectors_per_table;
        printf("%-30s %u\n", "FAT32 sectors per table", temp);

        temp = bs_view32->ebpb32.root_dir;
        printf("%-30s %u\n", "FAT32 root cluster", temp);

        temp = bs_view32->ebpb32.fs_sector;
        printf("%-30s %u\n", "FAT32 fs sector", temp);

        temp = bs_view32->ebpb32.copy_addr;
        printf("%-30s %u\n", "FAT32 Backup sector", temp);

        // Jeżeli to pole jest inne to następne są nieważne
        if (bs_view32->ebpb.boot_signature != FAT_BOOT_SIGNATURE)
            return;

        printf("%-31s", "Volume label");
        display_ascii_line(bs_view32->ebpb.volume_label, 11);
        printf("\n");

        printf("%-31s", "File system type");
        display_ascii_line(bs_view32->ebpb.fs_type, 8);
        printf("\n");
    }
}