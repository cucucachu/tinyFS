#ifndef __TINY_FS_H__
#define __TINY_FS_H__
/* The default size of the disk and file system block */
#define BLOCKSIZE 256
/* Your program should use a 10240 Byte disk size giving you 40 blocks total. This is 
 * a default size. You must be able to support different possible values */
#define DEFAULT_DISK_SIZE 10240
/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME "tinyFSDisk"


#define SUPER 1
#define INODE 2
#define FILE_EXTENT 3
#define FREE 4

#define DATA_SIZE 250
#define MAGIC_NUM 69
#define MAX_NAME_LEN 8

#endif
