#include <stdio.h>
int logmsg(char *fmt, ...);
int writeDebugLog(char* data);
long reusableFileMem();
long filesize(char* fileName);
long lineAtPos(FILE *stream,long apprxPos, long totalSize, char* line);
int isPagePresent(int prevNext);
char* readSanity(char* filename, long totalSize);
char* readEntire(char* fileName);
char* readPartial(char* fileName, int prevNext, long* nxtOffset  );
void writePartial(char* fileName,char* data ,long offset, long nxtOffset, long* newOffset /*New next offset*/);