#ifndef _PROCESS_MESSAGE_INC
#define _PROCESS_MESSAGE_INC

void ProcessContentInRawMessage();
void ProcessContentInEncodedChunk( char* asciiBuf, int nBytes );
void PrintKeywordHeaders();
void RemoveUTFFromHeaders();
void ScanMessageForKeywords();

#endif
