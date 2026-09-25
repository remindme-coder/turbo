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
   long orig = ftell(stream), tmpPos, beginPos,wall;
   
   //blankOut(tmp, MAXROWCHARS);

   if(apprxPos < 0) {
      strcpy(line, "");
      *endPos = 0;
      return;
   }
   else if( totalSize - (apprxPos+3) < 3 /* Adjustment */) {
      // The requested line is at the end of the file
      beginPos = totalSize - MAXROWCHARS;
      if( beginPos < 0) beginPos = 0;
      
      fseek(stream, beginPos  , SEEK_SET);
      fread(tmp,sizeof(char),MAXROWCHARS,stream);
      trimJunk(tmp);
      
      lastI = i = strlen(tmp) - 1;
      pass = 0;
      while( i > 0 ) { 
         if(isNewLn(tmp[i])) {
            if( pass > 3) break; else pass=0; // Adjustment: A lne should have atleast 3 chars
         } 
         if(isprint(tmp[i])) {lastI = i; pass++;}
         i--;
      }
      i = exact = lastI;

      range = strlen(&tmp[lastI]);
      tmpPos = (totalSize - range);
      movmem(&tmp[lastI],&tmp[0], range );
      tmp[range] = 0;
      ptr = strchr(tmp,'\n');
      if(ptr) ptr[0] = 0;
      logmsg("Temp pos is %ld, strlen:%d, str:%s\n", tmpPos, range, tmp);

      // Correct tmpPos
      pass=0;
      tmpPos += 3;// Adjustment to identify correct start
      fseek(stream, tmpPos, SEEK_SET);
      fread(ch,sizeof(char),1,stream);
      while(isprint( ch[0]) && (tmpPos-1) > 0 ){
         fseek(stream, --tmpPos, SEEK_SET);
         fread(ch,sizeof(char),1,stream);
         pass = 1;
      }
      if(pass) tmpPos++;
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
               tmpPos += (occur1 - &tmp[0]);// beginning of line
               
               fseek(stream, tmpPos, SEEK_SET);
               fread(ch,sizeof(char),1,stream);
               while(ch[0] != occur1[1] /*start of line char*/ ){
                  fseek(stream, ++tmpPos, SEEK_SET);
                  fread(ch,sizeof(char),1,stream);
               }
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
               
               while(iscntrl( occur1[0])) occur1++;
               i = occur1 - &tmp[0];
               
               tmpPos = tmpPos + i;// beginning of line
               fseek(stream, tmpPos, SEEK_SET);
               fread(ch,sizeof(char),1,stream);
               while(ch[0] != occur1[0] ){
                  fseek(stream, ++tmpPos, SEEK_SET);
                  fread(ch,sizeof(char),1,stream);
               }
               wall = tmpPos;
            }
         }
         
         range *= 2;
         if(left) tmpPos = wall - range + 1; //include new line char too
      } while( iter < 50 );

      trimJunk(tmp);
      lastI = strlen(tmp) - 1;
   }
   
   if (line != NULL){
      strcpy(line, tmp);
      trimJunk(line);
      //logmsg("Extracted line:%s at Pos:%ld\n", tmp, tmpPos);
   }

   // Find the next line position
   i = strlen(tmp);
   *startPos = tmpPos; // start of line position
   tmpPos += i;
   //logmsg("Tmppos:%ld , TZ:%ld, orig:%ld\n", tmpPos, totalSize, orig);
   if( tmpPos < totalSize ){
      fseek(stream, tmpPos, SEEK_SET);
      fread(ch,sizeof(char),1,stream);
      while(iscntrl( ch[0]) && (tmpPos+1) < totalSize ){
         fseek(stream, ++tmpPos, SEEK_SET);
         fread(ch,sizeof(char),1,stream);
      }
   }

   fseek(stream, orig, SEEK_SET);
   *endPos = tmpPos; // end of line position
}

int isPagePresent(int prevNext) {
   if(prevNext == 0) return di.curPage >= 1; // There is no Page 0;
   else if(prevNext == -1) return di.curPage > 1;
   else if(prevNext == 1) return (di.totalSize - di.offsets[di.curPage]) > 3  ;
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
   strcpy( di.fileName , fileName);
   di.curPage = 1;         // Used
   for(i=0; i< 12; i++) di.offsets[i] = 0; // clearing...
   di.offsets[1] = offset;

   return readPartial(fileName , prevNext , nxtOffset);
}

char* readPartial(char* fileName , int prevNext /*-1 is prev, +1 is next, 0 curr*/, long* nxtOffset){
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
   logmsg("offset:(%ld,%ld), page:%d, size:(%ld/%ld)\n",offsetA, offsetB,pageNo, (long)readSize, (long)totalSize);
   
   if(!book)	book = (char*)calloc(readSize+1,sizeof(char));
   else 		book = (char*)realloc(book, readSize+1);

   setmem(book,readSize,' ');
   stream = fopen(fileName,"r");
   if(!stream) return NULL;
   strcpy(book , NULL);

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
   dataSZ = allocSZ;
   remCount = 0;//Unnecessary lines
   xtraData = pencil - lastPtr;

   pencil = lastPtr ; // Last char of the book
   endPtr = lastPtr;
   if(curPos-offsetA > dataSZ) xtraCh = (float)(curPos-offsetA-dataSZ)/(float)MAXROWS;

   logmsg("lines: %d, with data %d, max %d, Pos:%ld, book:%d chars\n", lines, dataSZ, allocSZ, curPos, strlen(book));
   logmsg("Differences Pos:%d, Data:%d, xtraCh:%1.1f, xtraData:%d \n",  (int)(curPos-offsetA), dataSZ, (float)xtraCh, (int)xtraData);

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

      if(curPos >= totalSize){
         // No need to worry about extra spaces
         trim(lastPtr);
         xtraData = strlen(lastPtr)-1;// exclude new line
         curPos = totalSize;
      } else {
         // Calculate the length added by xtra lines
         tmpCalc = (float)remCount * (float)xtraCh;
         xtraData += (int)(endPtr - lastPtr) + (int)ceil(tmpCalc);
      }

      logmsg("lastPtr - endPtr : %d, xtraData : %d,curPos:%ld \n", (int)(endPtr - lastPtr), (int)xtraData, (long)curPos);
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
   logmsg( "page:%d, offset(%ld,%ld), book:%d chars\n", pageNo, offsetA, offsetB, strlen(book));
   di.partialSize = dataSZ;
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

   //if(fromOff > 0L) fromOff = offsetCorrection(fromStre, fromOff);
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

   // step 1: Save the original to a tmp file
   totalSize = filesize(fileName);
   if(totalSize <= 0) {
      logmsg("File :%s, has 0 data\n", fileName);
      return;
   }
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
   if(nxtOffset < totalSize){
      copyStream(nxtOffset, duplStream, totalSize, stream );
   }

   fclose(duplStream);
   fclose(stream);
}