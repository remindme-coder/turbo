/* Turbo C++ - (C) Copyright 1987-1991 by Borland International */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mem.h>
#include <conio.h>
#include <string.h>
#include "tcfile.h"
#include "tcalc.h"
#include <dos.h>
#include <math.h>

char debugLog[100];
char *book = NULL;
long bookSZ = 3 * 1024;

int timeAdded = 0;
DATAINFO di;

void logmsg(char *fmt, ...)
{
   int cnt; va_list argptr;

   if(strchr(fmt,'%')){
      va_start(argptr, fmt);
      vsprintf( (char*)debugLog, (char*)fmt, argptr);//cnt = 
      va_end(argptr);
   } else {
      strcpy((char*)debugLog,fmt);
      debugLog[strlen(fmt)] = 0;
   }

   writeDebugLog((char*)debugLog);
   //return(cnt);
}

void writeDebugLog(char* data)
{
   struct  time t;
   char k[100];
   struct date d;
   int i;
   FILE *logStream;

   if ((logStream = fopen("data/debugLog.$$$", "a")) == NULL) /* open file debugLog.$$$ */
   {
      fprintf(stderr, "Cannot open output file.\n");
      return;
   }

   if(!timeAdded){
	  gettime(&t);
     getdate(&d);

	  sprintf(k,"\n%d-%d-%d %2d:%02d:%02d.%02d\n",d.da_year, d.da_mon, d.da_day, t.ti_hour, t.ti_min, t.ti_sec, t.ti_hund);
	  fwrite(k, sizeof(char), strlen(k), logStream); 
	  for(i=0;i<=60;i++) fwrite( "-", sizeof(char), 1, logStream);
	  fwrite( "\n", sizeof(char), 1, logStream);

	  timeAdded = 1;
   }

   fwrite(data, sizeof(char), strlen(data), logStream); 
   fclose(logStream); /* close file */
}

long reusableFileMem() {
   return (book? bookSZ:0L);
}
char* reusableFile() {
   if( reusableFileMem() <= 0L ){
    book = (char*)calloc(bookSZ, sizeof(char));
   }
   return book;
}

long filesize(char* fileName)
{
   long curpos, length;
   FILE *stream;
	stream = fopen(fileName,"r");
   curpos = ftell(stream);
   fseek(stream, 0L, SEEK_END);
   length = ftell(stream);
   fseek(stream, curpos, SEEK_SET);
   fclose(stream);
   return length;
}


void lineAtPos(FILE *stream,long apprxPos, long totalSize, char* line, long* startPos, long* endPos)
/* This method returns the line start/end position, and the line also */
{
   char tmp[MAXROWCHARS+1], ch[1], *ptr, *ptr2, *occur1, *occur2;
   int i,exact,left=0,right=0, lastI, range,pass, iter=0;
   long orig, tmpPos, beginPos,wall;
   
   blankOut(tmp, MAXROWCHARS);

   if(apprxPos < 0) {
      strcpy(line, "");
      *endPos = 0;
      return;
   }
   else if(apprxPos <= 3) {
      tmpPos = beginPos = 0;
      fseek(stream, beginPos  , SEEK_SET);
      fread(tmp,sizeof(char),MAXROWCHARS,stream);
      ptr = strchr(tmp, '\n');
      
      if(ptr) {
         i = (int)(ptr - &tmp[0]);
         tmp[i] = 0;
      }
   }
   else if( totalSize - apprxPos < 3 /* Adjustment */) {
      // The requested line is at the end of the file
      beginPos = totalSize - MAXROWCHARS;
      tmpPos = beginPos;
      if( beginPos < 0) beginPos = 0;
      
      fseek(stream, beginPos  , SEEK_SET);
      fread(tmp,sizeof(char),MAXROWCHARS,stream);

      ptr2 = &tmp[0];
      ptr = strchr(tmp, '\n');

      // surf and reach last line
      while(ptr && (ptr - ptr2) > 0 && (apprxPos - tmpPos > 3) ){
         i = ptr - ptr2 + 2;
         tmpPos += i;
         ptr++;
         ptr2 = ptr;
         ptr = strchr(ptr, '\n');
      }

      tmpPos = tmpPos - i;
      ptr2 = (ptr2 - i) + 1;
      movmem(ptr2,&tmp[0], i-2 );
      tmp[i-2] = 0;
   } else {
      range = 10;
      
      do{
         occur1 = occur2 = NULL;
         pass = 0;
         exact = range/2;
         if(left == 0 && right == 0)   tmpPos = apprxPos - exact - 1;

         fseek(stream, tmpPos, SEEK_SET);
         fread(tmp,sizeof(char),range,stream);
         tmp[range] = 0;
         
         //logmsg("L:%d, R: %d, Text at pos:%ld is %s\n",left,right,tmpPos, tmp);
         occur1 = strchr(tmp, '\n');
         iter++;

         if( left ){
            i = strlen(tmp) - 1;
            while( i >=0 ) { 
               if( !occur2 && iscntrl(tmp[i]) ) occur2 = &tmp[i];
               else if( occur2 ) {
                  if(pass){
                     occur1 = &tmp[i];
                     if(iscntrl(tmp[i])) break;
                  } else {
                     if(isprint(tmp[i]))  pass = 1;
                  }
               }
               i--;
            }
            
            if(i>=0) {
               tmpPos += (occur1 - &tmp[0]) + 2;// beginning of line
               i = occur2-occur1;
               movmem(occur1,&tmp[0], i);
               tmp[i] = 0;
               break;
            }
         }
         else if( right ){
            if(occur1){
               occur1[0] = 0;
               break;
            }
         }
         else if(occur1) {
            i = occur1 - &tmp[0];

            if( exact <= i ){
               // clearly we are at the traling end of the line
               left = 1;
               wall = tmpPos + i;
            }
            else if( exact > i ){
               // clearly we are at the beginning of the line
               right = 1;
               tmpPos += occur1 - &tmp[0] + 2;// beginning of line
               wall = tmpPos;
            }
         }
         
         range *= 2;
         if(left) tmpPos = wall - range + 1; //include new line char too
      } while( iter < 50 );

      trimJunk(tmp);
   }
   
   if (line != NULL){
      strcpy(line, tmp);
      trimJunk(line);
      //logmsg("Extracted line:%s at Pos:%ld\n", tmp, tmpPos);
   }

   // Find the next line position
   i = strlen(tmp);
   *startPos = tmpPos; // start of line position
   *endPos = tmpPos + i + 2; // end of line position
}

int isFirstPage() {
   return (isPagePresent(0) && di.curPage == 1);
}
int isLastPage() {
   return (isPagePresent(0) && ((di.totalSize - di.offsets[di.curPage+1]) <= 3));
}

int isPagePresent(int prevNext) {
   //logmsg( "di.totalSize:%ld, di.curPage:%d, di.offsets[]:%ld\n", (long)di.totalSize, di.curPage, (long)di.offsets[di.curPage] );
   if(prevNext == 0) {
    if (di.curPage == 1) return ((di.totalSize - di.offsets[di.curPage]) > 3);// might be a single page document
    else if (di.curPage <= 0) return FALSE; // There is no Page 0 (or) -1;
    else return TRUE; // There is always a page for multi-paged document
   }
   else if(prevNext == -1) return di.curPage > 1;
   else if(prevNext == 1) return (di.totalSize - di.offsets[di.curPage]) > 3  ;
   return 0;
}

char* readEntire(char* fileName, char *header){
   char temp[25],lastLine[MAXROWCHARS], *lastPtr;
   FILE *stream;
   long readSize,sPos,curpos, totalSize = filesize(fileName);
   
   readSize = bookSZ;
   if(totalSize > readSize)   bookSZ = readSize = totalSize;

   if(totalSize < 1 * 1024 ){
      if(!book)	book = (char*)malloc(readSize);
      else 		book = (char*)realloc(book, readSize);
   } else {
      if(!book)	book = (char*)calloc(readSize, sizeof(char));
      else 		book = (char*)realloc(book, readSize);
   }

   stream = fopen(fileName,"r");
   if(stream){
      // Read the header first
      lineAtPos(stream, 0, totalSize, header, &sPos, &curpos);
      
      fseek(stream, curpos, SEEK_SET);
      setmem(book,readSize,' ');
      fread(book,sizeof(char),totalSize,stream);
      curpos = ftell(stream);

      // Adjustment : 3 char adjustments to fetch the last line
      lineAtPos(stream, curpos-3, totalSize, lastLine, &sPos, &curpos);
      lastPtr = strstr(book, lastLine);
      if(lastPtr) lastPtr[strlen(lastLine)] = 0;
      else book[totalSize] = 0;

	   fclose(stream);
   }

   logmsg("BuffSZ:%ld, DataSZ:%d, mem PTR:%ld,sPos:%ld\n", totalSize, strlen(book), (long)book,sPos);
   return book;
}

int isNewLn(char k){
   return (iscntrl(k) && (k == '\r'||k == '\n')) ? 1 : 0;
}

char* readPartialOffset(char* fileName , int prevNext, long offset, long* nxtOffset){
   int i;
   char *header;
   strcpy( di.fileName , fileName);
   di.curPage = 1;         // Used
   for(i=0; i< 12; i++) di.offsets[i] = 0; // clearing...
   di.offsets[1] = offset;

   return readPartial(fileName , prevNext, header , nxtOffset);
}

char* readPartial(char* fileName , int prevNext /*-1 is prev, +1 is next, 0 curr*/, char *header, long* nxtOffset){
   long curPos, prevPos, offsetA=0, offsetB=0, totalSize = filesize(fileName), partialSize = 0  ;
   FILE *stream;
   char *pencil,*endPtr, tmp[MAXROWCHARS+1], lastLine[MAXROWCHARS], *lastPtr, first10[11];
   float tmpCalc,xtraCh = 0; // ranges 0.0 ... 1.0 
   int lines = 0, remCount, readSize, i, xtraData, pageNo =0, dataSZ, allocSZ = 0;
   char *lineMsg="line at %d : %s\n" ;

   if((di.curPage <= 1 && prevNext == -1) || ((totalSize - di.offsets[di.curPage] < 3 /* Adjustment */ ) && prevNext == 1) ) 
      return NULL;   

   // Book keeping
   di.totalSize = totalSize;
   //logmsg("captured:%s\nactual:%s", di.fileName, fileName);
   if(strcmp(di.fileName, fileName) == 0) {
      pageNo = di.curPage + prevNext;
      di.curPage = pageNo;
      offsetA = di.offsets[pageNo - 1];// Used
      offsetB = di.offsets[pageNo];    // Book keeping only
      partialSize = di.partialSize;    // Used
   } else {
      strcpy( di.fileName , fileName);
      di.curPage = pageNo = 1;         // Used
      offsetA = offsetB = 0;           // Used
      for(i=0; i< 12; i++) di.offsets[i] = 0; // clearing...
   }

   if(offsetA >= totalSize) return NULL;
   
   readSize = bookSZ;
   if(totalSize < readSize) {
      bookSZ = readSize = partialSize = totalSize;
   } else {
      if(partialSize < readSize) partialSize = readSize;
      else bookSZ = readSize = partialSize;
   }

   allocSZ = readSize;
   
   if(!book)	book = (char*)calloc(readSize+1,sizeof(char));
   else 		book = (char*)realloc(book, readSize+1);

   setmem(book,readSize,' ');
   stream = fopen(fileName,"r");
   if(!stream) return NULL;
   strcpy(book , NULL);

   if(pageNo == 1 && offsetA == 0 ) {
    // Read the header first when at page 1
    lineAtPos(stream, 0, totalSize, header, &prevPos, &curPos);

    #if HEADERROW == 1 
    offsetA = curPos;
    di.offsets[0] = curPos;
    #endif
   }

   //logmsg("offset:(%ld,%ld), page:%d, size:(%ld/%ld)\n",offsetA, offsetB,pageNo, (long)readSize, (long)totalSize);
   prevPos = offsetA;
   fseek(stream, offsetA, SEEK_SET);
   fread(book,sizeof(char),readSize,stream);
   curPos = ftell(stream);
   
   
   if((curPos - prevPos) >= readSize) {
      dataSZ = readSize;
   } else {
      dataSZ = (curPos - prevPos);
   }
   

   // 1. count the number of lines fetched first time
   book[dataSZ] = 0;
   pencil = &book[0];
   lines = 0;
   memcpy(first10,book,10);
   first10[10]=0;
   
   while( pencil && (isprint(pencil[0])||isNewLn(pencil[0])) ){
      while(isNewLn(pencil[0])) pencil++;
      while(isprint(pencil[0]) ) pencil++;
      
      if(pencil ) {
         if(isNewLn(pencil[0])) {
            lines++;
            lastPtr = pencil;
         }
      }
   }

   // 2. Load all MAXROWS
   if(lines < MAXROWS  && curPos < totalSize) {
      allocSZ = dataSZ;
      i = lines;

      // a. Identify the additional allocations required
      while(i < MAXROWS && curPos < totalSize ){
         prevPos = curPos;
         fread(tmp,sizeof(char),MAXROWCHARS,stream);
         curPos = ftell(stream);

         if((curPos - prevPos) >= MAXROWCHARS) {
            tmp[MAXROWCHARS] = 0;
            allocSZ += MAXROWCHARS;
         } else {
            tmp[(int)(curPos - prevPos)] = 0;
            allocSZ += strlen(tmp);
         }
         
         // continuing... read from where we left
         if(isNewLn(tmp[0])) i++;
         pencil = &tmp[0];

         while(pencil && (isprint(pencil[0])||isNewLn(pencil[0])) ){
            while(isNewLn(pencil[0])) pencil++;
            while(isprint(pencil[0]) ) pencil++;
            
            if(pencil ) {
               if(isNewLn(pencil[0])) {
                  i++;
                  lastPtr = pencil;
               }
            }
         }
      }
      
      // b. lets reallocate and reload the data
      book = (char*)realloc(book, allocSZ);
      setmem(book,allocSZ,' ');
      fseek(stream, offsetA, SEEK_SET);
      fread(book,sizeof(char),allocSZ,stream);
      curPos = ftell(stream);
      lines = 0;
      book[allocSZ] = 0;
      pencil = &book[0];

      while(pencil && (isprint(pencil[0])||isNewLn(pencil[0])) ){
         while(isNewLn(pencil[0])) pencil++;
         while(isprint(pencil[0]) ) pencil++;
         
         if(pencil ) {
            if(isNewLn(pencil[0])) {
               lines++;
               lastPtr = pencil;

               if(lines<4){
                  memcpy(lastLine, pencil-11, 10) ;lastLine[10]=0;
                  logmsg(lineMsg, lines,lastLine);
               }
            }
         }
      }
   }

   //logmsg("last pencil :%s\n", pencil);
   dataSZ = pencil - &book[0];
   remCount = 0;//Unnecessary lines
   xtraData = pencil - lastPtr;

   pencil = lastPtr ; // Last char of the book
   endPtr = lastPtr;
   if(curPos-offsetA > dataSZ) xtraCh = (float)(curPos-offsetA-dataSZ)/(float)MAXROWS;
   else if(curPos == totalSize) xtraCh = 1; // default xtra char

   //logmsg("lines: %d, with data %d, max %d, Pos:%ld, book:%d chars\n", lines, dataSZ, allocSZ, curPos, strlen(book));
   //logmsg("Differences Pos:%d, Data:%d, xtraCh:%1.1f, xtraData:%d \n",  (int)(curPos-offsetA), dataSZ, (float)xtraCh, (int)xtraData);

   // 3. Reverse truncate lines added extra
   if(curPos < totalSize || lines > MAXROWS){
      i = 0;
      while(pencil && lines > MAXROWS && i < 50){
         while(isNewLn(pencil[0])) pencil--;
         while(isprint(pencil[0])) pencil--;
         if(isNewLn(pencil[0])) {
            lines--; 
            remCount++; 
            lastPtr = pencil;
            if(lines<=103){
               memcpy(lastLine, pencil-10, 10) ;lastLine[10]=0;
               logmsg(lineMsg, lines,lastLine);
            }
         }
         
         i++;
      }

      // Calculate the length added by xtra lines
      tmpCalc = (float)remCount * (float)xtraCh;

      if(curPos >= totalSize){
         // No need to worry about extra spaces
         trim(lastPtr);
         xtraData = strlen(lastPtr) + (int)ceil(tmpCalc);
         curPos = totalSize;
      } else {
         xtraData += (int)(endPtr - lastPtr) + (int)ceil(tmpCalc);
      }

      //logmsg("lastPtr - endPtr : %d, xtraData : %d,curPos:%ld \n", (int)(endPtr - lastPtr), (int)xtraData, (long)curPos);
      lineAtPos(stream, curPos - xtraData -3, totalSize, lastLine, &prevPos, &offsetB); // Adjustment : 3 char b4 for eol
      curPos = offsetB;
   } else {
      lineAtPos(stream, totalSize - 3, totalSize, lastLine, &prevPos, &offsetB); // Adjustment : 3 char b4 for eol
      curPos = offsetB;

      // Reverse truncate lines added extra
      i = 0;
      while(pencil && i < 50){
         while(isNewLn(pencil[0])) pencil--;
         while(isprint(pencil[0])) pencil--;
         if(strstr(pencil,lastLine)){
            endPtr = strstr(pencil,lastLine);
            endPtr += strlen(lastLine);
            endPtr[0] = 0;
            break;
         }
         else if(isNewLn(pencil[0])) {
            lines--; 
            remCount++; 
            endPtr = pencil;
         }
         i++;
      }
   }
   
   fclose(stream);

   // 4. Book keeping
   //logmsg( "page:%d, offset(%ld,%ld), book:%d chars\n", pageNo, offsetA, offsetB, strlen(book));
   di.partialSize = di.partialSize < dataSZ ? dataSZ : di.partialSize;
   di.offsets[pageNo - 1] = offsetA ;
   di.offsets[pageNo] = offsetB ;
   *nxtOffset = offsetB;
   strcpy(di.fileName, fileName);
   bookSZ = di.partialSize;

   return book;
}


int truncate(char *tmp, long fromOff, long* curPos ) {
  char *ptrSt,*pencil,*ptr,ch[1];
  long tmpPos = fromOff, newPos = 0L;
  int length = strlen(tmp), tmpLen, linesFound = 0, lineI;

  // Extract Lines
  ptrSt = &tmp[0];
  pencil = ptrSt;
  ptr = strchr(tmp, '\n');

  if(!ptr){
    newPos = fromOff + length;
    linesFound++;
  } else {
    // Skip the last line if it occupies whole MAXROWCHARS
    if( length > (MAXROWCHARS-(MAXROWS/2)) ){
      while( !( isNewLn(tmp[length-1]) || isprint(tmp[length-1]) ) ) length--;
      while( isNewLn(tmp[length-1]) ) length--;
      while( isprint(tmp[length-1]) ) length--;
    }

    while(ptr && (ptr - pencil) > 0 && (pencil - ptrSt < length) ){
     tmpPos += ptr - pencil + 2;
     ptr++;
     pencil = ptr;
     ptr = strchr(ptr, '\n');
     linesFound++;
    }

    newPos = tmpPos;
  }

  *curPos = newPos;
  return length;
}

void copyStream(long fromOff, FILE *fromStre, long fromMax, FILE *to, int checkState ) {
   int iter=0,origLen,length = MAXROWCHARS, correction=0;
   long curpos, prevPos;
   char tmp[MAXROWCHARS+1];

   curpos = fromOff;

   do{
      blankOut(tmp,MAXROWCHARS);
      length = MAXROWCHARS;
      if( fromMax - curpos < MAXROWCHARS) {
         length = fromMax - curpos;
      }

      fseek(fromStre, curpos, SEEK_SET);
      fread(tmp,sizeof(char),length,fromStre);
      tmp[length] = 0;
      prevPos = curpos;
      curpos = ftell(fromStre);
      if(curpos >= fromMax) {
         trim(tmp);
         length = strlen(tmp);
         correction = (curpos - fromMax);
         length -= correction;
         tmp[length] = 0;
      }
      // no extra characters, only a complete line considered
      length = truncate(/*out*/tmp, prevPos, /*out*/&curpos);
      if( checkState && length > MINROWCHARS ) {
         // Schema modifications
         origLen = length;
         tmp[length] = 0;
         length = modifyLines(/*out*/tmp);
         logmsg("Actual:%d, Modified:%d ", origLen, length );
      }

      fwrite(tmp, sizeof(char), length, to);
      iter++;
   } while( (fromMax - curpos) > 3 && iter<1000); // Adjustment

   if(isprint(tmp[length-1]))
      fwrite("\n", sizeof(char), 1, to);
   
   //logmsg("offset(%ld,%ld), curpos:%ld, correction:%d, iter: %d\n", fromOff, fromMax, curpos, correction, iter);
}


void writePartial(char* fileName,char* dataFile ,long offset, long nxtOffset, char* header, long* newOffset ){
   long curpos=0L/* Destination Stream */, length, readSize, totalSize  ;
   FILE *stream, *duplStream, *modStream;
   char duplFile[15] = "tcdupl.tmp" ;
   int pageNo;

   // step 1: Save the original to a tmp file
   totalSize = filesize(fileName);
   if(totalSize <= 0) {
      logmsg("File :%s, has 0 data\n", fileName);
      return;
   }
   //logmsg("origninal offset:%ld,file size:%ld\n", offset, totalSize);
   
   stream = fopen(fileName,"r");
   duplStream = fopen(duplFile, "w");
   copyStream(0L, stream, totalSize, duplStream, FALSE/*Only backup*/ );
   fclose(stream);
   fclose(duplStream);

   // step 2: Write the fresh data up to the current page( offset )
   totalSize = filesize(duplFile);
   if(totalSize == 0) return;
   stream = fopen(fileName,"w");
   duplStream = fopen(duplFile, "r");
   
   // step 2.1: Write the changed header
   if(di.offsets[0] > 0 && HEADERROW){
     fwrite(header, sizeof(char), strlen(header), stream);
     curpos = ftell(stream);
   }
   
   // step 2.2: Append the original data up to the current page( offset )
   if(di.offsets[0] <= offset){
     pageNo = 0;// 0 is header
     while( di.offsets[pageNo+1] <= offset ) {
      copyStream(di.offsets[pageNo], duplStream, di.offsets[pageNo+1], stream, TRUE/* schema check*/ );
      di.offsets[pageNo] = curpos;
      pageNo++;
      curpos = ftell(stream);
     }
     di.offsets[pageNo] = curpos;
   }

   // step 3: Append the changed data from the current page( offset - nxtOffset )
   readSize = filesize(dataFile);
   modStream = fopen(dataFile, "r");
   copyStream(0L, modStream, readSize, stream, FALSE/*Actual Changes from APP*/ );
   fclose(modStream);
   curpos = ftell(stream);
   *newOffset = curpos;

   // step 4: Append the rest of the unvisited pages( nxtOffset )
   if(nxtOffset < totalSize){
     copyStream(nxtOffset, duplStream, totalSize, stream, TRUE/* schema check*/  );
   }

   fclose(duplStream);
   fclose(stream);
}