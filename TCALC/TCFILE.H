#include <stdio.h>
void logmsg(char *fmt, ...);
void writeDebugLog(char* data);
long reusableFileMem();
long filesize(char* fileName);
void lineAtPos(FILE *stream,long apprxPos, long totalSize, char* line, long* startPos, long* endPos);
int isPagePresent(int prevNext);
char* readSanity(char* filename, long totalSize);
char* readEntire(char* fileName);
char* readPartial(char* fileName, int prevNext, long* nxtOffset  );
char* readPartialOffset(char* fileName , int prevNext, long offset, long* nxtOffset);
void writePartial(char* fileName,char* data ,long offset, long nxtOffset, long* newOffset /*New next offset*/);