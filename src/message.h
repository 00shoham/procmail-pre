#ifndef _READ_MESSAGE_INC
#define _READ_MESSAGE_INC

void FreeMessage();
void SetDefaultMessage();

void StripTrailingNL( char* buf );
void ReadMessageFromFile( FILE* f );
void ReadMessageFromFilename( char* name );
void ReadMessageFromStdin();
void PrintMessage();

#endif
