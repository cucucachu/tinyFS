#ifndef __TINYFS_ERRNO_H__
#define __TINYFS_ERRNO_H__

#define UNOPENED_FILE_ERROR -8
#define OUT_OF_SPACE_ERROR -9
#define INVALID_FD_ERROR -10
#define TINY_EOF_ERROR -11
#define FILE_NOT_FOUND_ERROR -12
#define DISK_OPEN_ERROR -13
#define FILE_CLOSED_ERROR -14
#define NO_MOUNTED_FS_ERROR -15
#define INVALID_FILENAME_LENGTH -16
#define READ_ONLY_ERROR -17
#define INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_ERROR -18
#define DUPLICATE_NAME_ERROR -19
#define NEGATIVE_OFFSET_ERROR -20
#define BAD_SEEK_ERROR -21
#define INVALID_BLOCK_ERROR -22
#define BAD_DISK_SIZE_ERROR -23
#define CORRUPT_DISK_ERROR -24
#define BAD_ARGS_ERROR -25

#define UNOPENED_FILE_MSG "Error: Requested file not open."
#define OUT_OF_SPACE_MSG "Error: Insufficient disk space."
#define INVALID_FD_MSG "Error: Invalid file descriptor."
#define TINY_EOF_MSG "Error: End of File."
#define FILE_NOT_FOUND_MSG "Error: File not found."
#define DISK_OPEN_MSG "Error: Could not open requested disk."
#define FILE_CLOSED_MSG "Error: File already closed."
#define NO_MOUNTED_FS_MSG "Error: No file system mounted." 
#define INVALID_FILENAME_LENGTH_MSG "Error: File names must be less than 9 characters in length."
#define READ_ONLY_MSG "Error: Attempted write to a read only file."
#define INAPPROPRIATELY_WRITING_TO_SUPERBLOCK_MSG "Error: This type of block cannot write to bNum 0."
#define DUPLICATE_NAME_MSG "Error: Cannot have two files of the same name."
#define NEGATIVE_OFFSET_MSG "Error: Offset cannot be negative."
#define BAD_SEEK_MSG "Error: Attempted to seek to illegal offset."
#define INVALID_BLOCK_MSG "Error: Attempted to access non-existent block."
#define BAD_DISK_SIZE_MSG "Error: Bad disk size" 
#define CORRUPT_DISK_MSG "Error: The disk you attempted to mount is corrupt"
#define BAD_ARGS_MSG "Error: Attempted to call a library function with invalid arguements."

void tfs_SetErrno(int newErrno);

void PrintErrorMessage();

#endif
