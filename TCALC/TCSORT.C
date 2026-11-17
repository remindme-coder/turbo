#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>

#define STRWIDTH 5

struct STACKINFO {
   int lowStack[50];
   int highStack[50];
   int stackI;
   int *locations;
   int size;
} si;


// Utility to print array
void printLongArray(long* arr,int arrLen) {
   int i, sz;
   sz = arrLen / 10;
   printf("Array length:%d, increment:%d\n", arrLen, sz);
   
   for (i = 0; i < 10; i++) {
     printf("%ld(%d) at %d,", arr[i * sz], (si.locations == NULL) ? i * sz : si.locations[i * sz], i * sz );
   }
   printf("%ld(%d) at %d,", arr[arrLen-1], (si.locations == NULL) ? arrLen-1 : si.locations[arrLen-1], arrLen-1 );
   printf("\n");
}
void printIntArray(int* arr,int arrLen) {
   int i;
   
   for (i = 0; i < arrLen; i++) {
     printf("%d(%d) ", arr[i], (si.locations == NULL) ? i: si.locations[i] );
   }
   printf("\n");
}
void printStrArray(char* arr,int arrLen) {
   int i,loc,sz;
   char tmp[STRWIDTH+1];
   sz = arrLen / 10;
   
   for (i = 0; i < 10; i++){
     loc = i*STRWIDTH*sz;
     memcpy(tmp,&arr[loc],STRWIDTH);
     tmp[STRWIDTH]=0;
     printf("%s(%d) at %d,", tmp, (si.locations == NULL) ? i * sz : si.locations[i * sz], i * sz );
   }
   loc = (arrLen-1)*STRWIDTH;
   memcpy(tmp,&arr[loc],STRWIDTH);
   tmp[STRWIDTH]=0;
   printf("%s(%d) at %d,", tmp, (si.locations == NULL) ? arrLen-1 : si.locations[arrLen-1], arrLen-1 );
   printf("\n");
}
void printArray(char* arr, size_t typ, int arrLen) {
   switch (typ) {
     case 1 : printStrArray(arr, arrLen);	break;
     case 2 :	printIntArray((int*)arr, arrLen);	break;
     case 4 :	printLongArray((long*)arr, arrLen);	break;
   } /* switch */
}

// Swap two long integers
void swapLong(long* arr, int i, int j) {
   long tmp, tmpL;
   
   tmpL = si.locations[j];
   tmp = arr[j];
   
   si.locations[j] = si.locations[i];
   arr[j] = arr[i];
   
   si.locations[i] = tmpL;
   arr[i] = tmp;
}

// Swap two integers
void swapInt(int* arr, int i, int j) {
   int tmp, tmpL;
   
   tmpL = si.locations[j];
   tmp = arr[j];
   
   si.locations[j] = si.locations[i];
   arr[j] = arr[i];
   
   si.locations[i] = tmpL;
   arr[i] = tmp;
}

// Swap two strings
void swapStr(char* arr,int i, int j) {
   int locI, locJ, tmpL;
   char tmp[STRWIDTH+1];

   locI = i*STRWIDTH;
   locJ = j*STRWIDTH;

   tmpL = si.locations[j];
   memcpy(tmp,&arr[locJ],STRWIDTH);
   tmp[STRWIDTH]=0;

   si.locations[j] = si.locations[i];
   memcpy(&arr[locJ],&arr[locI],STRWIDTH);

   si.locations[i] = tmpL;
   memcpy(&arr[locI],tmp,STRWIDTH);
}

// Partition function for int
int partitionLong(long* arr, int low, int high) {
   long pivot = arr[low]; // Choosing first element as pivot
   int i = low, j = high;

   while (i < j) {
      while (arr[i] <= pivot && i <= high - 1) i++;
      while (arr[j] > pivot && j >= low + 1) j--;
      if (i < j) swapLong(arr, i, j);
   }
   swapLong(arr, low, j);
   return j;
}

// Partition function for int
int partitionInt(int* arr, int low, int high) {
   int pivot = arr[low]; // Choosing first element as pivot
   int i = low, j = high;

   while (i < j) {
      while (arr[i] <= pivot && i <= high - 1) i++;
      while (arr[j] > pivot && j >= low + 1) j--;
      if (i < j) swapInt(arr, i, j);
   }
   swapInt(arr, low, j);
   return j;
}

// Partition function for strings
int partitionStr(char* arr, int low, int high) {
   int i = low, j = high, locI, locJ;
   char pivot[STRWIDTH+1], tmp[STRWIDTH+1]; // Choosing first element as pivot

   locI = low*STRWIDTH;
   memcpy(pivot,&arr[locI],STRWIDTH);
   pivot[STRWIDTH]=0;
   tmp[STRWIDTH]=0;

   while (i < j) {
      locI = i*STRWIDTH; 
      memcpy(tmp,&arr[locI],STRWIDTH);

      while ( strcmp(tmp, pivot) <= 0 && i <= high - 1) {
        i++;
        if( i <= high - 1 ) {
          locI = i*STRWIDTH; 
          memcpy(tmp,&arr[locI],STRWIDTH);
        }
      }

      locJ = j*STRWIDTH;
      memcpy(tmp,&arr[locJ],STRWIDTH);
      tmp[STRWIDTH]=0;

      while (strcmp(tmp, pivot) > 0 && j >= low + 1) {
        j--;
        if( j >= low + 1 ) {
          locJ = j*STRWIDTH;
          memcpy(tmp,&arr[locJ],STRWIDTH);
        }
      }

      if (i < j) swapStr(arr, i, j);
   }
   
   if(low>-1 && j>-1) swapStr(arr, low, j);
   return j;
}

int* quickSort(char* arr, size_t typ,int direction, int length) {
   int pi,i, tmpI, high, low, stackMaxI;
   int prev, curr, locI;
   char tmp[5+1];
   
   low = 0;
   high = length - 1;
   stackMaxI = -1;
   si.stackI = -1;
   si.size = length;
   si.locations = (int*)malloc(si.size*sizeof(int));
   for(i=0;i<si.size;i++) si.locations[i] = i;

   //printf("In quickSort stack low:%d, high: %d, type: %d\n", low, high, typ);
   while (low < high) {
      switch (typ)
      {
       case 1 : pi = partitionStr(arr, low, high);	break;
       case 2 :	pi = partitionInt((int*)arr, low, high);	break;
       case 4 :	pi = partitionLong((long*)arr, low, high);	break;
      } /* switch */

      tmpI = pi + 1;
      if(tmpI < high){
       si.stackI++;
       if(stackMaxI < si.stackI) stackMaxI = si.stackI;

       si.lowStack[si.stackI] = tmpI;
       si.highStack[si.stackI] = high;
      }

      tmpI = pi-1;
      if(low<tmpI){
       high = tmpI;
      } else {
       if(si.stackI < 0) break;
       low = si.lowStack[si.stackI];
       high = si.highStack[si.stackI];
       si.stackI--;
      }
   }
   //printf("Max stack utilized :%d\n", stackMaxI);
   //printArray(arr, typ, length);
   
   if(direction == 1/*DESC*/) {
      pi = si.size / 2;
      for(i=0;i<pi;i++) {
        curr = si.locations[i];
        tmpI = si.size-1-i;
        si.locations[i] = si.locations[tmpI];
        si.locations[tmpI] = curr;
      }
   }

   return si.locations;
}

