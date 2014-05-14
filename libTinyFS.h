#ifndef __LIB_TINY_FS_H__
#define __LIB_TINY_FS_H__
#include <time.h>
#include "tinyFS.h"
#define ushort unsigned short

typedef int fileDescriptor;

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *filename);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);
int tfs_makeRO(char *name);
int tfs_makeRW(char *name);
int tfs_writeByte(fileDescriptor FD, unsigned char data);
int tfs_rename(char * oldname, char * newname);
int tfs_readdir();
char* tfs_readFileInfo(fileDescriptor FD);


typedef struct {
   char type;
   char magicNum;
   ushort freeList; //first free block
   ushort lastFree;
   ushort firstInode;
   ushort blockCount;
} SuperBlock;

typedef struct {
   char type;
   char magicNum;
   unsigned short startBlock;
   int size;
   char name[MAX_NAME_LEN];
   unsigned short blockNdx;
   ushort next;
	int readOnly;
   time_t time;
} Inode;

typedef struct {
   char type;
   char magicNum;
   ushort next;
   char data[DATA_SIZE];
} FileExtent;

typedef struct {
   char type;
   char magicNum;
   ushort next;
} Free;


typedef struct {
   Inode *inode;  // pointer to inode in memory
   int offset;    // offset to current location in file
   int open;      // whether or not the file descriptor is in use
} OpenFile;

typedef struct FileSystem {
   int disk;
   OpenFile *fdsTable;
   int fdsTableSize;      // size currently allocated for the fds table
   int fdsTableCount;     // used as an index for the last entry in the table
   SuperBlock sblock;
   Inode *inodes;
   char *validInode;
   int inodeCount;
   int freeCount;
	int inodeSize;
   char *timestampBuff;
} FileSystem;


char getBlockType(int disk, int bNum);
int getNextBlock(int disk, int bNum);
int findFreeBlock(SuperBlock *sblock, int disk);

void getSuperBlock(int disk, SuperBlock *superBlock);
void writeSuperBlock(int disk, unsigned short freeList, unsigned short firstInode, unsigned short lastFree, unsigned short blockCount);
void getInode(int disk, int bNum, Inode *inode);
void writeInode2(int disk, Inode *inode);
void writeInode(int disk, int bNum, int startBlock, int size, char * name, int next, time_t time);
void getFileExtent(int disk, int bNum, FileExtent *fileExtent);
void writeFileExtent(int disk, int bNum, int nextBlock, char *data, int dataSize);
void getFreeBlock(int disk, int bNum, Free *freeBlock);
void writeFreeBlock(int disk, int bNum, int nextBlock);

void pBlock(int blockNum);
void printFdTable();
void listBlocks();

#endif

