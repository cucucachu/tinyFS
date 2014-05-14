#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "libTinyFS.h"
#include "TinyFS_errno.h"

int main() {
   char command = 0;
   int bNum;
	unsigned char writebyte;
   char filename[100], newname[100];
   char inbuff[5000] = "My name is George Stevenson. I met Cathy Lawry in the "
   "High school. She was a pretty girl. Her eyes were in blue color and had a nice"
   " long blond hair. We were in same age too.  I was fascinating about her and we "
   "became very good friends in months. After one year of friendship, we became lovers. "
   "Cathy meant everything to me, since then.After I came out the High school, I had to"
   " find out a job. I did many jobs but I couldn’t find a fare job for me. So...I "
   "didn’t have money in my pocket.  But Cathy never said a word about it. She always "
   "did courage me for find a job. At the time, I noticed that couples of rich guys were "
   "tailing on Cathy. But she never gave a even any damn look about them. She was always "
   "stick with me. I was really happy about her love. That was the only thing I got."
   "Time passed, I knew that without a job, I’m gonna loose Cathy soon. However I got a "
   "sales job in a super market, where I found my destiny.There were three other sales "
   "guys were there, and they were practicing as a music band. I was born in talent for "
   "guitars. Eventually we set up our band and got chances to play in parties. It didn’t "
   "assed years; our success was rising up day by day. We quite from our sales jobs and"
   "played full time. Once we got a TV show and we hit the jackpot.There after nobody "
   "can stop our progress. Our band was populated rapidly. All this time Cathy was "
   "nearby me like my shadow. Money was coming to me continuously. I spent more and "
   "more money for her.But day by day, my life was changing; I got much money, lots of "
   "friends, and the girls. Most of the nights, I was drunk. Eventually Cathie’s "
   "association was a headache for me. I wanted to stay away from her. Because, I felt "
   "Cathy ruins my privacy. I met new girls every night. I didn’t want to stick with "
   "Cathy any more. But she came to me almost every time she could.One evening she came "
   "to my apartment, on that day I was with another girl. But she didn’t blame on me. but "
   "e just asked,Why…why this? What happened to you now?I felt much guilty; I thought "
   "she may jump on me like a devil. But she didn’t do that. I felt a shame on me. I "
   "felt guilty about me. Finally I got angry about myself. But I exposed my anger on "
   "her…Bitch…you don’t need to fingering my life anymore…let me live my life alone…get "
   "lost you bitch…!!!  She left me forever on that day. I didn’t see her again. it was "
   "so ease to my mind. After one year, I left Munich. I came to Nuremberg and my life "
   "was a heaven since then. I got all most every happiness, there. Girls, money, Drunk "
   "nights, I thought this is the life.Life was going like a fairytale, for another eight "
   "ars. One night I felt very uncomfortable for my body. So I got some tablets and "
   "energy drinks like the other days. But it was continuing for two three weeks. "
   "Eventually, I got fever too. Some time I sweat all over my body in nights. I felt "
   "that my body is not same any more. Finally I visited a doctor.There, I got my lifes"
   "shocked news. I was infected by HIV. In a second, I realized everything. But I was "
   "too late. Eventually my body and Soul were decaying. I was abandoned by my music"
   "and so easily.. My friends abandoned me. but still I got money to spend.  I used "
   "to medicate every day now. But I knew that in very near future, medicine cannot "
   "live me anymore. day by day I reached to my death. Sometimes while I was just "
   "sleeping on my bed, I memorized my past. There, I felt Cathy, as one and only my "
   "own angel. She full filled all my life. My inside was filled with her memories. "
   "She was the one and only thing, in my mind. Then I realized what was there in my "
   "subconscious. But everything is now gone.One day, while I was sitting on my bed, in "
   "the nursing room, I saw that my angel was coming to me. Though I didn’t see her for "
   "ast 10 years, it didn’t difficult to recognize her pretty face. She is  more "
   "beautiful than before I thought to myself.  But now, I’m just a living Skeleton. "
   "She came and stand in front of me. I couldn’t stare her eyes straightly any more. "
   "I tuned my head down.  I felt, my eyes were getting wet, eventually that wet became "
   "tear drops and they were oozing one by one on my cheeks. Suddenly I felt something "
   "run through my hair, in seconds I remembered warm feelings of her hands, on my head,"
   " after ten years of time. she was running her fingers through my hair. I couldn’t "
   "stay still anymore. I just hold her hands in my hands. There, I felt the wedding ring"
   "on her left hand’s ring finger. I screamed and cried a lot. So... her too. There was "
   " more to talk. Silent was so good, rather than making sad by talking any more. She "
   "left me after few minutes and promised to visit me again, as soon as possible.When "
   "she was leaving I felt that, there is no more strength to live, in my body or soul. "
   "After 13 days of the above event, one morning, a nurse found Georg's death body, on "
   "the bed in the nursing room. His diary was found inside his pillow cover.This was the "
   "love and life story of George Stevenson.";

   char outbuff[10];
   int buffSize = strlen(inbuff);
   fileDescriptor FD;
   int nBytes, offset;
   int retVal;
   char *timestamp;

   printf("Testing the libTinyFs library\n\n");
   printf("Type H for a list of commands.\n");

   while (command != 'Q') {
      fscanf(stdin, "%c", &command);
      command = toupper(command);

      switch (command) {
         case 'H':
            printf(
               "G - mkfs       [filename] [nBytes]\n"
               "M - mount      [filename]\n"
               "U - unmount\n"
               "O - openFile   [name]\n"
               "C - closeFile  [FD]\n"
               "W - writeFile  [FD] [nBytes]\n"
               "D - deleteFile [FD]\n"
               "R - readByte   [FD]\n"
               "B - writeByte  [FD] [data]\n"
               "S - seek       [FD] [offset]\n"
					"X - make RO    [filename]\n"
					"Y - make RW    [filename]\n"
               "N - rename     [oldname] [newname]\n"
               "A - list all files\n"
               "P - printBlock [blockNum]\n"
               "F - printFdTable"
               "L - listBlocks\n"
               "T - print timestamp [FD]\n"
               "Q - Quit this program\n"
            );
            break;
         case 'G':
            fscanf(stdin, "%s %d", filename, &nBytes);
            retVal = tfs_mkfs(filename, nBytes);
            if (retVal >= 0)
               printf("Generated filesystem \"%s\" with %d bytes.\nReturned %d\n", filename, nBytes, retVal);
            break;
         case 'M':
            fscanf(stdin, "%s", filename);
            retVal = tfs_mount(filename);
            if (retVal >= 0)
               printf("Mounted filesystem \"%s\".\nReturned %d\n", filename, retVal);
            break;
         case 'U':
            retVal = tfs_unmount();
            if (retVal >= 0)
               printf("Unmounted the filesystem.\nReturned %d\n", retVal);
            break;
         case 'O':
            fscanf(stdin, "%s", filename);
            retVal = tfs_openFile(filename);
            if (retVal >= 0)
               printf("Opened file \"%s\".\nReturned (FD) = %d\n", filename, retVal);
            break;
         case 'C':
            fscanf(stdin, "%d", &FD);
            retVal = tfs_closeFile(FD);
            if (retVal >= 0)
               printf("Closed file with fd = %d.\nReturned %d\n", FD, retVal);
            break;
         case 'W': 
            fscanf(stdin, "%d %d", &FD, &buffSize);
            retVal = tfs_writeFile(FD, inbuff, buffSize);
            if (retVal >= 0)
               printf("Wrote to file with fd = %d.\nReturned %d\n", FD, retVal);
            break;
         case 'D':
            fscanf(stdin, "%d", &FD);
            retVal = tfs_deleteFile(FD);
            if (retVal >= 0)
               printf("Deleted file with fd = %d.\nReturned %d\n", FD, retVal);
            break;
         case 'R':
            fscanf(stdin, "%d", &FD);
            retVal = tfs_readByte(FD, outbuff);
            if (retVal >= 0)
               printf("Read byte from fd %d. Byte = %c.\nReturned %d\n", FD, *(char *)outbuff, retVal);
            break;
         case 'B':
            fscanf(stdin, "%d %c", &FD, &writebyte);
            retVal = tfs_writeByte(FD, writebyte);
            if (retVal >= 0)
               printf("Wrote byte to fd %d. Byte = %u.\nReturned %d\n", FD, writebyte, retVal);
            break;
         case 'S':
            fscanf(stdin, "%d %d", &FD, &offset);
            retVal = tfs_seek(FD, offset);
            if (retVal >= 0)
               printf("Seeked within fd %d to offset %d.\nReturned %d\n", FD, offset, retVal);
            break;
         case 'X':
            fscanf(stdin, "%s", filename);
            retVal = tfs_makeRO(filename);
            if (retVal >= 0)
               printf("Made file \"%s\" Read-Only.\nReturned %d\n", filename, retVal);
            break;
         case 'Y':
            fscanf(stdin, "%s", filename);
            retVal = tfs_makeRW(filename);
            if (retVal >= 0)
               printf("Made file \"%s\" Read-Write.\nReturned %d\n", filename, retVal);
            break;
         case 'N':
            fscanf(stdin, "%s %s", filename, newname);
            retVal = tfs_rename(filename, newname);
            if (retVal >= 0)
               printf("Changed file named \"%s\" to \"%s\"\nReturned %d\n", filename, newname, retVal);
            break;
         case 'A':
            tfs_readdir();
            break;
         case 'T':
            fscanf(stdin, "%d", &FD);
            timestamp = tfs_readFileInfo(FD);
            if (timestamp)
					printf("Timestamp for fd #%d: %s\n", FD, timestamp ? timestamp : "(none)");
            break;
         case 'F':
            printFdTable();
            break;
         case 'P':
            fscanf(stdin, "%d", &bNum);
            pBlock(bNum);
            break;
         case 'L':
            listBlocks();
            break;
         case 'Q':
            printf("Bye!\n");
            break;
      }
      printf("\n");
   }

   return 0;
}
