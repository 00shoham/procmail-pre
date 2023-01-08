#define _GNU_SOURCE

#include "base.h"

#define ARRAY_SIZE(XX) (sizeof(XX) / sizeof(XX[0]))

unsigned char headerOmitChars[] = "\t";

/*
 *      =?UTF-8?B?RG9uJ3QgYmUgbGFzdCBpbiBsaW5lIOKAlCBTcGVjaWFsIG9mZmVy?=
 */

#define UTF_PREFIX "=?UTF-8?B?"
#define UTF_SUFFIX "?="

void MySlowCopy( char* dst, char* src, int n )
  {
  for( int i=0; i<n; ++i )
    *(dst++) = *(src++);
  *dst = 0;
  }

void RemoveUTFFromHeaders()
  {
  if( config==NULL )
    Error( "RemoveUTFFromHeaders() with no CONFIG object" );

  if( config->decodeHeaderLines==0 )
    return; /* nobody asked us to do this */

  if( config->nLines==0
      || config->nHeaderLines==0
      || config->message==NULL )
    return;

  for( int i=0; i<config->nHeaderLines; ++i )
    {
    char* header = config->message[i];
    char* tombstone = header + strlen(header);
    if( NOTEMPTY( header ) )
      {
      int gotAny = 0;
      for(;;)
        {
        char* start = strcasestr( header, UTF_PREFIX );
        char* end = strcasestr( header, UTF_SUFFIX );
        if( start!=NULL && end!=NULL && end>start )
          {
          ++gotAny;
          int nChars = end - ( start + strlen(UTF_PREFIX) );
          char* base64chunk = (char*)SafeCalloc( nChars+1, sizeof(char), "RemoveUTFFromHeaders - buffer" );
          if( nChars )
            memcpy( base64chunk, start + strlen(UTF_PREFIX), nChars );
          base64chunk[nChars] = 0;
          int decodedLen = 0;
          unsigned char* decoded = DecodeFromBase64( base64chunk, nChars, &decodedLen );
          free( base64chunk );
          if( decoded!=NULL && decodedLen>=0 )
            {
            int leftoverChars = tombstone - (end + strlen(UTF_SUFFIX) );
            memcpy( start, decoded, decodedLen );
            MySlowCopy( start+decodedLen, end+strlen(UTF_SUFFIX), leftoverChars );
            tombstone = start + decodedLen + leftoverChars;
            *tombstone = 0;
            free( decoded );
            }
          }
        else
          break;
        }
      if( gotAny )
        {
        char* newHeader = EncodeNonPrintableChars( (unsigned char*) header, headerOmitChars );
        free( header );
        config->message[i] = newHeader;
        }
      }
    }
  }

void ProcessContentInEncodedChunk( char* asciiBuf, int nBytes )
  {
  if( config==NULL )
    Error( "ProcessContentInEncodedChunk() with no CONFIG object" );

  if( config->keywords==NULL )
    Error( "ProcessContentInEncodedChunk() - keywords==NULL" );
  if( config->keywordTypes==NULL )
    Error( "ProcessContentInEncodedChunk() - keywordTypes==NULL" );
  if( config->keywordMatches==NULL )
    Error( "ProcessContentInEncodedChunk() - keywordMatches==NULL" );

  int decodedLen = 0;
  unsigned char* decoded = DecodeFromBase64( asciiBuf, nBytes, &decodedLen );
  if( decoded==NULL || decodedLen<=0 )
    Error( "Failed to Base64Decode %d bytes", nBytes );

#ifdef DEBUG
  printf( "DECODED:\n");
  fputs( (char*)decoded, stdout );
  fputs( "\n\n\n", stdout );
#endif

  for( int i=0; i<config->nKeywords; ++i )
    {
    if( config->keywords[i]==NULL )
      Warning( "Keyword[%d] is NULL", i );
    else
      {
      if( config->keywordTypes[i] == kt_literal )
        {
        if( strcasestr( (char*)decoded, config->keywords[i] )!=NULL )
          {
          config->keywordMatches[i] = 1;
          }
        }
      else if( config->keywordTypes[i] == kt_regex )
        {
        if( StringMatchesRegex( config->keywords[i], (char*)decoded )==0 )
          {
          config->keywordMatches[i] = 1;
          }
        }
      }
    }

  /* clean up this chunk */
  free( decoded );
  decoded = NULL;
  }

void DoSingleMatch( int isLiteral,
                    char* keyword,
                    int* matches,
                    char** matchText,
                    regex_t *re,
                    regmatch_t* pmatch,
                    char* line
                    )
  {
  if( EMPTY( line ) )
    return;

  if( isLiteral )
    {
    if( strcasestr( line, keyword )!=NULL )
      {
      *matches = 1;
      }
    }
  else if( regexec( re, line, (re->re_nsub)+1, pmatch, 0 )==0 )
    {
    *matches = 1;

    if( pmatch[0].rm_so >= 0
        && pmatch[0].rm_eo >= pmatch[0].rm_so )
      { /* store the match string */
      int l = pmatch[0].rm_eo - pmatch[0].rm_so;
      char* buf = (char*)SafeCalloc( l+5, sizeof(char), "RegEx match buffer" );
      strncpy( buf, line + pmatch[0].rm_so, l );
      buf[l] = 0;
      if( matchText != NULL )
        {
        if( *matchText != NULL )
          free( *matchText );
        *matchText = buf;
        }
      }
    }
  }

void ProcessContentInRawMessage()
  {
  if( config==NULL )
    Error( "ProcessContentInRawMessage() with no CONFIG object" );

  if( config->searchRawMessage==0 )
    return; /* we were not asked to do this */

  char* rawMessage = NULL;
  if( NOTEMPTY( config->contentType )
      && strcasestr( config->contentType, "text" )!=NULL
      && NOTEMPTY( config->contentEncoding )
      && strcasestr( config->contentEncoding, "quoted-printable" )!=NULL )
    {
    int nLinesBody = config->nLines - config->nHeaderLines;
    char** messageBody = config->message + config->nHeaderLines;
    int messageLen = 0;
    rawMessage = DecodeQuotedPrintableMessage( messageBody, nLinesBody, &messageLen );
    }

  for( int k=0; k<config->nKeywords; ++k )
    {
    char* keyword = config->keywords[k];
    if( EMPTY( keyword ) )
      continue;
    if( config->keywordMatches[k] )
      continue; /* already got a match, no need to find more */

    int isLiteral = (config->keywordTypes[k]==kt_literal) ? 1 : 0;

    regex_t re;
    regmatch_t* pmatch = NULL;
    if( ! isLiteral )
      {
      int err = regcomp( &re, keyword, REG_EXTENDED );
      if( err )
        {
        char msg[BUFLEN];
        msg[0] = 0; /* just in case */
        (void)regerror( err, &re, msg, sizeof(msg)-1 );
        Warning( "Failed to compile RegEx [%s] - %d - %s", keyword, err, msg );
        continue;
        }
      pmatch = (regmatch_t*)SafeCalloc( re.re_nsub+1, sizeof( regmatch_t ), "RegMatch array" );
      }

    if( rawMessage !=NULL )
      {
      DoSingleMatch( isLiteral,
                     keyword,
                     config->keywordMatches + k,
                     config->keywordMatchText + k,
                     &re,
                     pmatch,
                     rawMessage );
      }
    else
      {
      for( int l=0; l<config->nLines; ++l )
        {
        char* line = config->message[l];

        DoSingleMatch( isLiteral,
                       keyword,
                       config->keywordMatches + k,
                       config->keywordMatchText + k,
                       &re,
                       pmatch,
                       line );
        }
      }

    if( ! isLiteral )
      {
      regfree( &re );
      free( pmatch );
      pmatch = 0;
      }
    }

  if( rawMessage !=NULL )
    free( rawMessage );
  }


void PrintKeywordHeaders()
  {
  if( config==NULL )
    Error( "PrintKeywordHeaders() with no CONFIG object" );

  for( int i=0; i<config->nKeywords; ++i )
    {
    if( config->keywordMatches[i] )
      {
      fputs( "KeywordMatch: ", stdout );
      if( config->keywordTypes[i]==kt_literal )
        fputs( config->keywords[i], stdout );
      else if( NOTEMPTY( config->keywordMatchText[i] ) )
        fputs( config->keywordMatchText[i], stdout );
      fputs( "\n", stdout );
      }
    }
  }

enum scanning_states
  {
  ss_for_blank,
  ss_for_separator,
  ss_headers,
  ss_content
  };

int MessageHasContentTypeMultipart()
  {
  if( config==NULL )
    Error( "MessageHasContentTypeMultipart() with no CONFIG object" );
  
  for( int i=0; i<config->nHeaderLines; ++i )
    {
    char* line = config->message[i];
    if( strstr( line, "Content-Type: multipart")!=NULL )
      return 1;
    }

  return 0;
  }

int MessageHasContentTypeText()
  {
  if( config==NULL )
    Error( "MessageHasContentTypeMultipart() with no CONFIG object" );
  
  for( int i=0; i<config->nHeaderLines; ++i )
    {
    char* line = config->message[i];
    if( strstr( line, "Content-Type:")!=NULL
        && strstr( line, "text")!=NULL )
      return 1;
    }

  return 0;
  }

int MessageHasContentEncodingBase64()
  {
  if( config==NULL )
    Error( "MessageHasContentTypeMultipart() with no CONFIG object" );
  
  for( int i=0; i<config->nHeaderLines; ++i )
    {
    char* line = config->message[i];
    if( strstr( line, "Content-Transfer-Encoding: base64")!=NULL )
      return 1;
    }

  return 0;
  }

void ScanMessageForKeywords()
  {
  if( config==NULL )
    Error( "ScanMessageForKeywords() with no CONFIG object" );

  char* separator = NULL;
  char* contentType = NULL;
  char* contentEncoding = NULL;
  int captureContent = 0;

  char* asciiBuf = NULL;
  char* readPtr = NULL;
  char* endPtr = NULL;
  int asciiBufSize = 0;

  if( config->nLines==0
      || config->nHeaderLines==0
      || config->message==NULL )
    return;

  /* first, see if we have keywords in the raw message */
  ProcessContentInRawMessage();

  if( MessageHasContentTypeMultipart()==0 )
    {
#ifdef DEBUG
        printf( "MessageHasContentTypeMultipart - false\n" );
#endif
    /* might have to decode the body itself */
    if( MessageHasContentTypeText() )
      {
#ifdef DEBUG
      printf( "MessageHasContentTypeText()\n" );
#endif
      if( MessageHasContentEncodingBase64() )
        {
#ifdef DEBUG
        printf( "MessageHasContentEncodingBase64()\n" );
#endif
        int nMessageBytes = 0;
        for( int i=config->nHeaderLines; i<config->nLines; ++i )
          nMessageBytes += strlen( config->message[i] );

        char* asciiBuf = (char*)SafeCalloc( nMessageBytes+10, sizeof(char), "decode msg buf" );
        char* ptr = asciiBuf;
        for( int i=config->nHeaderLines; i<config->nLines; ++i )
          {
          char* line = config->message[i];
          strcpy( ptr, line );
          ptr += strlen( ptr );
          }

        ProcessContentInEncodedChunk( asciiBuf, ptr-asciiBuf );

        free( asciiBuf );
        }
      }

    return;
    }

  if( config->searchEncodedTextAttachments==0 )
    return; /* we were not asked to do this */

  /* Next, scan the message for MIME encoded blocks that are
   * (a) type text/html or text/plain or text/something; and
   * (b) base64 encoded, so the above scan would not have done
   * much with them.
   * 
   * For each such block, decode the base64 chunk and scan it
   * for matches against our strings.
   */
  enum scanning_states state = ss_for_blank;

  for( int i=config->nHeaderLines; i<config->nLines; ++i )
    {
    char* line = config->message[i];
    switch( state )
      {
      case ss_for_blank:
#ifdef DEBUG
        printf( "SS_FOR_BLANK: %s\n", line );
#endif
        if( *line==0 )
          state = ss_for_separator;
        else if( NOTEMPTY( separator ) )
          {
          if( strncmp( line, "Content-Type: ", 14 )==0
              || strncmp( line, "Content-Transfer-Encoding: ", 27 )==0
              || strstr( line, ": " )!=NULL )
            { /* already in headers */
            if( asciiBuf!=NULL )
              {
              /* Warning( "(a) Extraneous free - improperly terminated chunk?" ); */
              free( asciiBuf );
              asciiBuf = NULL;
              }

            state = ss_headers;
            goto CHECK_THIS_HEADER;
            }
          }
        break;

      case ss_for_separator:
#ifdef DEBUG
        printf( "SS_FOR_SEPARATOR: %s\n", line );
#endif
        if( strncmp( line, "--", 2 )==0 )
          {
          if( asciiBuf!=NULL )
            {
            /* Warning( "(b) Extraneous free - improperly terminated chunk?" ); */
            free( asciiBuf );
            asciiBuf = NULL;
            }

          state = ss_headers;
          separator = line;
          contentType = NULL;
          contentEncoding = NULL;
          }
        else if( *line==0 )
          { /* more blanks, okay */
          }
        else
          state = ss_for_blank;
        break;

      case ss_headers:
        CHECK_THIS_HEADER:
#ifdef DEBUG
        printf( "SS_FOR_HEADERS: %s\n", line );
#endif
        if( strncmp( line, "Content-Type: ", 14 )==0 )
          {
          contentType = line + 14;
          if( strcasestr( contentType, "multipart" )!=NULL )
            { /* we are not unpacking multipart objects */
            state = ss_for_blank;
            contentType = NULL;
            contentEncoding = NULL;
            separator = NULL;
            }
          }
        else if( strncmp( line, "Content-Transfer-Encoding: ", 27 )==0 )
          contentEncoding = line + 27;
        else if( strchr( line, ':' )!=NULL )
          {}
        else if( *line==0 )
          {
          state = ss_content;
          if( NOTEMPTY( contentType )
              && strcasestr( contentType, "text" )!=NULL
              && NOTEMPTY( contentEncoding )
              && strcmp( contentEncoding, "base64" )==0 )
            {
            captureContent = 1;
            asciiBuf = (char*)malloc( BUFLEN );
            if( asciiBuf==NULL )
              Error( "Failed to allocate buffer for base64 decoding" );
            asciiBufSize = BUFLEN;
            readPtr = asciiBuf;
            endPtr = asciiBuf + asciiBufSize;
            }
          else
            captureContent = 0;
#ifdef DEBUG
          printf( "Content-Type: %s\n", NULLPROTECT( contentType ) );
          printf( "Content-Encoding: %s\n", NULLPROTECT( contentEncoding ) );
#endif
          }
        break;

      case ss_content:
#ifdef DEBUG
        printf( "SS_CONTENT: %s\n", line );
#endif
        if( *line==0 )
          { /* Probably blank after content */
          /* do base64 decode here */
          int nBytes = readPtr - asciiBuf;
          if( nBytes>0 )
            {
            ProcessContentInEncodedChunk( asciiBuf, nBytes );

            free( asciiBuf );
            asciiBuf = NULL;
            asciiBufSize = 0;
            readPtr = NULL;
            endPtr = NULL;
            captureContent = 0;
            }
          }
        else if( strcmp( line, separator )==0 )
          state = ss_for_blank;
        else
          { /* capture content here */
          if( captureContent )
            {
            int l = strlen( line );

            if( (endPtr - readPtr - 1) >= l )
              {
              int pos = readPtr - asciiBuf;
              asciiBuf = (char*)realloc( asciiBuf, asciiBufSize + BUFLEN );
              if( asciiBuf==NULL )
                Error( "Failed to expand buffer for base64 decoding" );
              asciiBufSize += BUFLEN;
              readPtr = asciiBuf + pos;
              endPtr = asciiBuf + asciiBufSize;
              }
            memcpy( readPtr, line, l );
            readPtr += l;
            *readPtr = 0;
            }
          }
        break;
      }
    } /* lines */

  if( asciiBuf!=NULL )
    {
    /* Warning( "(c) Extraneous free - improperly terminated chunk?" ); */
    free( asciiBuf );
    asciiBuf = NULL;
    }
  }
