/* Turbo C++ - (C) Copyright 1987-1991 by Borland International */

#include <math.h>
#include <alloc.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <mem.h>
#include <stdio.h>
#include <conio.h>
#include "tcalc.h"

int alloctext(int col, int row, char *s)
/* Allocates space for a text cell */
{
 int size;
 CELLPTR cellptr;

 if (memleft < (size = textcellsize(s)))
  return(FALSE);
 memleft -= size;
 cellptr = (CELLPTR)(malloc(strlen(s) + 2));
 cellptr->attrib = TEXT;
 strcpy(cellptr->v.text, s);
 cell[col][row] = cellptr;
 return(TRUE);
} /* alloctext */

int allocvalue(int col, int row, double amt)
/* Allocates space for a value cell */
{
 CELLPTR cellptr;

 if (memleft < valuecellsize)
  return(FALSE);
 memleft -= valuecellsize;
 cellptr = (CELLPTR)(malloc(sizeof(double) + 1));
 cellptr->attrib = VALUE;
 cellptr->v.value = amt;
 cell[col][row] = cellptr;
 return(TRUE);
} /* allocvalue */

int allocformula(int col, int row, char *s, double amt)
/* Allocates space for a formula cell */
{
 int size;
 CELLPTR cellptr;

 if (memleft < (size = formulacellsize(s)))
  return(FALSE);
 memleft -= size;
 cellptr = (CELLPTR)(malloc(strlen(s) + sizeof(double) + 2));
 cellptr->attrib = FORMULA;
 strcpy(cellptr->v.f.formula, s);
 cellptr->v.f.fvalue = amt;
 cell[col][row] = cellptr;
 return(TRUE);
} /* allocformula */

void deletecell(int col, int row, int display)
/* Deletes a cell */
{
 CELLPTR cellptr = cell[col][row];

 if (cellptr == NULL)
  return;
 switch (cellptr->attrib)
 {
  case TEXT :
   memleft += textcellsize(cellptr->v.text);
   clearoflags(col + 1, row, display);
   break;
  case VALUE :
   memleft += valuecellsize;
   break;
  case FORMULA :
   memleft += formulacellsize(cellptr->v.f.formula);
   break;
 } /* switch */
 if(row == 0) format[col] &= ~OVERWRITE;
 free(cell[col][row]);
 cell[col][row] = NULL;
 if (col == lastcol)
  setlastcol();
 if (row == lastrow)
  setlastrow();
 updateoflags(col, row, display);
 changed = TRUE;
} /* deletecell */

void printfreemem(void)
/* Prints the amount of free memory */
{
 memleft = coreleft();
 writef(strlen(MSGMEMORY) + 2, 1, MEMORYCOLOR, 6, "%6ld", memleft);
} /* printfreemem */

int rowwidth(int row)
/* Returns the width in spaces of row */
{
 return((row == 0) ? 1 : (int)log10(row + 1) + 1);
} /* rowwidth */

int formulastart(char **input, int *col, int *row)
/* Returns TRUE if the string is the start of a formula, FALSE otherwise.
   Also returns the column and row of the formula.
*/
{
 int len, maxlen = rowwidth(MAXROWS);
 char *start, numstring[10];

 if (!isalpha(**input))
  return(FALSE);
 *col = *((*input)++) - 'A';
 if (isalpha(**input))
 {
  *col *= 26;
  *col += *((*input)++) - 'A' + 26;
 }
 if (*col >= MAXCOLS)
  return(FALSE);
 start = *input;
 for (len = 0; len < maxlen; len++)
 {
  if (!isdigit(*((*input)++)))
  {
   (*input)--;
   break;
  }
 }
 if (len == 0)
  return(FALSE);
 strncpy(numstring, start, len);
 numstring[len] = 0;
 *row = atoi(numstring) - 1;
 if ((*row >= MAXROWS) || (*row == -1))
  return(FALSE);
 return(TRUE);
} /* formulastart */

void errormsg(char *s)
/* Prints an error message at the bottom of the screen */
{
 printf("%c", 7);      /* Beeps the speaker */
 writef(1, 25, ERRORCOLOR, 79, "%s  %s", s, MSGKEYPRESS);
 gotoxy(strlen(s) + strlen(MSGKEYPRESS) + 3, 25);
 getkey();
 gotoxy(1, 25);
 writef(1, 25, WHITE, 79, "");
} /* errormsg */

void successmsg(char *s)
/* Prints a success message at the bottom of the screen */
{
 printf("%c", 7);      /* Beeps the speaker */
 writef(1, 25, SUCCESSCOLOR, 79, "%s", s);
 writef(1, 24, WHITE, 79, "");
 gotoxy(strlen(s) + strlen(MSGKEYPRESS) + 3, 25);
 getkey();
 gotoxy(1, 25);
 writef(1, 25, WHITE, 79, "");
} /* successmsg */

void fixformula(int col, int row, int action, int place)
/* Modifies a formula when its column or row designations need to change. */
{
 char *colstart, *rowstart, s[6], newformula[MAXINPUT + 1],
      *curpos = newformula;
 int fcol, frow;
 CELLPTR cellptr = cell[col][row];
 double value;

 strcpy(newformula, cellptr->v.f.formula);
 while (*curpos != 0)
 {
  if (formulastart(&curpos, &fcol, &frow))
  {
   rowstart = curpos - rowwidth(frow);
   colstart = rowstart - ((fcol > 25) ? 2 : 1);
   switch (action)
   {
    case COLADD :
     if (fcol < place)
      break;
     if (fcol == 25)
     {
      if (strlen(newformula) == MAXINPUT)
      {
       deletecell(col, row, NOUPDATE);
       alloctext(col, row, newformula);
       return;
      }
      movmem(colstart, colstart + 1, strlen(colstart) + 1);
     }
     colstring(fcol + 1, s);
     movmem(s, colstart, strlen(s));
     break;
    case ROWADD :
     if (frow < place)
      break;
     if (rowwidth(frow + 1) != rowwidth(frow))
     {
      if (strlen(newformula) == MAXINPUT)
      {
       deletecell(col, row, NOUPDATE);
       alloctext(col, row, newformula);
       return;
      }
      movmem(rowstart, rowstart + 1, strlen(rowstart) + 1);
     }
     sprintf(s, "%d", frow + 2);
     movmem(s, rowstart, strlen(s));
     break;
    case COLDEL :
     if (fcol <= place)
      break;
     if (fcol == 26)
      movmem(colstart + 1, colstart, strlen(colstart) + 1);
     colstring(fcol - 1, s);
     movmem(s, colstart, strlen(s));
     break;
    case ROWDEL :
     if (frow <= place)
      break;
     if (rowwidth(frow) != rowwidth(frow - 1))
      movmem(rowstart + 1, rowstart, strlen(rowstart) + 1);
     sprintf(s, "%d", frow);
     movmem(s, rowstart, strlen(s));
     break;
   } /* switch */
  }
  else
   curpos++;
 }
 if (strlen(newformula) != strlen(cellptr->v.f.formula))
 {
  value = cellptr->v.f.fvalue;
  deletecell(col, row, NOUPDATE);
  allocformula(col, row, newformula, value);
 }
 else
  strcpy(cellptr->v.f.formula, newformula);
} /* fixformula */

void colstring(int col, char *colstr)
/* Changes a column number to a string */
{
 setmem(colstr, 3, 0);
 if (col < 26)
  colstr[0] = col + 'A';
 else
 {
  colstr[0] = (col / 26) - 1 + 'A';
  colstr[1] = (col % 26) + 'A';
 }
} /* colstring */

void centercolstring(int col, char *colstr)
/* Changes a column to a centered string */
{
 char s[3];
 int spaces1, spaces2;

 colstring(col, s);
 spaces1 = (colwidth[col] - strlen(s)) >> 1;
 spaces2 = colwidth[col] - strlen(s) - spaces1;
 sprintf(colstr, "%*s%s%*s", spaces1, "", s, spaces2, "");
} /* centercolstring */

void setleftcol(void)
/* Sets the value of leftcol based on the value of rightcol */
{
 int total = 80, col = 0;

 while ((total >= LEFTMARGIN) && (rightcol - col >= 0))
 {
  colstart[SCREENCOLS - col - 1] = total - colwidth[rightcol - col];
  total -= colwidth[rightcol - col++];
 }
 if (total >= LEFTMARGIN)
  col++;
 movmem(&colstart[SCREENCOLS - col + 1], colstart, col - 1);
 leftcol = rightcol - col + 2;
 total = colstart[0] - LEFTMARGIN;
 if (total != 0)
 {
  for (col = leftcol; col <= rightcol; col++)
   colstart[col - leftcol] -= total;
 }
 printcol();
} /* setleftcol */

void setrightcol(void)
/* Sets the value of rightcol based on the value of leftcol */
{
 int total = LEFTMARGIN, col = 0;

 do
 {
  colstart[col] = total;
  total += colwidth[leftcol + col++];
 }
 while ((total <= 80) && (leftcol + col <= MAXCOLS));
 rightcol = leftcol + col - 2;
 printcol();
} /* setrightcol */

void settoprow(void)
/* Figures out the value of toprow based on the value of bottomrow */
{
 if (bottomrow - SCREENROWS < -1)
  bottomrow = 19;
 toprow = bottomrow - 19;
 printrow();
} /* settoprow */

void setbottomrow(void)
/* Figures out the value of bottomrow based on the value of toprow */
{
 if (toprow + SCREENROWS > MAXROWS)
  toprow = MAXROWS - 20;
 bottomrow = toprow + 19;
 printrow();
} /* setbottomrow */

void setlastcol(void)
/* Sets the value of lastcol based on the current value */
{
 register int row, col;

 for (col = lastcol; col >= 0; col--)
 {
  for (row = 0; row <= lastrow; row++)
  {
   if (cell[col][row] != NULL)
   {
    lastcol = col;
    return;
   }
  }
 }
 lastcol = 0;
} /* setlastcol */

void setlastrow(void)
/* Sets the value of lastrow based on the current value */
{
 register int row, col;

 for (row = lastrow; row >= 0; row--)
 {
  for (col = 0; col <= lastcol; col++)
  {
   if (cell[col][row] != NULL)
   {
    lastrow = row;
    return;
   }
  }
 }
 lastrow = 0;
} /* setlastrow */

void act(char *s)
/* Acts on a particular input */
{
 int attrib, allocated;
 double value;

 deletecell(curcol, currow, UPDATE);
 value = parse(s, &attrib);
 switch(attrib)
 {
  case TEXT :
   allocated = alloctext(curcol, currow, s);
   if (allocated)
    displaycell(curcol, currow, NOHIGHLIGHT, NOUPDATE);
   break;
  case VALUE :
   allocated = allocvalue(curcol, currow, value);
   break;
  case FORMULA :
   allocated = allocformula(curcol, currow, s, value);
   break;
 } /* switch */
 if (allocated)
 {
  if(currow==0)format[curcol] &= ~OVERWRITE;
  clearoflags(curcol + 1, currow, UPDATE);
  if (attrib == TEXT)
    setoflags(curcol, currow, UPDATE);
  if (curcol > lastcol)
   lastcol = curcol;
  if (currow > lastrow)
   lastrow = currow;
  if (autocalc)
   recalc();
 }
 else
  errormsg(MSGLOMEM);
 printfreemem();
} /* act */

int setoflags(int col, int row, int display)
/* Sets the overwrite flag on cells starting at (col + 1, row) - returns
   the number of the column after the last column set.
*/
{
 int len;

 len = strlen(cell[col][row]->v.text) - colwidth[col];
 while ((++col < MAXCOLS) && (len > 0) && (cell[col][row] == NULL))
 {
  if(row==0)format[col] |= OVERWRITE;
  len -= colwidth[col];
  if (display && (col >= leftcol) && (col <= rightcol))
   displaycell(col, row, NOHIGHLIGHT, NOUPDATE);
 }
 return(col);
} /* setoflags */

void clearoflags(int col, int row, int display)
/* Clears the overwrite flag on cells starting at (col, row) */
{
 while ((format[col] >= OVERWRITE) && (col < MAXCOLS) &&
        (cell[col] == NULL))
 {
  if(row==0)format[col] &= ~OVERWRITE;
  if (display && (col >= leftcol) && (col <= rightcol))
   displaycell(col, row, NOHIGHLIGHT, NOUPDATE);
  col++;
 }
} /* clearoflags */

void updateoflags(int col, int row, int display)
/* Starting in col, moves back to the last TEXT cell and updates all flags */
{
 while ((cell[col][row] == NULL) && (col-- > 0));
 if ((cell[col][row] != NULL) && (cell[col][row]->attrib == TEXT) && 
     (col >= 0))
  setoflags(col, row, display);
} /* updateoflags */

void textstring(char *instring, char *outstring, int col, int fvalue,
                int formatting)
/* Sets the string representation of text */
{
 char *just, *ljust = "%-*s", *rjust = "%*s";

 if ((fvalue & RJUSTIFY) && (formatting))
  just = rjust;
 else
  just = ljust;
 sprintf(outstring, just, colwidth[col], instring);
 if (formatting)
  outstring[colwidth[col]] = 0;
} /* textstring */

void valuestring(CELLPTR cellptr, double value, char *vstring, int col,
                 int fvalue, int *color, int formatting)
/* Sets the string representation of a value */
{
 char s[81];
 char *fstring;
 int width, pos;

 if (value == HUGE_VAL)
 {
  strcpy(vstring, MSGERROR);
  *color = ERRORCOLOR;
 }
 else
 {
  if (formatting)
  {
   //sprintf(vstring, "%1.*f", fvalue & 15, cellptr->v.value);
   strcpy(vstring, doubletostr(cellptr->v.value));
   trimdecimals(vstring);
   if (fvalue & COMMAS)
   {
    pos = strlen(vstring);
    while (pos > 3)
    {
     pos -= 3;
     if (vstring[pos - 1] != '-')
     {
      movmem(&vstring[pos], &vstring[pos + 1], strlen(vstring) - pos + 1);
      vstring[pos] = ',';
     }
    }
   }
   if (fvalue & DOLLAR)
   {
    if (vstring[0] == '-')
    {
     fstring = " $";
     width = colwidth[col] - 2;
    }
    else
    {
     fstring = " $ ";
     width = colwidth[col] - 3;
    }
   }
   else
   {
    fstring = "";
    width = colwidth[col];
   }
   strcpy(s, vstring);
   if (fvalue & RJUSTIFY)
   {
    if (strlen(vstring) > width)
     vstring[width] = 0;
    else
     sprintf(vstring, "%*s", width, s);
   }
   else
    sprintf(vstring, "%-*s", width, s);
   movmem(vstring, &vstring[strlen(fstring)], strlen(vstring) + 1);
   strncpy(vstring, fstring, strlen(fstring));
  }
  else
   strcpy(vstring, doubletostr(value));// MAXPLACES format removed
  *color = VALUECOLOR;
 }
} /* valuestring */

char *cellstring(int col, int row, int *color, int formatting)
/* Creates an output string for the data in the cell in (col, row), and
   also returns the color of the cell */
{
 CELLPTR cellptr = cell[col][row];
 int newcol, formatvalue;
 static char s[81], temp[MAXCOLWIDTH + 1];
 char *p;
 double value;

 if (cellptr == NULL)
 {
  if (!formatting || (format[col] < OVERWRITE))
  {
   sprintf(s, "%*s", colwidth[col], "");
   *color = BLANKCOLOR;
  }
  else
  {
   newcol = col;
   while (cell[--newcol][row] == NULL);
   p = cell[newcol][row]->v.text;
   while (newcol < col)
    p += colwidth[newcol++];
   strncpy(temp, p, colwidth[col]);
   temp[colwidth[col]] = 0;
   sprintf(s, "%s%*s", temp, colwidth[col] - strlen(temp), "");
   *color = TEXTCOLOR;
  }
 }
 else
 {
  formatvalue = format[col];
  switch (cellptr->attrib)
  {
   case TEXT :
    textstring(cellptr->v.text, s, col, formatvalue, formatting);
    *color = TEXTCOLOR;
    break;
   case FORMULA :
    if (formdisplay)
    {
     textstring(cellptr->v.f.formula, s, col, formatvalue, formatting);
     *color = FORMULACOLOR;
     break;
    }
    else
     value = cellptr->v.f.fvalue;
   case VALUE :
    if (cellptr->attrib == VALUE)
     value = cellptr->v.value;
    valuestring(cellptr, value, s, col, formatvalue, color,
                formatting);
    break;
  } /* switch */
 }
 return(s);
} /* cellstring */

void writeprompt(char *prompt)
/* Prints a prompt on the screen */
{
 writef(1, 24, PROMPTCOLOR, 80, prompt);
} /* writeprompt */

void swap(int *val1, int *val2)
/* Swaps the first and second values */
{
 int temp;

 temp = *val1;
 *val1 = *val2;
 *val2 = temp;
} /* swap */

char* removewords(char *buff){
  int length, i = 0;

  if(!buff || strlen(buff) == 0) return buff;

  length = strlen(buff);
  i = 0;
  while( i<length ) { 
    if ( isalpha( buff[i] ) || isdigit( buff[i] ) ) {
      movmem(&buff[i+1], &buff[i], (length-1+i) );
      length--;
    }
    else i++;
  }

  return buff;
}

char* trimdecimals(char *buff){
  char *pos;
  int length;
  pos = strchr(buff,'.');
  if(pos) {
   length = strlen(buff);
   while(buff[length-1] != '.') {  buff[length-1] = '\0'; length--;  }
   buff[length-1] = '\0';
  }
  return buff;
}
char* trimCharsRight(char *buff, char *chars){
  char rem[10];
  int length, i = 0;
  strcpy(rem,chars);
  rem[strlen(chars)] = 0;

  if(!buff || strlen(buff) == 0) return buff;

  // Lets discard junkies present at the end
  i=0;
  while( i<strlen(buff) && ( isprint(buff[i]) ||  iscntrl(buff[i]) )) {  i++;  }
  if( i<strlen(buff) ) buff[i] = 0;

  // reverse trim
  i = strlen(buff) - 1;
  while( i>=0 && iscntrl(buff[i])) {  buff[i] = 0; i--;  }
  i = strlen(buff) - 1;
  while( i>=0 && strchr(rem, buff[i])) {  buff[i] = 0; i--;  }

  return buff;
}
char* trimJunkRight(char *buff){
  return trimCharsRight(buff, " \r\n");
}
char* trimChars(char *buff, char *chars){
  char rem[10];
  int length, i = 0;
  strcpy(rem,chars);
  rem[strlen(chars)] = 0;

  // start reverse
  buff = trimCharsRight(buff,chars);
  
  // start forward
  length = strlen(buff);
  i = 0;
  while( i<length && strchr(rem, buff[i])) { i++;length--;  }
  movmem(&buff[i], &buff[0], length+1 );

  return buff;
}
char* trimJunk(char *buff){
  return trimChars(buff, " \r\n");
}
char* trim(char *buff){
  char *pos;
  int length, i = 0;

  if(!buff || strlen(buff) == 0) return buff;

  // Start from reverse
  i = strlen(buff) - 1;
  while( i>=0 && buff[i] == ' ') {  buff[i] = '\0'; i--;  }

  // start forward
  length = strlen(buff);
  i = 0;
  while( i<length && buff[i] == ' ' ) { i++;length--;  }
  movmem(&buff[i], &buff[0], length+1 );

  return buff;
}
char* doubletostr(double fv){
  double down, up;
  char s[25], *tmp;

  down = floor(fv);
  up = ceil(fv);

  if(down < up) sprintf(s,"%1.2lf",fv); // decimal places exists
  else 		sprintf(s,"%1.0lf",down);// decimal places not present

  //The following is required only for complex string operations
  //tmp = (char*)malloc(25);
  //strcpy(tmp,s);
  //return tmp;

  return s;
}


void checkforsave(void)
/* If the spreadsheet has been changed, will ask the user if they want to
   save it.
*/
{
 int save;

 if (changed && getyesno(&save, MSGSAVESHEET) && (save == 'Y'))
  savesheet();
} /* checkforsave */

void initvars(void)
/* Initializes various global variables */
{
 leftcol = toprow = curcol = currow = lastcol = lastrow = 0;
 setmem(colwidth, sizeof(colwidth), DEFAULTWIDTH);
 setmem(cell, sizeof(cell), 0);
 setmem(format, sizeof(format), DEFAULTFORMAT);

 // In case if the previous file is multi paged in 100's
 strcpy(di.fileName,"");
 di.partialSize = 0;
 di.totalSize = 0;
 di.curPage = 0;
 di.offsets[1] = 0; 
} /* initvars */

int getcommand(char *msgstr, char *comstr)
/* Reads in a command and acts on it */
{
 int ch, counter, len = strlen(msgstr);

 scroll(UP, 0, 1, 24, 80, 24, WHITE);
 for (counter = 0; counter < len; counter++)
 {
  if (isupper(msgstr[counter]))
   writef(counter + 1, 24, COMMANDCOLOR, 1, "%c", msgstr[counter]);
  else
   writef(counter + 1, 24, LOWCOMMANDCOLOR, 1, "%c", msgstr[counter]);
 }
 do
  ch = toupper(getkey());
 while ((strchr(comstr, ch) == NULL) && (ch != ESC));
 clearinput();
 return((ch == ESC) ? -1 : strlen(comstr) - strlen(strchr(comstr, ch)));
} /* getcommand */

void trace(char* info){
  int inx = 0;
  char *lastFewchars = info;
  if(info == NULL) strcpy(diagdata, "");
  else {
    if( strlen(info) > 25 ) {
      inx = strlen(info) - 25;
      lastFewchars = &info[inx];
    }

    if( (strlen(diagdata) + strlen(lastFewchars)) > MAXDIAGCHARS ) 
      return; // cannot add trace anymore
    
    strcat( diagdata, "\n");
    strcat( diagdata, lastFewchars) ;
  }
}

void diagnostics(void)
{
 int ch;char *ptr, diagCopy[MAXDIAGCHARS];
 long memdata = coreleft();

 writef(1, 24, LOWCOMMANDCOLOR, 79, "core memory available :%ld",  memdata  );
 memleft = memdata;
 printfreemem();
 ch = toupper(getkey());

 if(ch != ESC){
  writef(1, 24, LOWCOMMANDCOLOR, 79, "Frame:(%d,%d - %d,%d), Picture:(%d,%d - %d,%d)",
    toprow, leftcol , bottomrow, rightcol, 
    currow, curcol, lastrow, lastcol );
  ch = toupper(getkey());

  if(ch != ESC){
   // Display the data added for diagnostics
   strcpy(diagCopy, diagdata);
   ptr = strtok(diagCopy, "\n");
   
   while (ptr != NULL && ch != ESC) {
      writef(1, 24, LOWCOMMANDCOLOR, 79, ptr );
      ptr = strtok(NULL, "\n");
      ch = toupper(getkey());
   }
   free(ptr);
  }
 }

 clearinput();
} /* diagnostics */


