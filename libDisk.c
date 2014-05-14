#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include "libDisk.h"
#include <sys/stat.h>
#include "libTinyFS.h"
/* This functions opens a regular UNIX file and designates the first 
 * nBytes of it as space for the emulated disk. nBytes should be an 
 * integral number of the block size. If nBytes > 0 and there is already
 * file by the given filename, that file’s content may be overwritten.
 * If nBytes is 0, an existing disk is opened, and should not be overwritten.
 * There is no requirement to maintain integrity of any file content beyond nBytes. 
 * The return value is -1 on failure or a disk number on success. */
int openDisk(char *filename, int nBytes) {
   int disk;

   if (nBytes == 0) {
      if (access(filename, F_OK) != -1)
         disk = open(filename, O_RDWR);
      else
         disk = -1;
   }
   else if (access(filename, F_OK) != -1)
      disk = open(filename, O_RDWR);
   else 
      disk = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

   if (disk == -1) {
      printf("Could not open disk, errno %d\n", errno); 
      return disk;    
   }
   if (nBytes > 0)
      ftruncate(disk, nBytes);

   return disk;
}

/* readBlock() reads an entire block of BLOCKSIZE bytes from the open disk 
 * (identified by ‘disk’) and copies the result into a local buffer 
 * (must be at least of BLOCKSIZE bytes). The bNum is a logical block 
 * number, which must be translated into a byte offset within the disk. The 
 * translation from logical to physical block is straightforward: bNum=0 is 
 * the very first byte of the file. bNum=1 is BLOCKSIZE bytes into the disk,
 * bNum=n is n*BLOCKSIZE bytes into the disk. On success, it returns 0. -1 or
 * smaller is returned if disk is not available (hasn’t been opened) or any other
 * failures. You must define your own error code system. */
int readBlock(int disk, int bNum, void *block) {
   int error;

   error = lseek(disk, BLOCKSIZE * bNum, SEEK_SET);

   if (error == -1) 
      printf("read: seek error, errno %d, bNum %d\n", errno, bNum);
   
   error = read(disk, block, BLOCKSIZE);

   if (error < 0) {
      printf("read error, errno %d\n", errno);
      return -1;
   }
   else 
      return 0;
}

/* writeBlock() takes disk number ‘disk’ and logical block number ‘bNum’ and
 * writes the content of the buffer ‘block’ to that location. ‘block’ must be
 * integral with BLOCKSIZE. Just as in readBlock(), writeBlock() must translate 
 * the logical block bNum to the correct byte position in the file. On success,
 * it returns 0. -1 or smaller is returned if disk is not available (i.e. hasn’t
 * been opened) or any other failures. You must define your own error code system. */
int writeBlock(int disk, int bNum, void *block) {
   int error;

   error = lseek(disk, BLOCKSIZE *bNum, SEEK_SET);

   if (error == -1) {
      printf("seek error, errno %d, bNum %d\n", errno, bNum);
   }
   error = write(disk, block, BLOCKSIZE);

   if (error != BLOCKSIZE) {
      printf("write error, errno %d\n", errno);
      return -1;
   }
   else 
      return 0;

}
