/* Turbo C++ - (C) Copyright 1987-1991 by Borland International */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mem.h>
#include <conio.h>
#include <string.h>
#include "tcalc.h"

const char nmbers[12] = "0123456789.";
const char delimitchars[3] = ",;|";
char delimiter = ';';

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

int attribtype(char* data) {
   char *loc;
   int i=0, type = VALUE;

   if(!data || strlen(data) == 0) return TEXT;

   // Check if the type is not numeric
   for( i=0; i<strlen(data); i++ ){
    if(strchr(nmbers, data[i]) == NULL) {
       // Check if the text type is a formula
	    if(strchr(data, ':' ) != NULL)
	      type = FORMULA;
	    else if(strchr(data, '(' ) != NULL && strchr(data, ')' ) != NULL )
         type = FORMULA;
       else 
         type = TEXT;
       break;
      }
   }

   return type;
}

int trimzeros(char *buff){
   int length = strlen(buff);
   while(buff[length-1] == '0') {  buff[length-1] = '\0'; length--;  }
   if(buff[length-1] == '.') buff[length-1] = '\0';
   return -1;
}

/* strtok method requires delimitors separated by space, so add gaps */
void insertspace(char *vstring){
  int pos, found = 0, foundtwice = 0;
  char debug[25];
  if(!vstring || strlen(vstring) == 0) return;

  pos = strlen(vstring);
  while (pos > 0)
  {
   pos -= 1;
   if (vstring[pos] == delimiter){
    if(found == 1) foundtwice = 1;
    else found = 1;
   }
   else found = 0;

   if (foundtwice == 1) {
    movmem(&vstring[pos+1], &vstring[pos+2], strlen(vstring) - pos + 1);
    vstring[pos+1] = ' ';
    foundtwice = 0;
   }
  }
  // First column
  if (found == 1) {
   movmem(&vstring[pos], &vstring[pos+1], strlen(vstring) -pos + 1);
   vstring[pos] = ' ';
  }
  //sprintf(debug, "found:%d,Pos:%d", found, pos );
  //trace(debug);
}

char finddelimiter(char *sheetheader, int *count){
   int length = strlen(sheetheader), tmplen=0, tmpcount=0, delimcount=0, i;
   char delim = delimitchars[0];
   for( i=0; i<strlen(delimitchars); i++ ){
      tmplen = length;
      tmpcount = 0;
      while(tmplen>-1) {  
         if( sheetheader[tmplen-1] == delimitchars[i] ) tmpcount++; 
         tmplen--;  
      }
      if(delimcount < tmpcount){
         delimcount = tmpcount;
         *count = tmpcount;
         delim = delimitchars[i];
      } 
   }
   
   return delim;
}

char* readentire(char* fileName){
   int num,bytes;

   long buffSize = filesize(fileName);
   //char *docPtr = (char*)malloc(buffSize + MEMPIT);
   char *docPtr = (char*)calloc(buffSize , sizeof(char));

   FILE *stream;
   stream = fopen(fileName,"r");
   if(stream){
      fread(docPtr,sizeof(char),buffSize,stream);
	   fclose(stream);
      //free(docPtr);
   }
   return docPtr;
}

void loadcsvfile(char* fileName){
   int i=0, j=0, maxI=0, maxJ=0, allocated, delimcount,dummy;
   struct CELLREC rec;
   char* doc = readentire(fileName);
   char *ptr,*temp, nLine[2] = "\n", delim[2] = ";", debug[25];
   char* recs[MAXROWS];

   // If the doc contains no data
   if(!doc || strlen(doc) == 0) return;
   
   // Split Row-wise starting from header
   ptr = strtok(doc, nLine);
   trace(NULL);

   // Identify the delimiter from the sheet header
   delimiter = finddelimiter(ptr, &delimcount);
   sprintf(debug, "column count : %d", delimcount);
   trace(debug);
   strset(delim, delimiter);

   recs[0] = (char*)malloc( strlen(ptr) + delimcount + MEMPIT );
   strcpy(recs[0], ptr);
   insertspace(recs[0]);
   ptr = strtok(NULL, nLine);
   i++;
   
   while (ptr != NULL) {
      recs[i] = (char*)malloc( strlen(ptr) + delimcount + MEMPIT );
      strcpy(recs[i], ptr);
      insertspace(recs[i]);
      ptr = strtok(NULL, nLine);
      i++;

      if(maxI < i-1) maxI = i-1;
   }
   free(doc);

   // Split Column-wise
   for(i=0; i<= maxI; i++) {
      j=0;
      temp = recs[i];
      temp = strtok(temp, delim);
      temp = trim(temp);
      currow = i;

      do{
         rec.attrib = attribtype(temp);
         curcol = j;
         
         switch (rec.attrib)
         {
          case TEXT :
           strcpy(rec.v.text, temp);
           if ((allocated = alloctext(curcol, currow, rec.v.text)) == TRUE)
            setoflags(curcol, currow, NOUPDATE);
           break;
          case VALUE :
           rec.v.value = atof(temp);
           allocated = allocvalue(curcol, currow, rec.v.value);
           break;
          case FORMULA :
           strcpy(rec.v.f.formula, temp);
           rec.v.f.fvalue = parse(rec.v.f.formula, &dummy);
           allocated = allocformula(curcol, currow, rec.v.f.formula, rec.v.f.fvalue);
           break;
         }

         format[curcol][currow] = DEFAULTFORMAT;
         lastrow = currow;
         lastcol = curcol;
         if (!allocated)
         {
          errormsg(MSGFILELOMEM);
          if(curcol == 0) {
            lastrow = currow-1;
            lastcol = maxJ;
          }
          break;
         }

         // Next field in the row
         temp = strtok(NULL, delim);
         j++;
      } while (temp != NULL);

      if (!allocated) break; // allocation didn't happen
      if(maxJ < j-1) maxJ = j-1;
      free(recs[i]);
   }
}

/* Saves the current spreadsheet */
void savecsvfile(char* fileName)
{
  char record[MAXROWCHARS] ="", valBuff[MAXVALCHARS] = "", delim[2]=";";
  int col, row, overwrite, file;
  CELLPTR cellptr;
  FILE *stream;
  int rows = lastrow, cols = lastcol;

  // The delimiter character is identified
  strset(delim, delimiter);

  stream = fopen(fileName, "w+");

  for (row = 0; row <= rows; row++)
  {
   strcpy(record , "");

   // Form the record
   for (col = 0; col <= cols; col++)
   {
    cellptr = cell[col][row];
    if (cellptr != NULL)
    {
     switch(cellptr->attrib)
     {
      case TEXT :     strcat(record, cellptr->v.text);      break;
      case VALUE :
        strcpy(valBuff, doubletostr(cellptr->v.value));
        strcat(record, valBuff);
        break;
      case FORMULA :  strcat(record, cellptr->v.f.formula);      break;
      default : break;
     }
    }
    if(col < cols) strcat(record, delim);
   }
   strcat(record, "\n");

   // Write the record
   fwrite(record, strlen(record), 1, stream);
 }
 fclose(stream);
}

