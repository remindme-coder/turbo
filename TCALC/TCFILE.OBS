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

char debugLog[MAXROWCHARS];
char *book = NULL;
long bookSZ = 3 * 1024;

int timeAdded = 0;
DATAINFO di;

int logmsg(char *fmt, ...)
{
   int cnt; va_list argptr;

   if(strchr(fmt,'%')){
      va_start(argptr, fmt);
      cnt = vsprintf( (char*)debugLog, (char*)fmt, argptr);
      va_end(argptr);
   } else {
      strcpy((char*)debugLog,fmt);
      debugLog[strlen(fmt)] = 0;
   }

   writeDebugLog((char*)debugLog);
   return(cnt);
}

int writeDebugLog(char* data)
{
   FILE *stream;
   struct  time t;
   char k[100];
   struct date d;
   int i;

   if ((stream = fopen("data/debugLog.$$$", "a")) == NULL) /* open file debugLog.$$$ */
   {
      fprintf(stderr, "Cannot open output file.\n");
      return 1;
   }

   if(!timeAdded){
	  gettime(&t);
     getdate(&d);

	  sprintf(k,"\n%d-%d-%d %2d:%02d:%02d.%02d\n",d.da_year, d.da_mon, d.da_day, t.ti_hour, t.ti_min, t.ti_sec, t.ti_hund);
	  fwrite(k, sizeof(char), strlen(k), stream); /* write struct s to file */
	  for(i=0;i<=60;i++) fwrite( "-", sizeof(char), 1, stream);
	  fwrite( "\n", sizeof(char), 1, stream);

	  timeAdded = 1;
   }

   fwrite(data, sizeof(char), strlen(data), stream); /* write struct s to file */
   fclose(stream); /* close file */
   return 0;
}

long reusableFileMem() {
   return (book? sizeof(book):0L);
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

long lineAtPos(FILE *stream,long apprxPos, long totalSize, char* line){
   char tmp[MAXROWCHARS+1], *ptr, *ptr2;
   int i,exact,left,right, lastI, range = 0 ;
   long orig = ftell(stream);
   blankOut(tmp, MAXROWCHARS);

   if(totalSize <= MAXROWCHARS) {
      fseek(stream, 0L, SEEK_SET);
      fread(tmp,sizeof(char),totalSize,stream);
      trim(tmp);
      lastI = i = 0;
      while( i < (strlen(tmp) - (totalSize - apprxPos)) ) { 
         if(isprint(tmp[i])) lastI = i; 
         i++;
      }
      i = exact = lastI;
   } else {
      // RANGE : (-255 to +255 characters)
      range = apprxPos > (MAXROWCHARS/2) ? (MAXROWCHARS/2) : apprxPos;

      fseek(stream, apprxPos-range, SEEK_SET);
      fread(tmp,sizeof(char),MAXROWCHARS,stream);

      if(apprxPos == totalSize) {
         trimJunkRight(tmp);
         range = strlen(tmp);
         while( iscntrl( tmp[range] )  && range > 0 ) range--;
      }

      i = exact = (range - 1);
   }
   
   //logmsg("Extraction step 1) %ld, index:%d, line:%s\n", apprxPos-range,i, tmp);
   
   // Navigate to beginning of line from apprxpos
   while( isprint( tmp[i] )  && i > 0 )i--;
   left = exact - i ;

   blankOut(tmp, MAXROWCHARS);
   fseek(stream, apprxPos-left, SEEK_SET);
   fread(tmp,sizeof(char),MAXROWCHARS,stream);
   trim(tmp);
   //logmsg("Extraction step 2) %ld,%d line:%s\n", apprxPos-left, left, tmp);

   // Adjustment : The line should contain atleast 3 chars. If not, skip!!!
   ptr = strchr(tmp, '\n');
   if( (ptr - &tmp[0]) > 3 ) ptr = &tmp[0]; 
   while( iscntrl(ptr[0]) || isspace(ptr[0]) ) ptr++;
   
   if( strchr(ptr, '\n') ){
      ptr2 = strchr(ptr, '\n') ;
      i = (ptr2 - ptr);
      memcpy(line, ptr, i );
      line[i]=0;
   } else {
      i = strlen(ptr);
      memcpy(line, ptr, i );
      line[i]=0;
	  ptr2 = ptr + i;
   }

   // Find the exact end-of-line Position
   apprxPos = (apprxPos-left) + (ptr2-&tmp[0]);
   trimJunk(line);
   //logmsg("Extracted line:%s\n", line);

   fseek(stream, orig, SEEK_SET);
   return apprxPos;
}

char* lastLineInChunk(char* chunk, char* line){
   char *lastPtr, tmp[MAXROWCHARS];int i=0;
   lastPtr = strstr(chunk, line);
   if(!lastPtr){
      strcpy(tmp, line);
      i = strlen(line);
      
	   tmp[--i] = 0;
      lastPtr = strstr(chunk, tmp);
      // Adjustment : Atleast 15 char match required
	   while(!lastPtr && i >= 15) {
		   tmp[--i] = 0;
		   lastPtr = strstr(chunk, tmp);
	   }
      if(lastPtr) return lastPtr;
      else return NULL;
   } else {
      return lastPtr;
   }
}

long linePosition(FILE *stream,char* search,long apprxPos/*<=*/){
	char tmp[MAXROWCHARS],inp[MAXROWCHARS/2],*ptr;int i,loc ;
   long orig;

   orig = ftell(stream);
   apprxPos-=MAXROWCHARS/2;
	
	do{
		fseek(stream, apprxPos, SEEK_SET);
		fread(tmp,sizeof(char),MAXROWCHARS,stream);

		strcpy( inp, search );
		inp[strlen(search)]=0;
		ptr = strstr(tmp, inp);
		i = strlen(inp);

		while(!ptr && i > 0) {
			i--;
			inp[i] = 0;// Try for a partial match instead of exact one
			ptr = strstr(tmp, inp);
		}

      if(i>0)  loc = (ptr - &tmp[0]);
      else     loc = MAXROWCHARS/2;

      apprxPos += loc;
	} while(loc>0);

	fseek(stream, orig, SEEK_SET);

	return apprxPos;
}


int isPagePresent(int prevNext) {
   if(prevNext == 0) return di.curPage >= 1; // There is no Page 0;
   else if(prevNext == -1) return di.curPage > 1;
   else if(prevNext == 1) return di.offsets[di.curPage] < di.totalSize;
   return 0;
}

char* readSanity(char* filename, long totalSize) {
   long curpos, length, i, readSize=0;
   FILE *stream;char *docPtr;

   i = 0;
   while( totalSize > (MAXROWCHARS*(i+1)) && i < 3 ){
      readSize += MAXROWCHARS; // consider top 3 rows
      i++;
   }

   // only few chars left, so read them
   if( (totalSize-readSize) < MAXROWCHARS ) readSize = totalSize;

   docPtr = (char*)malloc(readSize);
   logmsg("Sanity check mem PTR:%ld\n", (long)docPtr);
   stream = fopen(filename,"r");
   if(stream){
      fread(docPtr,sizeof(char),readSize,stream);
      fclose(stream);
   }
   
   return docPtr;
}

char* readEntire(char* fileName){
   char temp[25],lastLine[MAXROWCHARS], *lastPtr;
   FILE *stream;
   long readSize,curpos, totalSize = filesize(fileName);
   
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
      fread(book,sizeof(char),totalSize,stream);
      curpos = ftell(stream);

      curpos = lineAtPos(stream, curpos, totalSize, lastLine);
      lastPtr = (strlen(lastLine) > 0) ? lastLineInChunk(book, lastLine) : NULL;

      if(lastPtr){
	      if(!strstr(lastPtr, lastLine))   memcpy(lastPtr, lastLine, strlen(lastLine));
			lastPtr[strlen(lastLine)] = 0;
         //logmsg("lastPtr:%s\n", lastPtr);
      }

	   fclose(stream);
   }

   logmsg("BuffSZ:%ld, DataSZ:%d, mem PTR:%ld\n", totalSize, strlen(book), (long)book);
   return book;
}

long offsetVerification(FILE *stream,long offset, char* data) {
   char tmp[7],*ptr;
   int i = 0;
   if(offset > 0){
      // 5 chars checked
	   fseek(stream, offset, SEEK_SET);
	   fread(tmp,sizeof(char),6,stream);
      tmp[6]=0;
      if(iscntrl( tmp[0]) ){
         i--;
         trimJunk(tmp);
      }

      //logmsg("offset is at %s, but actually %s \n", data, tmp);

	   if(strstr(data, tmp)){
         ptr = strstr(data, tmp);
         i += (ptr - data);
      }
   }
   
   return offset-i;
}

long offsetCorrection(FILE *stream,long offset) {
   char tmp[15],*ptr;
   int i;
   if(offset > 0){
      // Range : -5 to +10 chars offset correction for newline without return
      offset-=5;
	   fseek(stream, offset, SEEK_SET);
	   fread(tmp,sizeof(char),15,stream);
      tmp[14]=0;
      
      // reverse correction
	   i = 9;
	   while( i>=0 && isprint(tmp[i]) )i--;
      if(i == 9) {
         // it seems there is no data after the offset, can be EOF
         i = 0;
	      while( i<10 && isprint(tmp[i]) )i++;
      }
	   i++;
	   offset += i;
      offset = offsetVerification(stream, offset, &tmp[i]);
   }
   return offset;
}

void sweepData(){
   char *lastPtr;
   int i=0, len ;
   
   lastPtr = book;
   len = strlen(book);
   while( i < len && 
      (isprint(lastPtr[i]) || (iscntrl(lastPtr[i]) && 
         (lastPtr[i] == '\r'||lastPtr[i] == '\n') ) ) ) i++;

   // If char is ETX/ACK/ENQ/BEL
   if(i > 0 && i < len) {
      lastPtr[i] = 0;
   }
}

char* readPartial(char* fileName, int prevNext /*-1 is prev, +1 is next, 0 curr*/, long* nxtOffset){
   long curpos,actPos, length, readSize, newSize = 0, tpos, k, offsetA, offsetB, partialSize=0, totalSize  ;
   FILE *stream;
   char *pencil,*marginL,*marginR, tmp[MAXROWCHARS], lastLine[MAXROWCHARS], *lastPtr;
   int lines = 0, i, reallocations=0, charCount, endReach=0,iter=0, maxIterations=50, tmpLen, pageNo =0 ;

   totalSize = filesize(fileName);
   logmsg("file:%s, di.FileName:%s, page:%d \n",fileName, di.fileName, prevNext);
   if((di.curPage <= 1 && prevNext == -1) || (di.offsets[di.curPage] == totalSize && prevNext == 1) ) 
      return NULL;   

   // Book keeping
   di.totalSize = totalSize;
   if(strcmp(di.fileName, fileName) != 0) {
      strcpy( di.fileName , fileName);
      di.curPage = pageNo = 1;         // Used
      offsetA = offsetB = 0;           // Used
      for(i=0; i< 12; i++) di.offsets[i] = 0; // clearing...
   } else {
      pageNo = di.curPage + prevNext;
      di.curPage = pageNo;
      offsetA = di.offsets[pageNo - 1];// Used
      offsetB = di.offsets[pageNo];    // Book keeping only
      partialSize = di.partialSize;    // Used
   }

   readSize = bookSZ;
   if(totalSize < readSize) {
      bookSZ = readSize = partialSize = totalSize;
   } else {
      if(partialSize < readSize) partialSize = readSize;
      else bookSZ = readSize = partialSize;
   }
   
   logmsg("offset:(%ld,%ld), size:(%ld/%ld), mem PTR:%ld\n",offsetA, offsetB,readSize, totalSize, (long)book);
   if(offsetA >= totalSize) return NULL;
   if(!book)	book = (char*)calloc(readSize,sizeof(char));
   else 		book = (char*)realloc(book, readSize);

   stream = fopen(fileName,"r");
   if(!stream) return NULL;
   
   offsetA = offsetCorrection(stream,offsetA);
   iter = 0;

	do{
	   fseek(stream, offsetA, SEEK_SET);
      fread(book,sizeof(char),readSize,stream);
      actPos = ftell((FILE*)stream);
      curpos = lineAtPos(stream, actPos, totalSize, lastLine);

      logmsg( "Last Line at Pos %ld : %s\n", curpos, lastLine );
      lastPtr = (strlen(lastLine) > 0) ? lastLineInChunk(book, lastLine) : NULL;

      if(lastPtr){
		 if( strlen(lastPtr) < strlen(lastLine)  ) {
            // seems like the line is incomplete, which can be discarded
            lastPtr--;
            lastPtr[0] = 0;
            logmsg( "Chunk ends with partial line\n" );
         }
         else {
	         if(!strstr(lastPtr, lastLine)) memcpy(lastPtr, lastLine, strlen(lastLine));
			   lastPtr[strlen(lastLine)] = 0;
            logmsg( "Chunk ends like : %s, book length: %d\n", lastPtr, strlen(book) );
         }
      } else {
         //memcpy(tmp,&book[strlen(book)-200],200 );
         logmsg( "Chunk not matching last line \n", tmp );
         sweepData();
      }

	  length = strlen(book);	  
	  tmpLen = 0;
      charCount = 0;
	  
	  // offset correction 2 : data correction after read
	  while( iscntrl( book[tmpLen] ) ) {tmpLen++;offsetA++;charCount++;}
	  if( tmpLen > 0) movmem(&book[tmpLen], &book[0], (length-tmpLen+1) );

	  pencil = book;
	  lines = 0;tmpLen=0;

      while(pencil && strlen(pencil)>0  && lines < MAXROWS) {
         marginL = pencil;
         pencil = strchr(pencil, '\n');

         if (pencil) {
		      marginR = pencil;
		      pencil++;
		      charCount += (marginR - marginL) + 1;
		      lines++;
			  
			  //memcpy( tmp, marginL, 25);tmp[25]=0;
			  //if(lines == MAXROWS) logmsg( "Line: %s\n",tmp );
		   } else {
            if( actPos == totalSize ) {
			      marginR = marginL;
               trimJunkRight(marginL);
               tmpLen = strlen(marginL);
		         if(tmpLen > 0 )	{
                charCount += tmpLen;
				    marginR = &marginL[tmpLen];
			       lines++;
		         }
            }
         }
      }

	   curpos = offsetA + charCount;
	   if( lines < MAXROWS) endReach = (actPos == totalSize) ? 1 : 0;
	  
	   logmsg( "OFF: %ld,CH:%d, LN:%d, CP:%ld, TZ:%ld, Endreach:%d \n",offsetA,charCount,lines, curpos, totalSize, endReach);
	  
	   if( lines < MAXROWS  &&  !endReach ){
         setmem(book,readSize,' ');
		   newSize = abs( ( ((double)MAXROWS/(double)lines)) * (double)readSize ) + abs((double)readSize/(double)10 /*little extra*/);
		   readSize = newSize;
		   book = (char*)realloc(book, newSize);
		   reallocations++;
	   } else {
		   break;
		   // We have read all the lines required
      }
	   iter++;
	} while(iter < maxIterations);

    if(partialSize < readSize) partialSize = readSize;
	
	if(endReach) {
		offsetB = totalSize; // no data pending to be read.
	}
	else {
	  tmpLen = (marginR - marginL);
	  memcpy(lastLine, marginL, tmpLen);
	  trimJunk(lastLine);
	  offsetB = linePosition(stream,lastLine, curpos);
	  i = 0;
	  while( marginR[i] != '\n' && marginR[i] != 0 ) i++;
	  offsetB += tmpLen + i + 1; /* skip newline and point to start of next line */
	  marginR += i;
	  trimJunk(marginR);

     offsetB = offsetCorrection(stream,offsetB);
     if(offsetB >= totalSize) offsetB = totalSize;
     if(marginR) {
      setmem(marginR,strlen(marginR),' ');
      marginR[0]=0;
     }
	}
   
    fclose(stream);

    // Book keeping
    logmsg( "Reallocs : %d,lines: %d, max:%d, page:%d, offset(%ld,%ld)\n", reallocations, lines , MAXROWS, pageNo, offsetA, offsetB);
    di.partialSize = partialSize;
    di.offsets[pageNo - 1] = offsetA ;
    di.offsets[pageNo] = offsetB ;
    *nxtOffset = offsetB;
    strcpy(di.fileName, fileName);

    return book;
}

void copyStream(long fromOff, FILE *fromStre, long fromMax, FILE *to ) {
   int iter=0,length = MAXROWCHARS;
   long curpos;
   char tmp[MAXROWCHARS+1];

   if(fromOff > 0L) fromOff = offsetCorrection(fromStre, fromOff);
   curpos = fromOff;
   fseek(fromStre, curpos, SEEK_SET);

   do{
      if( fromMax - curpos < MAXROWCHARS) {
         blankOut(tmp,MAXROWCHARS);
         length = fromMax - curpos;
      }

      fread(tmp,sizeof(char),length,fromStre);
      curpos = ftell(fromStre);
      if(curpos >= fromMax) {
         trim(tmp);
         length = strlen(tmp);
      }
      fwrite(tmp, sizeof(char), length, to);
      iter++;
   } while(curpos < fromMax && iter<1000);

   if(isprint(tmp[length-1]))
      fwrite("\n", sizeof(char), 1, to);
   
   logmsg("fromOff:%ld, curpos:%ld, totalCopied:%ld, iter: %d\n", fromOff, curpos, fromMax, iter);
}


void writePartial(char* fileName,char* dataFile ,long offset, long nxtOffset, long* newOffset ){
   long curpos=0L, length, readSize, totalSize  ;
   FILE *stream, *duplStream, *modStream;
   char duplFile[15] = "tcdupl.tmp" ;
   int iter=0;

   // step 1: Save the original to a tmp file
   totalSize = filesize(fileName);
   if(totalSize == 0) return;
   logmsg("origninal offset:%ld,file size:%ld\n", offset, totalSize);
   
   stream = fopen(fileName,"r");
   duplStream = fopen(duplFile, "w");
   copyStream(0L, stream, totalSize, duplStream );
   fclose(stream);
   fclose(duplStream);

   // step 2: Write the fresh data up to the offset
   totalSize = filesize(duplFile);
   if(totalSize == 0) return;
   stream = fopen(fileName,"w");
   duplStream = fopen(duplFile, "r");
   copyStream(0L, duplStream, offset, stream );

   // step 3: Write the changed data that is fed, continuing from offset
   readSize = filesize(dataFile);
   modStream = fopen(dataFile, "r");
   fseek(stream, offset, SEEK_SET);
   copyStream(0L, modStream, readSize, stream );
   fclose(modStream);
   curpos = ftell(stream);
   *newOffset = curpos;

   // step 4: Write the rest of the unbuffered data
   if(nxtOffset < totalSize && curpos < totalSize ){
      copyStream(nxtOffset, duplStream, totalSize, stream );
   }

   fclose(duplStream);
   fclose(stream);
   logmsg("curpos:%ld,file totalSize:%ld, iter: %d\n", curpos, totalSize, iter);
   logmsg("finished making changes.\n");
}