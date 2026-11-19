/* Turbo C++ - (C) Copyright 1987-1991 by Borland International */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mem.h>
#include <conio.h>
#include <string.h>
#include <values.h>
#include "tcalc.h"
#include "tcfile.h"

const char nmbers[13] = " 0123456789.\0";
const char spcl[30] = "-,.;|'\"?\\/[]<>!@#$%&*_+\0";
const char delimitchars[3] = ",;|";
const char quot = '"';
const char curly[2] = "{}";
char delimiter = ';';
int delimCount = 0;

int columnChanged = FALSE;
int colDeleted = -1;
int colInserted = -1;

void getField(char *line, char* field, int lineLength, int col) {
  int start,end,quoted,pos = 0, colItr = 0, wdth = 0;
  blankOut(field, 10);

  while(pos < lineLength) {
   start = pos;
   end = pos;
   quoted=FALSE;

   // skip data without quotes
   while( pos < lineLength && line[pos] != quot && line[pos] != delimiter && line[pos] != '\n' ) pos++;

   if(pos < lineLength && line[pos] == quot) {
    pos++;start = pos;quoted=TRUE;
    // just surf the data
    while( pos < lineLength && line[pos] != quot ) {pos++;end=pos;}
    pos++;

    // skip data without quotes
    while( pos < lineLength && line[pos] != delimiter && line[pos] != '\n' ) pos++;
   }

   if(pos < lineLength) {
    if(colItr == col){
      if(!quoted) end = pos;
      wdth = end - start;
      wdth = (wdth > 10) ? 10 : wdth;
      memcpy( field, &line[start], wdth );
      concatSpace(field, wdth, 10-wdth, LEFT);
      break;
    }
    if( line[pos] == delimiter) {
      colItr++;pos++;
    } else if (line[pos] == '\n') {
      colItr = 0;
      pos = 0;
      break;
    }
   }
  }
}

void deleteField(char *line, int length, int col) {
  int start,pos = 0, colItr = 0;
  start = pos;

  while(pos < length) {

   // skip data without quotes
   while( pos < length && line[pos] != quot && line[pos] != delimiter && line[pos] != '\n' ) pos++;

   if(pos < length && line[pos] == quot) {
    pos++;
    // just surf the data
    while( pos < length && line[pos] != quot ) pos++;
    pos++;

    // skip data without quotes
    while( pos < length && line[pos] != delimiter && line[pos] != '\n' ) pos++;
   }

   if(pos < length) {
    if(colItr == col){
      if(line[pos] == '\n') movmem(&line[pos], &line[start-1], length - (pos) + 1);
      else movmem(&line[pos+1], &line[start], length - (pos+1) + 1);
      break;
    }
    if( line[pos] == delimiter) {
      colItr++;pos++;
      start = pos;
    } else if (line[pos] == '\n') {
      logmsg("pos %d, char %c, colItr %d, col %d\n", pos,line[pos], colItr, col );
      colItr = 0;
      pos = 0;
      break;
    }
   }
  }
}

void insertField(char *line, int length, int col) {
  int start,pos = 0, colItr = 0;
  start = pos;

  while(pos < length) {

   // skip data without quotes
   while( pos < length && line[pos] != quot && line[pos] != delimiter && line[pos] != '\n' ) pos++;

   if(pos < length && line[pos] == quot) {
    pos++;
    // just surf the data
    while( pos < length && line[pos] != quot ) pos++;
    pos++;

    // skip data without quotes
    while( pos < length && line[pos] != delimiter && line[pos] != '\n' ) pos++;
   }

   if(pos < length) {
    if(colItr == col){
      movmem(&line[start], &line[start+1], length - start + 1);
      line[start] = delimiter;
      break;
    }
    if( line[pos] == delimiter) {
      colItr++;pos++;
      start = pos;
    } else if (line[pos] == '\n') {
      colItr = 0;
      pos = 0;
      break;
    }
   }
  }
}

int modifyLines( char *tmp ) {
  char *ptrSt,*pencil,*ptr,ch[1];
  int length = strlen(tmp), tmpLen, lines;

  // Identification purpose check
  if( colDeleted < 0 && colInserted < 0 ) return length;

  // Extract Lines
  ptrSt = &tmp[0];
  pencil = ptrSt;
  ptr = strchr(tmp, '\n');

  if(!ptr){
    length = strlen(tmp);

    if(colInserted > -1)  insertField(tmp, length,  colInserted);
    else if(colDeleted > -1)  deleteField(tmp, length,  colDeleted);

    length = strlen(tmp);
  } else {
    // Apply schema modifications
    pencil = &tmp[0];
    tmpLen = strlen( pencil );

    while( pencil && tmpLen > 0 ){
       if(colInserted > -1)  insertField( pencil, tmpLen, colInserted);
       else if(colDeleted > -1)  deleteField( pencil, tmpLen, colDeleted);

       ptr = strchr(pencil, '\n');
       while( ptr && isNewLn(ptr[0]) ) ptr++;
       lines++;
       pencil = ptr;
       tmpLen = strlen( pencil );
    }
    length = (pencil-ptrSt);
  }
  return length;
}

void insertCSVCol(int col) {
   colInserted = col;
   columnChanged = TRUE;
   saveRequired = TRUE;
}

void deleteCSVCol(int col) {
   colDeleted  = col;
   columnChanged = TRUE;
   saveRequired = TRUE;
}

int isnumeric(char *data){
   int i=0, nu = FALSE;
   if(!data || strlen(data) == 0) return FALSE;
   for( i=0; i<strlen(data); i++ ){
      if(!strchr(nmbers, data[i])) return FALSE;
      else if(data[i] != ' ') nu = TRUE;
   }
   return nu ? TRUE : FALSE;
}
int attribtype(char* data) {
   int i=0, hasspace=0, hasbrackets=0, hascolon=0, hasalnum=0, haspunc=0;
   
   if(!data || strlen(data) == 0) return TEXT;
   
   // Check if the text is numeric
   if(isnumeric(data)) return VALUE;
   
   // check if the text is alpha numeric / space/ punc
   for( i=0; i<strlen(data); i++ ){
      //if( isalpha(data[i]) || isdigit(data[i]) || strchr("-,.;|\"?\\/[]<>!@#$%&*_+", data[i]))
	  if(isspace(data[i])) hasspace = 1;
	  else if(isalnum(data[i])) hasalnum = 1;
	  else if(strchr(spcl, data[i])) haspunc = 1;// everything but :()
	  else if(data[i]==':') hascolon = 1;
	  else if(strchr("()", data[i])) hasbrackets = 1;
   }
   
   if(!haspunc && !hasspace ){
	  // AB:B5
	  if(!hasbrackets && hasalnum && hascolon ) return FORMULA;
	  // SQRT(2)
	  if(!hascolon && hasalnum && hasbrackets ) {
		  if(strchr(data, '(') && strchr(data, ')'))	return FORMULA;
		  else return TEXT;
	  }
   }

   return TEXT;
}

int trimzeros(char *buff){
   int length = strlen(buff);
   while(buff[length-1] == '0') {  buff[length-1] = '\0'; length--;  }
   if(buff[length-1] == '.') buff[length-1] = '\0';
   return -1;
}


void unmaskStr(char *vstring) {
   int pos;
   if(!vstring || strlen(vstring) == 0) return;

   pos = strlen(vstring);
   while (pos >= 0)
   {
    pos -= 1;
    if (vstring[pos] == '*'){
     vstring[pos] = delimiter;
    }
   }
}

void maskStr(char *vstring) {
  int length, pos = 0;

  pos = strlen(vstring) - 1;
  while(pos > 0) {

   // skip data without quotes
   length = strlen(vstring);
   while( pos >= 0 && vstring[pos] != quot ) pos--;

   // skip end quotation for quoted data
   if(pos >= 0 && vstring[pos] == quot) {
    movmem(&vstring[pos+1], &vstring[pos], length - pos + 1);
    length--;
    pos--;

    // surf the data and mask the delimiters
    while( pos >= 0 && vstring[pos] != quot ) {
     if( vstring[pos] == delimiter ) vstring[pos] = '*';
     pos--;
    }

    // skip start quotation for quoted data
    if(pos >= 0 && vstring[pos] == quot) {
     movmem(&vstring[pos+1], &vstring[pos], length - pos + 1);
     length--;
     pos--;
    }

    // skip data without quotes
    while( pos >= 0 && vstring[pos] != quot ) pos--;
   }
  }
}

/* strtok method requires delimitors separated by space, so add gaps */
void insertspace(char *vstring){
  int pos, found = 0, foundtwice = 0,length;
  char debug[25];
  if(!vstring || strlen(vstring) == 0) return;

  maskStr(vstring);// This will escape the delimiters within quotations
  length = strlen(vstring);
  pos = length;
  while (pos > 0)
  {
   pos -= 1;
   if (vstring[pos] == delimiter){
    if(found == 1) foundtwice = 1;
    else found = 1;
   }
   else found = 0;

   if (foundtwice == 1) {
    movmem(&vstring[pos+1], &vstring[pos+2], length - pos + 1);
    length++;
    vstring[pos+1] = ' ';
    foundtwice = 0;
   }
  }
  // First column
  if (found == 1) {
   movmem(&vstring[pos], &vstring[pos+1], length - pos + 1);
   length++;
   vstring[pos] = ' ';
  }
  //sprintf(debug, "found:%d,Pos:%d", found, pos );
  //trace(debug);
}

char finddelimiter(char *sheetheader, int *count){
   int length = strlen(sheetheader), tmplen=0, tmpcount=0, dcount=0, i, skip = 0;
   char delim = delimitchars[0];
   for( i=0; i<strlen(delimitchars); i++ ){
      tmplen = length;
      tmpcount = 0;
      while(tmplen>-1) {  
         tmplen--;  
         /* format information arent considered */
         if( curly[1] == sheetheader[tmplen] ) skip = 1;
         if( skip == 1 ) {
            if( curly[0] == sheetheader[tmplen] ) skip = 0;
            continue;
         }

         if( sheetheader[tmplen] == delimitchars[i] ) tmpcount++; 
      }
      if(dcount < tmpcount){
         dcount = tmpcount;
         *count = tmpcount;
         delim = delimitchars[i];
      } 
   }
   
   return delim;
}

void findcolumformat(char *sheetheader){
   int length = strlen(sheetheader), colinx=0, i, scrptinx = 0, skip = 0, scrptlen, proplen, accumlen = 0;
   char fmt[25] = "",wdth[15], *pos1, *pos2;
   //trace(NULL);
   
   i = 0;
   format[colinx] = DEFAULTFORMAT;
   while(i < length) {  
      /* format information only considered */
      if( sheetheader[i] == '{' ) { 
         skip = 1;
         scrptinx = i;
      }
      if( skip == 1 ) {
         if( sheetheader[i] == '}') {
            skip = 0;
            scrptlen = i+1 - scrptinx;
            memcpy( fmt, (sheetheader + scrptinx), scrptlen );
            fmt[scrptlen] = '\0';
            
            if( strstr( fmt, "f:$" ) )       format[colinx] += (DOLLAR + COMMAS);
            else if( strstr( fmt, "f:L" ) )  format[colinx] = 0;
            else if( strstr( fmt, "f:R" ) )  format[colinx] += RJUSTIFY;
            else if( strstr( fmt, "f:C" ) )  format[colinx] += COMMAS;

            if( strstr( fmt, "w:" ) ) {
               pos1 = strstr( fmt, "w:" ) + 2;
               if( strchr(pos1,',') )        pos2 = strchr(pos1,',');
               else if ( strchr(pos1,'}') )  pos2 = strchr(pos1,'}');
               proplen = pos2 - pos1;

               memcpy( wdth, pos1, proplen );
               wdth[proplen] = '\0';// eliminate rest

               accumlen += atoi(wdth) - colwidth[colinx];
               colwidth[colinx] = atoi(wdth);// column width identified
            }

            movmem((sheetheader+i+1), (sheetheader+scrptinx), length - i - 1);
            length = length - scrptlen;
            i = i - scrptlen;

            sheetheader[length] = '\0';// eliminate rest
            //sprintf(fmt, "%d,%d", colinx, colwidth[colinx]);
            //trace( fmt );
         }
      }

      if( sheetheader[i] == delimiter ) {
         colinx++; 
         colstart[colinx] += accumlen;
         format[colinx] = DEFAULTFORMAT;
      }
      i++;
   }

   colinx++;
   while(colinx < 10) {
      colstart[colinx] += accumlen;
      colinx++;
   }
}

int validOffset( FILE *stream, long tmpPos){
   char ch[1];
   long actual = tmpPos;
   fseek(stream, tmpPos, SEEK_SET);
   fread(ch,sizeof(char),1,stream);
   while(isprint( ch[0]) && (tmpPos-1) > 0 ){
    fseek(stream, --tmpPos, SEEK_SET);
    fread(ch,sizeof(char),1,stream);
   }
   tmpPos++;
   return (actual == tmpPos ? TRUE: FALSE);
}

int validatecsvfile(char* filename){
   long curPos = 0L;
   char *ptr, *pencil, tmp[MAXROWCHARS+1], temp[25] ;
   int i = 0,j=0, length, dcount[3], chcount[3], wdth=0, overflo=FALSE /*, invRow = FALSE*/, invOff = FALSE;
   FILE *stream;

   if( memleft < 2 * 1024 ) {
      //sprintf(temp,"Buff = %ld, MemLeft = %ld", buffSize, memleft);
      //trace( temp );

      return -1; // maximum file size limitation
   }
	else {
      stream = fopen(filename,"r");
      fread(tmp,sizeof(char),MAXROWCHARS,stream);
      if(!strchr(tmp,'\n')) { fclose(stream); return -1;} // maximum record size limitation
   }

   pencil = tmp;
   ptr = strchr(pencil, '\n');

   // Identify the column alignment for the first 3 rows
   delimiter = finddelimiter(ptr, &dcount[i]);

   // Header & 2 rows sanity check
   while (ptr != NULL && i < 3) {
      length = ptr - pencil;
      curPos += length + 2;
      if(!validOffset(stream, curPos)) invOff = TRUE;
      chcount[i] = length;
      dcount[i] = 0;
      wdth=0;
      j=0;
      while(j<length) {
         if(pencil[j] == delimiter ) {
            dcount[i]++; 
            if(wdth > MAXCOLWIDTH) overflo = TRUE;
            wdth=0; 
         }
         else wdth++;
         j++;
      }

      if(wdth > MAXCOLWIDTH) overflo = TRUE;
      //if(length < MINROWCHARS) invRow = TRUE;

      pencil = ptr + 1;
      ptr = strchr(pencil, '\n');
      i++;
   }
   fclose(stream);

   // Large text
   if(overflo) return -2; // Indicates data that are huge for TCALC
   //if(invRow)  return -1; // Indicates low data for TCALC
   if(invOff)  return 0; // Indicates invalid line endings

   // Validate column alignment
   if(dcount[0] == dcount[1] ) return 1; // header matches first record
   if(dcount[0] > 0 && chcount[1] == 0 && chcount[2] == 0  ) return 1; // header but no data, which is valid

   // Invalid cases
   if( (dcount[0] > 0) && (abs(dcount[0] - dcount[1]) > 3) ) return -3; // header not marginally aligned to first record, invalid
   if( (dcount[0] > 0) && (dcount[1] != dcount[0] || dcount[2] != dcount[0]) ) return -3; // header not aligned to first or second, invalid

   // TODO: A few more can be added later...

   return 1;
}

int loadcsheader(char* record  ) {
   char *temp, delim[2] = ";";
   int j=0, allocated;
   struct CELLREC rec;

   // Validate
   if(!record || strlen(record)==0) return 1;
   insertspace(record);

   // Split column-wise
   strset(delim, delimiter);
   temp = strtok(record, delim);
   temp = trim(temp);
   j=0;

   do{
      rec.attrib = attribtype(temp);
      if(strchr(temp,'*')) unmaskStr(temp);
      strcpy(rec.v.text, temp );
      allocated = alloctext(j, -1, rec.v.text);

      lastcol = j;
      if (!allocated)
      {
       errormsg(MSGFILELOMEM);
       break;
      } else {
       //setoflags(j, row, NOUPDATE);
      }

      // Next field in the row
      temp = strtok(NULL, delim);
      j++;
   } while (temp != NULL);

   // Global variable curcol
   if(curcol < j-1) curcol = j-1;

   return allocated;
}

int loadcsvalue(char* record, int i  ) {
   char *temp, delim[2] = ";";
   int j=0, allocated, dummy, row, col;
   struct CELLREC rec;

   // Validate
   if(!record || strlen(record)==0) return 1;
   insertspace(record);

   // Split column-wise
   strset(delim, delimiter);
   j=0;
   temp = strtok(record, delim);
   temp = trim(temp);
   row = i;

   do{
      rec.attrib = attribtype(temp);
      col = j;
      
      switch (rec.attrib)
      {
      case TEXT :
      if(strchr(temp,'*')) unmaskStr(temp);
      strcpy(rec.v.text, temp );
      if ((allocated = alloctext(col, row, rec.v.text)) == TRUE)
         setoflags(col, row, NOUPDATE);
      break;
      case VALUE :
      rec.v.value = atof(temp);
      allocated = allocvalue(col, row, rec.v.value);
      break;
      case FORMULA :
      strcpy(rec.v.f.formula, temp);
      rec.v.f.fvalue = parse(rec.v.f.formula, &dummy);
      allocated = allocformula(col, row, rec.v.f.formula, rec.v.f.fvalue);
      break;
      }

      lastrow = row;
      lastcol = col;
      if (!allocated)
      {
      errormsg(MSGFILELOMEM);
      if(col == 0) {
         lastrow = row-1;
      }
      break;
      }

      // Next field in the row
      temp = strtok(NULL, delim);
      j++;
   } while (temp != NULL);

   // Global variable curcol
   if(curcol < j-1) curcol = j-1;

   return allocated;
}

void loadcsvfile(char* fileName, int prevNext /*-1 is prev, +1 is next, 0 curr*/){
   int i=0, j, allocated, len;
   long totalSize = filesize(fileName), tempMem, gridMem, nxtOffset;
   char *ptr,*prevPtr,*temp, debug[25];
   char record[MAXROWCHARS] = "";
   char* doc = NULL;

   if(totalSize > 3 * 1024 /* 3KB */ )
      doc = readPartial(fileName, prevNext, record, &nxtOffset );
   else
      doc = readEntire(fileName, record);

   // If the doc contains no data
   if(!doc || strlen(doc) == 0) return;

   tempMem = memleft ;
   if(prevNext == 0 && record != NULL && strlen(record) > 0 ){
      // Identify the delimiter from the sheet header
      delimiter = finddelimiter(record, &delimCount);
      findcolumformat(record);
      //logmsg( "column count : %d, header: %s\n", delimCount+1, record);

      #if HEADERROW == 1 
      allocated = loadcsheader(record);
      #endif
   }

   ptr = doc;
   while( ptr && i < MAXROWS ) {
      prevPtr = ptr;
      if(prevPtr[0]=='\n')prevPtr++;
      ptr = strchr(prevPtr, '\n');
      if(!ptr) {
         strcpy(record, prevPtr );
         len = strlen(prevPtr);
         record[len] = 0;
      }
      else {
         len = (int)(ptr - prevPtr);
         memcpy( record, prevPtr, len );
         record[len] = 0;
      }
      //logmsg( "Rec is : %s\n", record);

      if(len > 0){
         allocated = loadcsvalue(record, i);
         currow = i;
         lastrow = i;
         lastcol = curcol;
         if (!allocated) break; // allocation didn't happen
         i++;
      }
   }
   //free(doc);
   gridMem = tempMem - memleft;
   memgrid = (memgrid < gridMem) ? gridMem :  memgrid;
}

void borrowLine(int direction) {
   int ro,col;
   char line[MAXROWCHARS];
   long prevPos,offset,totalSZ;
   FILE *stream = NULL;

   offset = di.offsets[di.curPage];
   totalSZ = di.totalSize;

   if(lastrow == (MAXROWS-1)) {
     stream = fopen(di.fileName,"r");
     if(direction < 0) {
       // Indicates INSERT
       lineAtPos(stream, offset - 3, totalSZ, line, &prevPos,&offset );// Adjustment
       di.offsets[di.curPage] = prevPos;
	   di.offsets[di.curPage+1] = di.offsets[di.curPage+1] == 0 ? totalSZ : di.offsets[di.curPage+1];
	   //logmsg( "di.totalSize:%ld, di.curPage:%d, line pos:%ld, line end pos:%ld \n", (long)totalSZ, di.curPage, (long)prevPos, (long)offset );
     } else if(direction > 0) {
       // Indicates DELETE
       lineAtPos(stream, offset + 3, totalSZ, line, &prevPos,&offset );// Adjustment
       di.offsets[di.curPage] = offset;

       ro = currow; col = curcol;
       loadcsvalue(line, lastrow);
       currow = ro; curcol = col;
     }
     fclose(stream);
   }

   saveRequired = TRUE;
}


void savepage()
{
   char *bookPtr,headerStr[MAXROWCHARS]="",tmp[15]="tctmp.tmp";
   int page;
   long newOffset, prevPos, offset;
   page = di.curPage;

   writet(1, 25, WHITE, MSGSAVING);
   savecsvfile(tmp, /*out*/headerStr);
   //logmsg("Header is %s\n", headerStr);
   writePartial(di.fileName, tmp , di.offsets[page-1], di.offsets[page], headerStr, &newOffset );

   di.offsets[page] = newOffset;
   di.totalSize = filesize(di.fileName);

   if(columnChanged) {
    if(colDeleted > -1)	colDeleted = -1;
    if(colInserted > -1) colInserted = -1;
    columnChanged = FALSE;
   }

   changed = FALSE;
   saveRequired = FALSE;
   writet(1, 25, WHITE, "");
}

/* Saves the current spreadsheet */
void savecsvfile(char* fileName, char *headerCopy)
{
  char record[MAXROWCHARS] ="",dataBuff[MAXVALCHARS] = "", delim[2]=";",finfo[15] = "", temp[15]="";
  int col, row, overwrite, isFirst= FALSE,isMulti= FALSE,st = 0;
  CELLPTR cellptr;
  FILE *stream;
  int rows = lastrow, cols = lastcol;

  if(isPagePresent(0)) {
   isMulti = TRUE;
   isFirst = isFirstPage();
  }

  // The delimiter character is identified
  strset(delim, delimiter);

  stream = fopen(fileName, "w+");

  if( HEADERROW || isFirst || !isMulti )
  {
   /* header */
   strcpy(record , "");
   for (col = 0; col <= cols; col++)
   {
    cellptr = (HEADERROW) ? hcell[col] : cell[col][0];
    if (cellptr != NULL)
    {
      strcpy(finfo,"");
      strcpy(dataBuff,cellptr->v.text);

      if(strlen(dataBuff)==0) {
         strcpy(dataBuff, doubletostr(cellptr->v.value));
         strcat(record, dataBuff);
      }
      else if(strchr(dataBuff, delimiter)) {
         strcat(record, "\"" );
         strcat(record, dataBuff );
         strcat(record, "\"" );
      } 
      else strcat(record, dataBuff );
      
      if( format[col]&DOLLAR) {
         strcat(finfo, "f:$");
      }
      else if( format[col] == 0 ) {
         strcat(finfo, "f:L"); //justify-left
      }
      if( colwidth[col] != DEFAULTWIDTH ) {
         sprintf(temp, "w:%d", colwidth[col]);
         if(strlen(finfo) > 0) strcat(strcat(finfo, ","), temp);
         else                  strcpy(finfo, temp);
      }
      if(strlen(finfo) > 0) {
         sprintf(temp, "{%s}", finfo);
         strcat(record, temp);
      }
    }
    if(col < cols) strcat(record, delim);
   }

   strcat(record, "\n");
   if(headerCopy != NULL) strcpy(headerCopy, record);

   #if HEADERROW == 1 
   st = 0;// Data row starts from Row 0
   #else
   fwrite(record, strlen(record), 1, stream);// First page or not Multi-paged
   st = 1;// Data row starts from Row 1
   #endif
  }

  /* actual data */
  for (row = st; row <= rows; row++)
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
      case TEXT :     
       if(strchr(cellptr->v.text, delimiter)) {
        strcat(record, "\"" );
        strcat(record, cellptr->v.text );
        strcat(record, "\"" );
       } 
       else strcat(record, cellptr->v.text );
       break;
      case VALUE :
        strcpy(dataBuff, doubletostr(cellptr->v.value));
        strcat(record, dataBuff);
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

int isColumnNumeric(char *bookPtr, int totalLines) {
   int nu = TRUE, lineI = 0;
   char field[10+1];

   while( lineI < totalLines && lineI < 10) {
     memcpy(field, bookPtr, 10 );
     field[10] = 0;
     bookPtr += 10;
     lineI++;
     if( !isnumeric(field) ) {
      nu = FALSE;
      break;
     }
   }

   return nu;
}

int* getSortedLines(char* fileName, int col, int dir, int* typ, int* itemCount, int* lineSizes ) {
   FILE *stream, *tmpStream;
   int iter=0,dataItr,length = MAXROWCHARS, correction=0, tmpLen, linesFound = 0, totalLines, lineI, dataType, *locations;
   long curPos, prevPos, tmpPos = 0L, newPos = 0L, tmpVal, *bookLPtr, treshold ;
   char tmp[MAXROWCHARS+1], field[10+1];
   char *ptrSt,*pencil,*ptr,*book,*bookPtr,ch[1],tmpFile[15]="tctmp.tmp";
   long totalSize = filesize(fileName) ;

   curPos = 0L;
   stream = fopen(fileName,"r");
   tmpStream = fopen(tmpFile, "w");
   book = (char*)reusableFile();
   treshold = (reusableFileMem() - MAXROWCHARS);
   //blankOut(book, strlen(book));
   bookPtr = book;
   bookLPtr = (long*)book;
   totalLines = 0;
   dataItr = -1;// skip header

   do {
      blankOut(tmp,MAXROWCHARS);
      length = MAXROWCHARS;
      if( totalSize - curPos < MAXROWCHARS) {
         length = totalSize - curPos;
      }

      fseek(stream, curPos, SEEK_SET);
      fread(tmp,sizeof(char),length,stream);
      tmp[length] = 0;
      prevPos = curPos;
      curPos = ftell(stream);

      if(curPos >= totalSize) {
         trim(tmp);
         length = strlen(tmp);
         correction = (curPos - totalSize);
         length -= correction;
         tmp[length] = 0;
      }
      if( length > MINROWCHARS ) {
         length = strlen(tmp);
         tmpPos = prevPos;
         newPos = prevPos;
         linesFound = 0;

         // Skip the last line if it occupies whole MAXROWCHARS
         if( length > (MAXROWCHARS-(MAXROWS/2)) ){
          while( !( isNewLn(tmp[length-1]) || isprint(tmp[length-1]) ) ) length--;
          while( isNewLn(tmp[length-1]) ) length--;
          while( isprint(tmp[length-1]) ) length--;
         }

         // Extract Lines
         ptrSt = &tmp[0];
         pencil = ptrSt;
         ptr = strchr(tmp, '\n');

         if(!ptr){
          if( length > MINROWCHARS ) {
           tmpLen = strlen(tmp);

           newPos = prevPos + tmpLen;
           lineSizes[totalLines + linesFound] = tmpLen;
           linesFound++;

           getField( tmp, /*out*/field, tmpLen, col);
           if(totalLines == 0 && lineI == 0 ) {
            // skip as this is header
           } else {
            memcpy( bookPtr, field, 10);
            bookPtr += 10;
           }
          }
         } else {
          while(ptr && (ptr - pencil) > 0 && (pencil - ptrSt < length) )
          {
           lineSizes[totalLines + linesFound] = (ptr - pencil + 2);
           tmpPos += (ptr - pencil + 2);
           ptr++;
           pencil = ptr;
           ptr = strchr(ptr, '\n');
           linesFound++;
          }

          // now lets get the field data
          newPos = tmpPos;
          pencil = &tmp[0];
          ptr = strchr(tmp, '\n');
          lineI = 0;

          while(ptr && (lineI < linesFound) ){
           tmpLen = strlen( pencil );
           getField( pencil, /*out*/field, tmpLen, col);

           if(totalLines == 0 && lineI == 0 ) {
            // skip as this is header
           } else {
            memcpy( bookPtr, field, 10);
            bookPtr += 10;
           }

           ptr = strchr(pencil, '\n');
           while( isNewLn(ptr[0]) ) ptr++;

           pencil = ptr;
           ptr = strchr(pencil, '\n');
           lineI++;
          }
         }

         curPos = newPos;
         dataItr += linesFound;
         totalLines += linesFound;
      }

      // Dump the data to a temp file if it exceeds bookSZ
      if( (dataItr * 10) > treshold ) {
         fwrite(book, sizeof(char), dataItr * 10, tmpStream);
         bookPtr = book;
         dataItr = 0;
      }

      iter++;
   } while( (totalSize - curPos) > 3 && iter < 1000 ); // Adjustment

   // Dump the last iteration
   newPos = ftell( tmpStream );
   if( (dataItr * 10) <= treshold && newPos > 0L ){
     fwrite(book, sizeof(char), dataItr * 10, tmpStream);
     dataItr = 0;
   }
   fclose(tmpStream);

   bookPtr = book;
   bookLPtr = (long*)book;
   totalLines--; // skip Header as it is not captured

   // Identify if the string data collected is actually numeric
   if(newPos > 0L) {
     // Deal with data in file
     totalSize = filesize(tmpFile);
     tmpStream = fopen(tmpFile, "r");
     tmpLen = 100;
     fread(tmp,sizeof(char), tmpLen, tmpStream);
     tmp[tmpLen] = 0;
     fseek(tmpStream, 0L, SEEK_SET);
     dataType = isColumnNumeric(tmp, totalLines);
     dataItr = 0;

     if( dataType == VALUE ) {
       do{
         fread(tmp, sizeof(char), MAXROWCHARS , tmpStream);
         curPos = ftell(tmpStream);
         tmpLen = strlen( tmp );

         for(iter = 0; iter < (tmpLen / 10); iter++ ){
           memcpy( field, &tmp[iter*10], 10);
           field[10]=0;
           bookLPtr[dataItr] = atol( field );
           dataItr++;
         }
       } while( (totalSize - curPos) > 3 );
     } else {
       do{
         fread(tmp, sizeof(char), MAXROWCHARS , tmpStream);
         curPos = ftell(tmpStream);
         tmpLen = strlen( tmp );

         for(iter = 0; iter < (tmpLen / 10); iter++ ){
           memcpy( field, &tmp[iter*10], 10);
           field[10]=0;
           trim(field);

           if(strlen(field) >=5)
             memcpy( bookPtr + (dataItr*5), field, 5);
           else {
             concatSpace(field, strlen(field), 5-strlen(field), RIGHT);
             memcpy( bookPtr + (dataItr*5), field, 5);
           }
           dataItr++;
         }
       } while( (totalSize - curPos) > 3 );
     }

     fclose(tmpStream);
   } else {
     // Deal with data in memory
     tmpLen = 100;
     memcpy(tmp, book, tmpLen);
     tmp[tmpLen] = 0;
     dataType = isColumnNumeric(tmp, totalLines);

     if( dataType == VALUE ) {
       for(iter = 0; iter < totalLines; iter++ ){
         memcpy( field, bookPtr, 10);
         field[10]=0;
         bookPtr += 10;
         bookLPtr[iter] = atol( field );
       }
     } else {
       for(iter = 0; iter < totalLines; iter++ ){
         memcpy( field, bookPtr + (iter*10), 10);
         field[10]=0;
         trim(field);

         if(strlen(field) >=5)
           memcpy( bookPtr + (iter*5), field, 5);
         else {
           concatSpace(field, strlen(field), 5-strlen(field), RIGHT);
           memcpy( bookPtr + (iter*5), field, 5);
         }
       }
     }
   }

   if( dataType == VALUE ) {
     locations = quickSort((char*)bookLPtr, 4/*long*/, dir, totalLines);
   } else {
     locations = quickSort((char*)bookPtr, 1/*char*/, dir, totalLines);
   }
   *itemCount = totalLines;
   *typ = dataType;

   fclose(stream);

   //Locations doesn't include header, so increment by 1
   for(iter=0;iter<totalLines;iter++) locations[iter]++;
   //logmsg("totalLines: %d, last line:(%d,%d) -----\n", totalLines, lineSizes[totalLines-1],lineSizes[totalLines]);

   return locations;
}

void sortByField(char *fileName, int col) {
   char tmp[501],*ret1,data[6],duplFile[15] = "tcdupl.tmp" ;
   int typ,i,j,perc,sz,max=1000,*lnWdths,prevLoc, offI,loc, lineCount, *locations, pending = FALSE, offCP[5], thresCP = (MAXINT-MAXROWCHARS);
   long curPos = 0L, *ret2,offBase,stOff,endOff, totalSize = filesize(fileName), *lnOffs;
   FILE *duplStream,*stream = fopen(fileName,"r");

   // 1.make a duplicate of actual file
   duplStream = fopen(duplFile, "w");
   copyStream(0L, stream, totalSize, duplStream, FALSE/*Only backup*/ );
   fclose(stream);
   fclose(duplStream);

   // 2.get the sort order
   lnWdths = (int*)malloc((max+1)*sizeof(int));
   locations = getSortedLines(fileName, col, ASC, &typ, &sz , lnWdths);
   if( sz < max ) {
       lnWdths = (int*)realloc(lnWdths, (sz + 1)*sizeof(int));
   }

   // 3. form the offset checkpoint, which is useful for efficiently calculating offsets by linewidths
   lineCount = sz + 1;// header
   for(j=0;j<5;j++) offCP[j] = 0;
   offI = 0;
   offBase = 0L;
   i = 0;
   curPos = 0L;
   while(offI < lineCount) {
       if(offBase > thresCP) {
         offCP[i] = (offI-1);
         curPos += lnWdths[offCP[i]];
         //logmsg("Checkpoint Location %d), location:%d, Offset: %d, Aggr:%ld-----\n", i, offCP[i] + 1, lnWdths[offCP[i]], curPos);
         i++;
         offBase = 0;
       }
       offBase +=lnWdths[offI];
       lnWdths[offI] = offBase;
       offI++;
   }

   // 4.sort the locations
   duplStream = fopen(duplFile, "r");
   stream = fopen(fileName,"w");
   // First header
   stOff = 0L;endOff = lnWdths[0];
   //logmsg("%d)loc:%d,", 0, 0);
   copyStream(stOff, duplStream, endOff, stream, FALSE );

   prevLoc = -1;
   endOff = 0L;
   pending = FALSE;
   perc = sz / 10;

   // Body
   for(i=0; i<sz; i++) {
       loc = locations[i];

       if( loc - prevLoc == 1 )	{
         //printf("-");
       } else {
         if(pending){
           //logmsg("Start:%ld, end:%ld \n", stOff, endOff);
           copyStream(stOff, duplStream, endOff, stream, FALSE);
           pending = FALSE;
         }
       }

       // Identify the start and end offset for the record line
       offBase = 0L;
       for(j=0; (j<5 && offCP[j] > 0) ;j++){
         if( offCP[j] < loc )
           offBase += (long)lnWdths[offCP[j]];
       }

       curPos = (long)lnWdths[loc-1];
       if(curPos <= thresCP) {
         curPos += offBase;
         endOff = (long)lnWdths[loc];
         endOff += offBase;
       }
       else {
         curPos = offBase;// start is at checkpoint
         endOff = offBase + (long)lnWdths[loc];
       }

       if(!pending) stOff = curPos;
       pending = TRUE;
       prevLoc = loc;
       if(i%perc == 0) {
         //logmsg("%d%c complete\n", i/perc * 10, '%');
       }
   }

   copyStream(stOff, duplStream, endOff, stream, FALSE );
   fclose(stream);
   fclose(duplStream);
   free(lnWdths);
}