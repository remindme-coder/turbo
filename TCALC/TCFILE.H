#include <stdio.h>
int logmsg(char *fmt, ...);
int writeDebugLog(char* data);
long filesize(char* fileName);
long lineAtPos(FILE *stream,long apprxPos, char* line);
int isPagePresent(int prevNext);
char* readSanity(char* filename, long totalSize);
char* readEntire(char* fileName);
char* readPartial(char* fileName, int prevNext  );