#ifndef __FAT_H__
#define __FAT_H__

#include <stdint.h>

#define FAT_END_SEQUENCE	    0xAA55
#define FAT_BOOT_SIGNATURE	    0x29
#define FAT_ENTRY_SIZE		    32

#define FAT_ENTRY_FREE	        0x00
#define FAT_ENTRY_DEL1	        0x05
#define FAT_ENTRY_DEL2	        0xE5
#define FAT_ENTRY_DOTS	        0x2E

#define FAT_LONG_NAME_CLUSTER   0x0000
#define FAT_LONG_NAME_ATTR      0x0F
#define FAT_LONG_NAME_TYPE      0x00
#define FAT_LONG_NAME_LAST      0x40
#define FAT_LONG_NAME_NUMBER    0x1F

#define FAT_ATTR_READ_ONLY 		0x01
#define FAT_ATTR_HIDDEN 		0x02
#define FAT_ATTR_SYSTEM 		0x04
#define FAT_ATTR_VOLUME 		0x08
#define FAT_ATTR_SUBDIRECTORY 	0x10
#define FAT_ATTR_ARCHIVE 		0x20
#define FAT_ATTR_DEVICE 		0x40

#define FAT_CLUSTER_SIZE 		        (bs.bpb.sectors_per_cluster * bs.bpb.bytes_per_sector)
#define FAT_GET_DATA_ADDR(__CLUSTER)    (alloc_area_addr + ((__CLUSTER) - 2) * FAT_CLUSTER_SIZE)

#define FAT_CLUSTER_IS_END(__NUM) 		((__NUM)==0 || (__NUM)>0xFFF8)
#define FAT_CLUSTER_IS_DATA(__NUM)		((__NUM)>=0x0002 && (__NUM)<0xFFEF)
#define FAT_CLUSTER_FREE				0x0000
#define FAT_CLUSTER_BAD					0xFFF5

#define FAT_SECTOR_SIZE                 bs.bpb.bytes_per_sector

struct fat_bpb_t
{
	uint16_t bytes_per_sector;				// 0x00B
	uint8_t sectors_per_cluster;			// 0x00D
	uint16_t reserved_sectors;				// 0x00E
	uint8_t tables_count;					// 0x010
	uint16_t entries_in_root_directory;		// 0x011
	uint16_t all_sectors_1;					// 0x013
	uint8_t media_type;						// 0x015
	uint16_t sectors_per_table;				// 0x016
	uint16_t sectors_per_path;				// 0x018
	uint16_t paths_per_cylinder;			// 0x01A
	uint32_t hidden_sectors;				// 0x01C
	uint32_t all_sectors_2;					// 0x020
} __attribute__((packed));

struct fat_ebpb_t
{
	uint8_t drive_number;					// 0x024
	uint8_t reserved;						// 0x025
	uint8_t boot_signature;					// 0x026
	uint32_t volume_id;						// 0x027
	uint8_t volume_label[11];				// 0x02B
	uint8_t fs_type[8];						// 0x036
} __attribute__((packed));

struct fat_boot_sector_t
{
	uint8_t jump_addr[3];					// 0x000
	uint8_t oem_name[8];					// 0x003
	struct fat_bpb_t bpb;					// 0x00B
	struct fat_ebpb_t ebpb;  				// 0x024
	uint8_t loader_code[448];				// 0x03E
	uint16_t boot_sector_end;				// 0x1FE
} __attribute__((packed));

struct fat_directory_entry_t
{
	char name[8];						    // 0x00
	char ext[3];							// 0x08
	uint8_t attr;							// 0x0B
	uint8_t specific1;						// 0x0C
	uint8_t specific2;						// 0x0D
	uint16_t create_time;					// 0x0E
	uint16_t create_date;					// 0x10
	uint16_t access_date;					// 0x12
	uint16_t idk1;							// 0x14
	uint16_t modify_time;					// 0x16
	uint16_t modify_date;					// 0x18
	uint16_t file_start;					// 0x1A
	uint32_t file_size;						// 0x1C
} __attribute__((packed));

struct fat_long_name_directory_entry_t
{
    uint8_t number;                         // 0x00
    uint16_t name1[5];                      // 0x01
    uint8_t attr;                           // 0x0B
    uint8_t type;                           // 0x0C
    uint8_t checksum;                       // 0x0D
    uint16_t name2[6];                      // 0x0E
    uint16_t file_start;                    // 0x1A
    uint16_t name3[2];                      // 0x1C

} __attribute__((packed));

typedef struct fat_directory_entry_t FENTRY;

int fat_init(void);
uint32_t fat_get_chain_length(uint16_t start);
uint32_t fat_get_entries_count(uint16_t dir_start);
bool fat_is_entry_visible(struct fat_directory_entry_t *entry);
struct tm fat_convert_time(uint16_t date, uint16_t time);
char *fat_join_filename(const char *name, const char *ext, char *result);
uint32_t fat_read_file(void *buffer, uint32_t start_cluster, uint32_t offset, uint32_t size);
int fat_get_entry_by_path(const char *path, struct fat_directory_entry_t *res_entry);
int fat_get_entry_by_pos(uint16_t dir_start, uint32_t dir_pos, struct fat_directory_entry_t *res_entry);
void fat_get_cluster_summary(uint32_t *free, uint32_t *use, uint32_t *bad, uint32_t *end);
int fat_get_root_summary(uint32_t *free, uint32_t *used);
void fat_get_boot_sector(struct fat_boot_sector_t *boot_sector);
int fat_get_long_filename(uint16_t dir_start, uint32_t dir_pos, char *long_filename);
uint16_t fat_get_next_cluster(uint16_t cluster);
void fat_display_info(void);

#endif