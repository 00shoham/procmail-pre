#include "base.h"

#define EQUALS '='
#define LINES 1000

int RawSizeQuotedPrintable( char** lines, int nLines )
  {
  int nBytes = 0;
  for( int i=0; i<nLines; ++i )
    {
    char* line = lines[i];
    if( EMPTY( line ) )
      nBytes += 1;
    else
      nBytes += strlen( line );
    }

  return nBytes;
  }

int HexCharsToNum( int c1, int c2 )
  {
  int num = 0;

  if( c1>='0' && c1<='9' )
    num += (c1 - '0') * 16;
  else if( c1>='a' && c1<='f' )
    num += (c1 - 'a' + 10) * 16;
  else if( c1>='A' && c1<='F' )
    num += (c1 - 'A' + 10) * 16;
  else
    Error( "Invalid c1 - HexCharsToNum( %c, %c )", c1, c2 );

  if( c2>='0' && c2<='9' )
    num += (c2 - '0');
  else if( c2>='a' && c2<='f' )
    num += (c2 - 'a' + 10);
  else if( c2>='A' && c2<='F' )
    num += (c2 - 'A' + 10);
  else
    Error( "Invalid c2 - HexCharsToNum( %c, %c )", c1, c2 );

  return num;
  }

char* DecodeQuotedPrintableMessage( char** lines, int nLines, int* finalLength )
  {
  if( lines==NULL || nLines==0 )
    return NULL;

  int maxSize = RawSizeQuotedPrintable( lines, nLines );
  char* output = (char*)SafeCalloc( maxSize, sizeof(char),
                                    "DecodeQuotedPrintableMessage output buffer" );
  char* ptr = output;
  char* end = output + maxSize - 2;

  for( int i=0; i<nLines && ptr<end; ++i )
    {
    char* line = lines[i];
    if( line==NULL )
      {}
    else if( *line==0 )
      {
      *(ptr++) = '\n';
      }
    else
      {
      for( char* scan=line; *scan!=0; ++scan )
        {
        int c = *scan;
        if( c==EQUALS && *(scan+1)==0 )
          break; /* done with this line, skip encoding the trailing '=' */
        if( c==EQUALS && *(scan+1)!=0 && *(scan+2)!=0 )
          {
          int c1 = *(++scan);
          int c2 = *(++scan);
          int charNum = HexCharsToNum( c1, c2 );
          *(ptr++) = charNum;
          }
        else
          {
          *(ptr++) = c;
          }
        }
      }
    }

  *ptr = 0;

  if( finalLength!=NULL )
    *finalLength = ptr - output;

  return output;
  }

char** ReadFileIntoLinesArray( FILE* f, int* nLinesPtr )
  {
  if( f==NULL )
    return NULL;

  int nLines = LINES;
  int lineNo = 0;

  char** lines = (char**)SafeCalloc( nLines, sizeof(char*), "ReadFileIntoLinesArray - initial" );
  char buf[BIGBUF];

  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    TrimTail( buf );
    lines[lineNo] = strdup( buf );
    ++lineNo;
    if( lineNo == nLines )
      {
      nLines += LINES;
      lines = (char**)realloc( lines, nLines * sizeof(char*) );
      if( lines==NULL )
        Error( "Failed to expand buffer to %d lines", nLines );
      }
    }

  if( nLinesPtr!=NULL )
    *nLinesPtr = nLines;

  return lines;
  }

void FreeLinesArray( char** array, int size )
  {
  for( int i=0; i<size; ++i )
    {
    char* line = array[i];
    if( line!=NULL )
      free( line );
    }
  free( array );
  }
