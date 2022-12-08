#include "base.h"

#define COMMENT_CHAR '#'

void FreeKeywords()
  {
  if( config==NULL )
    Error( "FreeKeywords() with no CONFIG object" );

  if( config->keywords==NULL )
    return;

  if( config->keywordMatchText==NULL )
    return;

  for( int i=0; i<config->nKeywords; ++i )
    {
    char* k = config->keywords[i];
    if( k!=NULL )
      {
      free( k );
      config->keywords[i] = NULL;
      }

    k = config->keywordMatchText[i];
    if( k!=NULL )
      {
      free( k );
      config->keywordMatchText[i] = NULL;
      }
    }

  if( config->keywords!=NULL )
    free( config->keywords );
  config->keywords = NULL;

  if( config->keywordMatchText!=NULL )
    free( config->keywordMatchText );
  config->keywordMatchText = NULL;

  if( config->keywordTypes!=NULL )
    free( config->keywordTypes );
  config->keywordTypes = NULL;

  if( config->keywordMatches!=NULL )
    free( config->keywordMatches );
  config->keywordMatches = NULL;

  config->nKeywords = 0;
  }

void SetDefaultKeywords()
  {
  if( config==NULL )
    Error( "SetDefaultKeywords() with no CONFIG object" );

  FreeKeywords();
  }

void ReadKeywords( char* fileName )
  {
  if( config==NULL )
    Error( "ReadKeywords() with no CONFIG object" );

  /* just in case */
  FreeKeywords();

  if( EMPTY( fileName ) )
    Error( "Cannot open unspecified keywords file" );

  FILE* f = fopen( fileName, "r" );
  if( f==NULL )
    Error( "Cannot open keywords file %s", fileName );

  int nLines = 0;
  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    char* k = StripEOL( buf );
    if( *k==COMMENT_CHAR )
      continue;

    if( NOTEMPTY( k ) )
      {
      if( strncasecmp( k, "LITERAL ", 8 )==0
          || strncasecmp( k, "REGEX ", 6 )==0 )
        ++nLines;
      else
        {
        /* Warning( "Every keyword should be preceded by 'LITERAL' or 'REGEX'" ); */
        }
      }
    }

  if( nLines>0 )
    {
    config->keywords =
      (char**)SafeCalloc( nLines, sizeof(char*), "keyword pointers" );
    config->keywordMatchText =
      (char**)SafeCalloc( nLines, sizeof(char*), "keyword match text pointers" );
    config->keywordTypes =
      (enum keywordType *)SafeCalloc( nLines, sizeof(enum keywordType), "keyword type array" );
    config->keywordMatches =
      (int *)SafeCalloc( nLines, sizeof(int), "keyword match array" );

    config->nKeywords = nLines;
    rewind( f );
    int lineNo = 0;
    while( (lineNo < config->nKeywords)
           && (fgets( buf, sizeof(buf)-1, f )==buf) )
      {
      char* k = StripEOL( buf );

      if( *k==COMMENT_CHAR )
        continue;

      if( NOTEMPTY( k ) )
        {
        if( strncasecmp( k, "LITERAL ", 8 )==0 )
          {
          k += 7;
          while( isspace( *k ) )
            ++k;
          config->keywords[lineNo] = strdup( k );
          config->keywordTypes[lineNo] = kt_literal;
          config->keywordMatches[lineNo] = 0;

          ++lineNo;
          }
        else if( strncasecmp( k, "REGEX ", 6 )==0 )
          {
          k += 5;
          while( isspace( *k ) )
            ++k;

          regex_t re;
          int err = regcomp( &re, k, REG_EXTENDED|REG_NOSUB );
          if( err )
            {
            char msg[BUFLEN];
            msg[0] = 0; /* just in case */
            (void)regerror( err, &re, msg, sizeof(msg)-1 );
            Error( "Failed to compile RegEx [%s] - %d - %s", k, err, msg );
            }
          regfree( &re );

          config->keywords[lineNo] = strdup( k );
          config->keywordTypes[lineNo] = kt_regex;
          config->keywordMatches[lineNo] = 0;

          ++lineNo;
          }
        else if( strncasecmp( k, "DECODE-HEADER-LINES", 19 )==0 )
          config->decodeHeaderLines = 1;
        else if( strncasecmp( k, "MERGE-HEADER-LINES", 18 )==0 )
          config->mergeHeaderLines = 1;
        else if( strncasecmp( k, "SEARCH-ATTACHMENTS", 18 )==0 )
          config->searchEncodedTextAttachments = 1;
        else if( strncasecmp( k, "SEARCH-MESSAGE", 14 )==0 )
          config->searchRawMessage = 1;
        else
          Warning( "Invalid keyord - [%s]", k );
        }
      }
    }

  fclose( f );
  }

void PrintKeywords()
  {
  if( config->keywords==NULL || config->nKeywords==0 )
    Warning( "No keywords defined" );
  else
    {
    for( int i=0; i<config->nKeywords; ++i )
      {
      char* k = config->keywords[i];
      if( NOTEMPTY( k ) )
        {
        fputs( k, stdout );
        fputs( "\n", stdout );
        }
      }
    }
  }
