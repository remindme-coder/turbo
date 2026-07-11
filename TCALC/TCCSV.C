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

char finddelimiter(char *sheetheader){
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
   int i=0, j=0, maxI=0, maxJ=0, allocated, dummy;
   struct CELLREC rec;
   char* doc = readentire(fileName);
   char *ptr,*temp, nLine[2] = "\n", delim[2] = ";";
   char* recs[MAXROWS];

   // If the doc contains no data
   if(!doc || strlen(doc) == 0) return;

   // Split Row-wise starting from header
   ptr = strtok(doc, nLine);

   recs[0] = (char*)malloc( strlen(ptr) + MEMPIT );
   strcpy(recs[0], ptr);
   ptr = strtok(NULL, nLine);
   i++;
   
   // Identify the delimiter from the sheet header
   delimiter = finddelimiter(recs[0]);
   strset(delim, delimiter);

   while (ptr != NULL) {
      recs[i] = (char*)malloc( strlen(ptr) + MEMPIT );
      strcpy(recs[i], ptr);
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
      case TEXT :     strcat(record, (cellptr->v.text == "" ? " ": cellptr->v.text));      break;
      case VALUE :
        sprintf(valBuff,"%3.2lf", cellptr->v.value);
        trimzeros(valBuff);
        strcat(record, valBuff);
        break;
      case FORMULA :  strcat(record, cellptr->v.f.formula);      break;
      default : strcat(record, " ");
     }
    }
    else
    {
     strcat(record, " ");
    }
    if(col < cols) strcat(record, delim);
   }
   strcat(record, "\n");

   // Write the record
   fwrite(record, strlen(record), 1, stream);
 }
 fclose(stream);
}

