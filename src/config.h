#ifndef _CONFIG_INC
#define _CONFIG_INC

enum keywordType
  { 
  kt_literal,
  kt_regex
  };

typedef struct _config
  {
  /* search parameters */
  int nKeywords;
  char** keywords;
  enum keywordType *keywordTypes;
  int *keywordMatches;
  char** keywordMatchText;

  /* processing options */
  int mergeHeaderLines;
  int decodeHeaderLines;
  int searchEncodedTextAttachments;
  int searchRawMessage;

  /* what did we see in the headers? */
  char* contentType;
  char* contentEncoding;

  /* message from stdin*/
  int nLines;
  int allocatedLines;
  char** message;
  int nHeaderLines;
  int finishedHeaders;
  } _CONFIG;

extern _CONFIG* config;

void FreeConfig();
void InitConfig();

#endif
