#ifndef _QUOTED_PRINTABLE_INC
#define _QUOTED_PRINTABLE_INC

char* DecodeQuotedPrintableMessage( char** lines, int nLines, int* finalLength );
char** ReadFileIntoLinesArray( FILE* f, int* nLinesPtr );
void FreeLinesArray( char** array, int size );

#endif

