#include "base.h"

int main( int argc, char** argv )
  {
  char* configFile = NULL;
  char* messageFile = NULL;

  InitConfig();

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-config" )==0 && i+1<argc )
      {
      configFile = argv[++i];
      }
    else if( strcmp( argv[i], "-message" )==0 && i+1<argc )
      {
      messageFile = argv[++i];
      }
    else if( strcmp( argv[i], "-h" )==0 )
      {
      printf("USAGE: %s [-config fileName] [-message fileName|-]\n", argv[0] );
      exit(0);
      }
    else
      {
      printf("ERROR: unknown argument [%s]\n", argv[i] );
      exit(1);
      }
    }

  if( EMPTY( configFile ) || EMPTY( messageFile ))
    Error( "You must specify -config and -message" );

  if( FileExists( configFile )!=0 )
    Error( "No such file: [%s]", configFile );

  ReadKeywords( configFile );

  if( strcmp( messageFile, "-" )==0 )
    ReadMessageFromStdin();
  else
    ReadMessageFromFilename( messageFile );

  RemoveUTFFromHeaders();

  ScanMessageForKeywords();

  PrintMessage();

  FreeConfig();

  return 0;
  }
