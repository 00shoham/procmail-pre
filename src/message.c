#include "base.h"

#define BLOCKS 1000

void FreeMessage()
  {
  if( config==NULL )
    Error( "FreeMessage() with no CONFIG object" );

  if( config->message==NULL && config->allocatedLines==0 )
    return;

  if( config->message==NULL )
    Error( "message==NULL but allocatedLines==%d", config->allocatedLines );

  if( config->allocatedLines==0 )
    Error( "message!=NULL but allocatedLines==0" );

  for( int i=0; i < config->nLines; ++i )
    {
    char* l = config->message[i];
    if( l!=NULL )
      {
      free( l );
      config->message[i] = NULL;
      }
    }

  free( config->message );
  config->message = NULL;
  }

void SetDefaultMessage()
  {
  if( config==NULL )
    Error( "SetDefaultMessage() with no CONFIG object" );

  FreeMessage();

  config->nLines = 0;
  config->allocatedLines = 0;
  config->message = NULL;
  config->nHeaderLines = 0;
  config->finishedHeaders = 0;
  }

void ReadMessageFromFile( FILE* f )
  {
  if( config==NULL )
    Error( "ReadMessageFromFile() with no CONFIG object" );

  if( f==NULL )
    Error( "Cannot read message from NULL file" );

  /* just in case */
  FreeMessage();

  config->message = (char**)SafeCalloc( BLOCKS, sizeof(char*), "Initial essage block" );
  config->allocatedLines = BLOCKS;

  char buf[BIGBUF];
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    TrimTail( buf );
    if( config->nLines >= config->allocatedLines )
      {
      int newSize = config->allocatedLines + BLOCKS;
      config->message = (char**)realloc( config->message, newSize * sizeof( char* ) );
      if( config->message==NULL )
        Error( "Cannot reallocate message buffer to %d lines", newSize );
      config->allocatedLines = newSize;
      }

    if( config->finishedHeaders==0 )
      {
      if( EMPTY( buf ) )
        {
        config->nHeaderLines = config->nLines;
        config->finishedHeaders = 1;
        }
      }

    if( config->finishedHeaders || config->nLines==0 )
      {
      config->message[config->nLines] = strdup( buf );
      ++ (config->nLines);
      }
    else
      { /* in headers */
      if( config->mergeHeaderLines
          && isspace( buf[0] ) )
        { /* continuation of previous header */
        -- (config->nLines);
        int l1 = strlen( config->message[config->nLines] );
        char* ptr = buf;
        while( isspace( *ptr ) )
          ++ptr;
        if( ptr>buf )
          --ptr;
        int l2 = strlen( ptr );
        config->message[config->nLines]
          = (char*)realloc( config->message[config->nLines],
                            (l1+l2+1)*sizeof(char*) );
        if( config->message[config->nLines] == NULL )
          Error( "Cannot reallocate header line %d", config->nLines );
        strncat( config->message[config->nLines], ptr, l1+l2 );
        ++ (config->nLines);
        }
      else
        {
        config->message[config->nLines] = strdup( buf );
        ++ (config->nLines);

        if( strncmp( buf, "Content-Type: ", 14 )==0 )
          {
          if( NOTEMPTY( config->contentType ) )
            {
            FREE( config->contentType );
            }
          config->contentType = strdup( buf + 14 );
          }

        if( strncmp( buf, "Content-Transfer-Encoding: ", 27 )==0 )
          {
          if( NOTEMPTY( config->contentEncoding ) )
            {
            FREE( config->contentEncoding );
            }
          config->contentEncoding = strdup( buf + 27 );
          }
        }
      }
    }
  }

void ReadMessageFromFilename( char* name )
  {
  FILE* f = fopen( name, "r" );
  if( f==NULL )
    Error( "Cannot open [%s] for reading", name );
  
  ReadMessageFromFile( f );
  fclose( f );
  }

void ReadMessageFromStdin()
  {
  ReadMessageFromFile( stdin );
  }

void PrintMessage()
  {
  if( config==NULL )
    Error( "PrintMessage() with no CONFIG object" );

  if( config->message==NULL || config->allocatedLines==0 )
    return;

  for( int i=0; i < config->nHeaderLines; ++i )
    {
    char* l = config->message[i];
    if( l!=NULL )
    fputs( l, stdout );
    fputs( "\n", stdout );
    }

  PrintKeywordHeaders();

  for( int i = config->nHeaderLines; i < config->nLines; ++i )
    {
    char* l = config->message[i];
    if( l!=NULL )
    fputs( l, stdout );
    fputs( "\n", stdout );
    }
  }
