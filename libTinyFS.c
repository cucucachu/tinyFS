#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include "libTinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"
#include "tinyFS.h"

#define INIT_FILE_TABLE_SIZE 1 

static FileSystem *fs;
static int tfs_errno = 0;

static int checkFD(fileDescriptor FD);
static int min(int a, int b);

int tfs_mkfs(char *filename, int nBytes) {
   int disk, blockNdx;

	if (nBytes < 0) {
		tfs_SetErrno(BAD_DISK_SIZE_ERROR);
		return BAD_DISK_SIZE_ERROR;
	}

   disk = openDisk(filename, nBytes);

   if (disk < 0)
      return disk;
	
	if (nBytes == 0)
		return disk;

	writeSuperBlock(disk, 0x1, 0x0, nBytes / BLOCKSIZE - 1, nBytes / BLOCKSIZE);

   
   for (blockNdx = 1; blockNdx < nBytes / BLOCKSIZE - 1; blockNdx++)
      writeFreeBlock(disk, blockNdx, blockNdx + 1);
   writeFreeBlock(disk, blockNdx, 0);
   return disk;
}

int tfs_mount(char *filename) {
   int blockNdx;
   unsigned int size;
   char type;

   if (fs)
      tfs_unmount();

   fs = (FileSystem *)calloc(sizeof(FileSystem), 1);

   fs->inodeCount = 0;
   fs->disk = open(filename, O_RDWR);
   fs->freeCount = 0;

   if (fs->disk < 0) {
      tfs_SetErrno(DISK_OPEN_ERROR);
		free(fs);
		fs = NULL;
		return DISK_OPEN_ERROR;
	}
   
	size = lseek(fs->disk, 0L, SEEK_END);

   fs->fdsTableSize = INIT_FILE_TABLE_SIZE;
   fs->fdsTableCount = 0;
   fs->fdsTable = (OpenFile *)malloc(INIT_FILE_TABLE_SIZE * sizeof(OpenFile));

   getSuperBlock(fs->disk, &fs->sblock);

	if (fs->sblock.magicNum != MAGIC_NUM) {
		tfs_SetErrno(CORRUPT_DISK_ERROR);
		free(fs->fdsTable);
		free(fs);
		fs = NULL;
		return CORRUPT_DISK_ERROR;
	}

	fs->inodeSize = size / BLOCKSIZE;
   fs->inodes = (Inode*)malloc(sizeof(Inode) * fs->inodeSize);
   fs->validInode = (char*)calloc(sizeof(char), fs->inodeSize);
   
   for (blockNdx = 1; blockNdx < size / BLOCKSIZE; blockNdx++) {
      type = getBlockType(fs->disk, blockNdx);
      switch (type) {
         case INODE:
            getInode(fs->disk, blockNdx, &(fs->inodes[fs->inodeCount]));
            fs->validInode[fs->inodeCount] = 1;
            fs->inodeCount++;
            break;
         case FREE:
            fs->freeCount++;
            break;
         case FILE_EXTENT:
            break;
         default:
            tfs_SetErrno(CORRUPT_DISK_ERROR);
				free(fs->fdsTable);
				free(fs->inodes);
				free(fs->validInode);
				free(fs);
				fs = NULL;
            return CORRUPT_DISK_ERROR;
      }
   } 

   return 0;   // PROBABLY NEED TO REMOVE THIS, BUT RIGHT NOW I'M USING IT FOR DRIVER
}

int tfs_unmount(void) {
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }
   free(fs->inodes);
   free(fs->fdsTable);
   free(fs->timestampBuff);
   free(fs->validInode);
   free(fs);
   fs = NULL;
   return 0;
}

fileDescriptor tfs_openFile(char *name) {
   int i, FD, bNum;
   Free freeBlock;

   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }

	if (fs->sblock.blockCount == 0) {
		tfs_SetErrno(OUT_OF_SPACE_ERROR);
		return OUT_OF_SPACE_ERROR;
	}

   if (strlen(name) > MAX_NAME_LEN) {
      tfs_SetErrno(INVALID_FILENAME_LENGTH);
      return INVALID_FILENAME_LENGTH;
   }

   // find the inode if it exists
   for (i = 0; i < fs->inodeCount; i++)
      if (fs->validInode[i] && strcmp(fs->inodes[i].name, name) == 0)
         break;
   
   if (fs->fdsTableCount >= fs->fdsTableSize) {
      fs->fdsTableSize *= 2;
      fs->fdsTable = (OpenFile *)realloc(fs->fdsTable, fs->fdsTableSize * sizeof(OpenFile));
   }

   // create an inode if the file doesn't exist
   if (i == fs->inodeCount) {
      if (!fs->freeCount) {
         tfs_SetErrno(OUT_OF_SPACE_ERROR);
			return OUT_OF_SPACE_ERROR;
      }
      
      // write the inode to disk
      bNum = findFreeBlock(&fs->sblock, fs->disk);
      getFreeBlock(fs->disk, bNum, &freeBlock);

      fs->inodes[i].time = time(NULL);
      writeInode(fs->disk, bNum, 0, 0, name, 0, fs->inodes[i].time);

      // update the superblock in memory
      fs->sblock.freeList = freeBlock.next;

      // update the superblock on disk
      writeSuperBlock(fs->disk, fs->sblock.freeList, fs->sblock.firstInode, fs->sblock.lastFree, fs->sblock.blockCount);

      // update inode in memory
      fs->inodes[i].type = INODE;
      fs->inodes[i].magicNum = MAGIC_NUM;
      fs->inodes[i].startBlock = 0;
      fs->inodes[i].size = 0;
		fs->inodes[i].readOnly = 0;
      strcpy(fs->inodes[i].name, name);
      fs->inodes[i].blockNdx = bNum;
      fs->inodes[i].next = 0;     
      fs->inodeCount++;

      fs->freeCount--;
   }

   fs->fdsTable[fs->fdsTableCount].inode = &fs->inodes[i];
   fs->fdsTable[fs->fdsTableCount].open = 1;
   fs->fdsTable[fs->fdsTableCount].offset = 0;
   fs->validInode[i] = 1;
   FD = fs->fdsTableCount++;
   
	if (fs->inodeCount == fs->inodeSize) {
		fs->inodes = (Inode*)realloc(fs->inodes, sizeof(Inode) * fs->inodeSize * 2);
		fs->validInode = (char *)realloc(fs->validInode, sizeof(char) * fs->inodeSize * 2);
		fs->inodeSize *= 2;
	}

	return FD;
}

int tfs_closeFile(fileDescriptor FD) {
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }

   int fderror = checkFD(FD);
   
   if (fderror)
      return fderror;

   fs->fdsTable[FD].open = 0;

   return 0;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
   int copied = 0, tocopy, left, nextBlock, blocksToWrite, extentCount, 
      bNum, freeNdx, startBlock, error, nextNum;
   Inode * inode;
   Free freeBlock;
   
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }

   error = checkFD(FD);

   if (error)
      return error;

   inode = fs->fdsTable[FD].inode;

   if (inode->readOnly) {
      tfs_SetErrno(READ_ONLY_ERROR);
      return READ_ONLY_ERROR;
   }
   
   if (size <= 0 || buffer == NULL) {
		tfs_SetErrno(BAD_ARGS_ERROR);
		return BAD_ARGS_ERROR;
	}

	blocksToWrite = size / DATA_SIZE + (size % DATA_SIZE ? 1 : 0);
   extentCount = inode->size / DATA_SIZE + (inode->size % DATA_SIZE ? 1 : 0);
   if (blocksToWrite > fs->freeCount + extentCount) {
      tfs_SetErrno(OUT_OF_SPACE_ERROR);
      return OUT_OF_SPACE_ERROR;
   }
   inode->time = time(NULL);
   // Write extent to freeblocks
   bNum = inode->startBlock;
   while (bNum) {
      nextNum = getNextBlock(fs->disk, bNum);

		if (fs->sblock.lastFree)
      	writeFreeBlock(fs->disk, fs->sblock.lastFree, bNum);
      fs->sblock.lastFree = bNum;
      writeFreeBlock(fs->disk, bNum, nextNum);
      if (!nextNum) {
         fs->sblock.lastFree = bNum;
      }
      bNum = nextNum;
   }
   if (!fs->sblock.freeList) {
      fs->sblock.freeList = inode->startBlock;
   }
   writeSuperBlock(fs->disk, fs->sblock.freeList, fs->sblock.firstInode, fs->sblock.lastFree, fs->sblock.blockCount);

   // Write the file extent blocks
   startBlock = freeNdx = findFreeBlock(&fs->sblock, fs->disk);
   while (copied < size) {
      left = size - copied;

      if (freeNdx > 0) {
         getFreeBlock(fs->disk, freeNdx, &freeBlock);
      }
      
		nextBlock = left > DATA_SIZE ? freeBlock.next : 0;
      tocopy = min(left, DATA_SIZE);
      writeFileExtent(fs->disk, freeNdx ? freeNdx : startBlock, nextBlock, &buffer[copied], tocopy);
      //fs->sblock.freeList = freeBlock.next ? freeBlock.next : 0;
      copied += tocopy;
      if (copied < size)
         freeNdx = findFreeBlock(&fs->sblock, fs->disk);
   }

   writeSuperBlock(fs->disk, fs->sblock.freeList, fs->sblock.firstInode, fs->sblock.lastFree, fs->sblock.blockCount);

   inode->size = size;
   inode->startBlock = startBlock;
   writeInode(fs->disk, inode->blockNdx, startBlock, size, inode->name, 0, inode->time);

   fs->fdsTable[FD].offset = 0;
   fs->freeCount += extentCount - blocksToWrite;

   return 0;
}

int tfs_deleteFile(fileDescriptor FD) {
   OpenFile *fd;
   Inode *inode;
   int bNum, nextNum, i;
   int error;

   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }

   error = checkFD(FD);

   if (error)
      return error;

   fd = &fs->fdsTable[FD];

   inode = fd->inode;

   if (inode->readOnly) {
      tfs_SetErrno(READ_ONLY_ERROR);
      return READ_ONLY_ERROR;
   }
   // Make the inode block free
   bNum = inode->startBlock;  // bNum is 0 if the file is empty, so next will be 0 which is what we want

   if (!fs->sblock.freeList)
      fs->sblock.freeList = inode->blockNdx;

   if (fs->sblock.lastFree)
      writeFreeBlock(fs->disk, fs->sblock.lastFree, inode->blockNdx);

   writeFreeBlock(fs->disk, inode->blockNdx, bNum);
   fs->sblock.lastFree = inode->blockNdx;
   fs->freeCount++;

   // Go until there isn't a next block
   bNum = inode->startBlock;
   while (bNum) {
      nextNum = getNextBlock(fs->disk, bNum);

      writeFreeBlock(fs->disk, fs->sblock.lastFree, bNum);
      fs->sblock.lastFree = bNum;
      writeFreeBlock(fs->disk, bNum, nextNum);
      if (!nextNum) {
         fs->sblock.lastFree = bNum;
      }
      bNum = nextNum;
      fs->freeCount++;
   }
   fd->open = 0;
   for (i = 0; i < fs->inodeCount 
			&& strcmp(fs->inodes[i].name, inode->name) != 0
			|| fs->validInode[i] == 0; i++)
         ;
   fs->validInode[i] = 0;

   writeSuperBlock(fs->disk, fs->sblock.freeList, inode->next, fs->sblock.lastFree, fs->sblock.blockCount);
   return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
   FileExtent fileExtent;
   int bNum, block, offset;
   int i;
   int error;
   OpenFile *file;
   
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }

   error = checkFD(FD);

   if (error)
      return error;

   if (fs->fdsTable[FD].offset >= fs->fdsTable[FD].inode->size) {
      tfs_SetErrno(TINY_EOF_ERROR);
      return TINY_EOF_ERROR;
   }

   file = &(fs->fdsTable[FD]);
   block = file->offset / DATA_SIZE;
   offset = file->offset % DATA_SIZE;

   for (i = 0, bNum = file->inode->startBlock; i < block; i++)
      bNum = getNextBlock(fs->disk, bNum);

   getFileExtent(fs->disk, bNum, &fileExtent);
   *buffer = fileExtent.data[offset];
   file->offset++;
   return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
   int error;

   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }

   error = checkFD(FD);
   if (error)
      return error;

   if (offset > fs->fdsTable[FD].inode->size || offset < 0) {
      tfs_SetErrno(BAD_SEEK_ERROR);
      return BAD_SEEK_ERROR;
   }

   fs->fdsTable[FD].offset = offset;
   return 0;  
}

int tfs_makeRO(char *name) {
   int i;
   Inode *inode;
   // find the inode if it exists
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }
   
   for (i = 0; i < fs->inodeCount; i++)
      if (fs->validInode[i] && strcmp(fs->inodes[i].name, name) == 0)
         break;

   if (i == fs->inodeCount) {
      tfs_SetErrno(FILE_NOT_FOUND_ERROR);
      return FILE_NOT_FOUND_ERROR;
   }

   inode = &fs->inodes[i];
   inode->readOnly = 1;
   writeInode2(fs->disk, inode); 
   return 0;
}

int tfs_makeRW(char *name) {
   int i;
   Inode *inode;
   // find the inode if it exists
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }
   
   for (i = 0; i < fs->inodeCount; i++)
      if (fs->validInode[i] && strcmp(fs->inodes[i].name, name) == 0)
         break;

   if (i == fs->inodeCount) {
      tfs_SetErrno(FILE_NOT_FOUND_ERROR);
      return FILE_NOT_FOUND_ERROR;
   }

   inode = &fs->inodes[i];
   inode->readOnly = 0;
   writeInode2(fs->disk, inode); 
   return 0;
}

int tfs_writeByte(fileDescriptor FD, unsigned char data) {
   int error;
   OpenFile *fd;
   Inode *inode;
   FileExtent fileExtent;
   int size;
	int numBytes;
   char *buffer;
   int i;
   int offsetRegister = 0;
   unsigned short curBlock, nextBlock;

   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }
   
   error = checkFD(FD);
   if (error)
      return error;

   fd = &(fs->fdsTable[FD]);  

   inode = fd->inode;

   if (inode->readOnly) {
      tfs_SetErrno(READ_ONLY_ERROR);
      return READ_ONLY_ERROR;
   }

   if (fd->offset > inode->size) {
      tfs_SetErrno(TINY_EOF_ERROR);
      return TINY_EOF_ERROR;
   }

   if (inode->startBlock == 0) {
		buffer = calloc(1, sizeof(char));
		buffer[0] = data;
      error = tfs_writeFile(FD, buffer, 1);
		free(buffer);
		if (error)
			return error;
   }
   else {
      if (fd->offset < inode->size) {
         buffer = calloc(inode->size, sizeof(char));
         size = inode->size;
      }
      else {
         buffer = calloc(inode->size + 1, sizeof(char));
         size = inode->size + 1;
      }

      for (curBlock = inode->startBlock, i = 0; curBlock; curBlock = nextBlock, i++) {
         getFileExtent(fs->disk, curBlock, &fileExtent);
         nextBlock = fileExtent.next;
			numBytes = nextBlock ? DATA_SIZE : inode->size % DATA_SIZE;
         if (nextBlock == 0 && inode->size % DATA_SIZE == 0) 
				numBytes = DATA_SIZE;
			memcpy(buffer + (i * DATA_SIZE), fileExtent.data, numBytes);
      }
      
      buffer[fd->offset] = data;
      offsetRegister = fd->offset;
      error = tfs_writeFile(FD, buffer, size);
      free(buffer);
		if (error)
			return error;
   }
   
   fd->offset = offsetRegister + 1;

   return 0;
}

int tfs_rename(char * oldname, char * newname) {
   int i;
   Inode * inode;

   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }

   if (strlen(oldname) > MAX_NAME_LEN) {
      tfs_SetErrno(INVALID_FILENAME_LENGTH);
      return INVALID_FILENAME_LENGTH;
   }

   if (strlen(newname) > MAX_NAME_LEN) {
      tfs_SetErrno(INVALID_FILENAME_LENGTH);
      return INVALID_FILENAME_LENGTH;
   }

   for (i = 0; i < fs->inodeCount; i++)
      if (!strcmp(fs->inodes[i].name, newname) && fs->validInode[i]) {
         tfs_SetErrno(DUPLICATE_NAME_ERROR);
         return DUPLICATE_NAME_ERROR;
      }

   for (i = 0; i < fs->inodeCount; i++)
      if (!strcmp(fs->inodes[i].name, oldname) && fs->validInode[i]) {
         strcpy(fs->inodes[i].name, newname);
         inode = &fs->inodes[i];
         inode->time = time(NULL);
			writeInode2(fs->disk, inode);
         return 0;
      }

   tfs_SetErrno(FILE_NOT_FOUND_ERROR);
   return FILE_NOT_FOUND_ERROR;
}

int tfs_readdir() {
   int i;
   char *timestamp;
   
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NO_MOUNTED_FS_ERROR;
   }
   for (i = 0; i < fs->inodeCount; i++) {
      if (fs->validInode[i]) {
			printf("FILE ");
			fs->inodes[i].readOnly ? printf("r- ") : printf("rw ");
			printf("%5dB ", fs->inodes[i].size);
			timestamp = asctime(localtime(&fs->inodes[i].time));
			timestamp[strlen(timestamp) - 1] = '\0';
			printf("%s ", timestamp);
			printf("%s\n", fs->inodes[i].name);
		}
   }

   return 0;
}

char* tfs_readFileInfo(fileDescriptor FD) {
   OpenFile *fd;
   Inode *inode;
   char *timestamp;

   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return NULL;
   }
   if (FD >= fs->fdsTableCount || !fs->fdsTable[FD].open) {
      tfs_SetErrno(INVALID_FD_ERROR);
      return NULL;
   }
   if (!fs->timestampBuff)
      fs->timestampBuff = (char *)calloc(30, 1);

   fd = &fs->fdsTable[FD];
   inode = fd->inode;
   timestamp = asctime(localtime(&inode->time));
   strcpy(fs->timestampBuff, timestamp);
   return fs->timestampBuff;
}

/* ================= Static Helper Functions ======================= */

static int checkFD(fileDescriptor FD) {
   if (FD >= fs->fdsTableCount) {
      tfs_SetErrno(INVALID_FD_ERROR);
      return INVALID_FD_ERROR; 
   }

   if (fs->fdsTable[FD].open == 0) {
      tfs_SetErrno(FILE_CLOSED_ERROR);
      return FILE_CLOSED_ERROR;
   }

   return 0;
}

static int min(int a, int b) {
   return a < b ? a : b;
}

/* ====================== Block Functions ========================= */

char getBlockType(int disk, int bNum) {
   char type;
   int r = lseek(disk, BLOCKSIZE * bNum, SEEK_SET);
   r = read(disk, &type, 1);
   return type;
}

int getNextBlock(int disk, int bNum) {
   SuperBlock superBlock;
   Inode inode;
   FileExtent fileExtent;
   Free freeBlock;
   int next;
   char type = getBlockType(disk, bNum);
   
   switch (type) {
      case SUPER:
         getSuperBlock(disk, &superBlock);
         next = superBlock.firstInode;
         break;
      case INODE:
         getInode(disk, bNum, &inode);
         next = inode.startBlock;
         break;
      case FILE_EXTENT:
         getFileExtent(disk, bNum, &fileExtent);
         next = fileExtent.next;
         break;
      case FREE:
         getFreeBlock(disk, bNum, &freeBlock);
         next = freeBlock.next;
         break;
      default:
         next = 0;
   }

   return next;
}

int findFreeBlock(SuperBlock *sblock, int disk) {
   int next = sblock->freeList;

   if (sblock->freeList != 0) 
      sblock->freeList = getNextBlock(disk, next);
   else
      return 0;

   if (!sblock->freeList)
      sblock->lastFree = 0;

   return next;
}

void getSuperBlock(int disk, SuperBlock *superBlock) {
   char buffer[BLOCKSIZE];

   readBlock(disk, 0, buffer);
   memcpy(superBlock, buffer, sizeof(SuperBlock));
}

void writeSuperBlock(int disk, unsigned short freeList, unsigned short firstInode, unsigned short lastFree, unsigned short blockCount) {
   char buffer[BLOCKSIZE];
   SuperBlock sBlock;
   sBlock.type = SUPER;
   sBlock.magicNum = MAGIC_NUM;
   sBlock.freeList = freeList;
   sBlock.firstInode = firstInode;
   sBlock.lastFree = lastFree;
   sBlock.blockCount = blockCount;

   memset(buffer, 0, BLOCKSIZE);
   memcpy(buffer, &sBlock, sizeof(SuperBlock));
   writeBlock(disk, 0, buffer);
}

void getInode(int disk, int bNum, Inode *inode) {
   char buffer[BLOCKSIZE];

   readBlock(disk, bNum, buffer);
   memcpy(inode, buffer, sizeof(Inode));
}

void writeInode2(int disk, Inode *inode) {
	char buffer[BLOCKSIZE];
   int bNum = inode->blockNdx;

   if (bNum == 0) {
      tfs_SetErrno(INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_ERROR);
      return;
   }

	memset(buffer, 0, BLOCKSIZE);
	memcpy(buffer, inode, sizeof(Inode));
	writeBlock(disk, bNum, buffer);
}

void writeInode(int disk, int bNum, int startBlock, int size, char * name, int next, time_t time) {
   char buffer[BLOCKSIZE];
   Inode inode;
   if (bNum == 0) {
      tfs_SetErrno(INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_ERROR);
      return;
   }

   inode.type = INODE;
   inode.magicNum = MAGIC_NUM;
   inode.startBlock = startBlock,
   inode.blockNdx = bNum;
   inode.size = size;
   inode.readOnly = 0;
   inode.time = time;

   strcpy(inode.name, name);   
   inode.next = next;
   memset(buffer, 0, BLOCKSIZE);
   memcpy(buffer, &inode, sizeof(Inode));
   writeBlock(disk, bNum, buffer);
}

void getFileExtent(int disk, int bNum, FileExtent *fileExtent) {
   char buffer[BLOCKSIZE];

   readBlock(disk, bNum, buffer);
   memcpy(fileExtent, buffer, sizeof(FileExtent));
   
}

void writeFileExtent(int disk, int bNum, int nextBlock, char *data, int dataSize) {
   char buffer[BLOCKSIZE];
   FileExtent extentBlock;
   if (bNum == 0) {
      tfs_SetErrno(INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_ERROR);
      return;
   }
   extentBlock.type = FILE_EXTENT;
   extentBlock.magicNum = MAGIC_NUM;
   extentBlock.next = nextBlock;
   memset(extentBlock.data, 0, DATA_SIZE);
   memcpy(extentBlock.data, data, dataSize);

   memcpy(buffer, &extentBlock, sizeof(FileExtent));
   writeBlock(disk, bNum, buffer);
}

void writeFileExtent2(int disk, int bNum, FileExtent *fileExtent) {
	char buffer[BLOCKSIZE];

	memset(buffer, 0, BLOCKSIZE);
	memcpy(buffer, fileExtent, sizeof(FileExtent));
	writeBlock(disk, bNum, buffer);
}

void getFreeBlock(int disk, int bNum, Free *freeBlock) {
   char buffer[BLOCKSIZE];

   readBlock(disk, bNum, buffer);
   memcpy(freeBlock, buffer, sizeof(Free));
}

void writeFreeBlock(int disk, int bNum, int nextBlock) {
   char buffer[BLOCKSIZE];
   Free freeBlock;
   if (bNum == 0) {
      tfs_SetErrno(INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_ERROR);
      return;
   }
   freeBlock.type = FREE;
   freeBlock.magicNum = MAGIC_NUM;
   freeBlock.next = nextBlock;

   memset(buffer, 0, BLOCKSIZE); 
   memcpy(buffer, &freeBlock, sizeof(Free));
   writeBlock(disk, bNum, buffer);
}

// ================== Start of Debugging Functions =====================

void pBlock(int bNum) {
   SuperBlock superBlock;
   Inode inode;
   FileExtent fileExtent;
   Free freeBlock;
   int i;
   char type;
	int disk;

   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return;
   }

	if (bNum < 0 || bNum >= fs->sblock.blockCount) {
		tfs_SetErrno(INVALID_BLOCK_ERROR);
		return;
	}

	disk = fs->disk;

	type = getBlockType(disk, bNum);
   printf("Block %d:\n", bNum);
   switch (type) {
      case SUPER:
         getSuperBlock(disk, &superBlock);
         printf("Type:       SUPER\n");
         printf("MagicNum:   %d\n", superBlock.magicNum);
         printf("freeList:   %d\n", superBlock.freeList);
         printf("lastFree:   %d\n", superBlock.lastFree);
         printf("firstInode: %d\n", superBlock.firstInode);
         break;
      case INODE:
         getInode(disk, bNum, &inode);
         printf("Type:       INODE\n");
         printf("MagicNum:   %d\n", inode.magicNum);
         printf("startBlock: %d\n", inode.startBlock);
         printf("size:       %d\n", inode.size);
         printf("name:       %s\n", inode.name);
         printf("blockNdx:   %d\n", inode.blockNdx);
         printf("Next:       %d\n", inode.next);
         break;
      case FILE_EXTENT:
         getFileExtent(disk, bNum, &fileExtent);
         printf("Type:       EXTENT\n");
         printf("MagicNum:   %d\n", fileExtent.magicNum);
         printf("Next:       %d\n", fileExtent.next);
         printf("Data:\n   ");
         for (i = 0; i < DATA_SIZE; i++)
            printf("%c", fileExtent.data[i]);
         printf("\n");
         break;
      case FREE:
         getFreeBlock(disk, bNum, &freeBlock);
         printf("Type:       FREE\n");
         printf("MagicNum:   %d\n", freeBlock.magicNum);
         printf("Next:       %d\n", freeBlock.next);
         break;
      default:
         printf("block type unknown\n");
   }
   printf("\n");
}

void listBlocks() {
   char type;
   int i;
	int disk;

   if(!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return;
   }

	disk = fs->disk;

   printf("List of all blocks:\n");
   for (i = 0; i < fs->sblock.blockCount; i++) {
      type = getBlockType(disk, i);
      switch (type) {
         case SUPER:
            printf("S ");
            break;
         case INODE:
            printf("I ");
            break;
         case FILE_EXTENT:
            printf("E ");
            break;
         case FREE:
            printf("F ");
				break;
			default:
				printf("? ");
      }
   }
}

void printFdTable() {
   int i;
   
   if (!fs) {
      tfs_SetErrno(NO_MOUNTED_FS_ERROR);
      return;
   }
   
   printf("File Descriptor Table:\n");
   for (i = 0; i < fs->fdsTableCount; i++)
      printf("FD[%d] offset=%d, open=%d\n", i, fs->fdsTable[i].offset, fs->fdsTable[i].open);
}

/* ========================= Errno Functions ===================== */

void printErrorMessage() {
	char *msg;
	switch (tfs_errno) {
		case OUT_OF_SPACE_ERROR:
			msg = OUT_OF_SPACE_MSG;
			break;
		case UNOPENED_FILE_ERROR:
			msg = UNOPENED_FILE_MSG;
			break;
		case INVALID_FD_ERROR:
			msg = INVALID_FD_MSG;
			break;
		case TINY_EOF_ERROR:
			msg = TINY_EOF_MSG;
			break;
		case FILE_NOT_FOUND_ERROR:
			msg = FILE_NOT_FOUND_MSG;
			break;
		case DISK_OPEN_ERROR:
			msg = DISK_OPEN_MSG;
			break;
		case FILE_CLOSED_ERROR:
			msg = FILE_CLOSED_MSG;
			break;
		case NO_MOUNTED_FS_ERROR:
			msg = NO_MOUNTED_FS_MSG;
			break;
		case INVALID_FILENAME_LENGTH:
			msg = INVALID_FILENAME_LENGTH_MSG;
			break;
		case READ_ONLY_ERROR:
			msg = READ_ONLY_MSG;
			break;
		case INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_ERROR:
			msg = INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_MSG;
			break;
		case DUPLICATE_NAME_ERROR:
			msg = DUPLICATE_NAME_MSG;
			break;
		case NEGATIVE_OFFSET_ERROR:
			msg = NEGATIVE_OFFSET_MSG;
			break;
		case BAD_SEEK_ERROR:
			msg = BAD_SEEK_MSG;
			break;
		case INVALID_BLOCK_ERROR:
			msg = INVALID_BLOCK_MSG;
			break;
		case BAD_DISK_SIZE_ERROR:
			msg = BAD_DISK_SIZE_MSG;
			break;
		case CORRUPT_DISK_ERROR:
			msg = CORRUPT_DISK_MSG;
			break;
		case BAD_ARGS_ERROR:
			msg = BAD_ARGS_MSG;
			break;
		default:
			printf("Undefined tfs_errno %d\n", tfs_errno); 
			msg = "No Error";
	}

	fprintf(stderr, "%s \n", msg);
}

void tfs_SetErrno(int newErrno) {
	tfs_errno = newErrno;
	printErrorMessage();
}


