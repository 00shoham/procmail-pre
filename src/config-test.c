#include "base.h"

int main( int argc, char** argv )
  {
  char* keywordsFile = NULL;
  char* messageFile = NULL;
  char* fromBase64 = NULL;
  char* fromQP = NULL;
  char* toBase64 = NULL;
  char* fromBase64Text = NULL;
  int printKeywords = 0;
  int printMessage = 0;
  int maskOutput = 0;

  InitConfig();

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-keywords" )==0 && i+1<argc )
      {
      keywordsFile = argv[++i];
      }
    else if( strcmp( argv[i], "-message" )==0 && i+1<argc )
      {
      messageFile = argv[++i];
      }
    else if( strcmp( argv[i], "-fromBase64" )==0 && i+1<argc )
      {
      fromBase64 = argv[++i];
      }
    else if( strcmp( argv[i], "-fromQP" )==0 && i+1<argc )
      {
      fromQP = argv[++i];
      }
    else if( strcmp( argv[i], "-fromBase64Text" )==0 && i+1<argc )
      {
      fromBase64Text = argv[++i];
      }
    else if( strcmp( argv[i], "-mask" )==0 )
      {
      maskOutput = 1;
      }
    else if( strcmp( argv[i], "-toBase64" )==0 && i+1<argc )
      {
      toBase64 = argv[++i];
      }
    else if( strcmp( argv[i], "-printkeywords" )==0 )
      {
      printKeywords = 1;
      }
    else if( strcmp( argv[i], "-printmessage" )==0 )
      {
      printMessage = 1;
      }
    else if( strcmp( argv[i], "-h" )==0 )
      {
      printf("USAGE: %s [-keywords fileName] [-printkeywords] [-message fileName|-] [-printmessage]\n", argv[0] );
      printf("          [-toBase64 fileName] [-fromBase64 fileName] [-fromBase64Text text [-mask]]\n" );
      printf("          [-fromQP fileName]\n" );
      exit(0);
      }
    else
      {
      printf("ERROR: unknown argument [%s]\n", argv[i] );
      exit(1);
      }
    }

  if( EMPTY( keywordsFile )
      && EMPTY( messageFile )
      && EMPTY( fromBase64 )
      && EMPTY( fromBase64Text )
      && EMPTY( toBase64 )
      && EMPTY( fromQP )
      )
    Error( "You must specify -keywords, -message, -fromBase64, -fromBase64Text, -fromQP or -toBase64" );

  if( NOTEMPTY( keywordsFile ) )
    {
    if( FileExists( keywordsFile )!=0 )
      Error( "No such file: [%s]", keywordsFile );

    ReadKeywords( keywordsFile );

    if( printKeywords )
      PrintKeywords();
    }

  if( NOTEMPTY( messageFile ) )
    {
    if( strcmp( messageFile, "-" )==0 )
      ReadMessageFromStdin();
    else
      ReadMessageFromFilename( messageFile );

    RemoveUTFFromHeaders();

    ScanMessageForKeywords();

    if( printMessage )
      PrintMessage();
    }

  if( NOTEMPTY( toBase64 ) )
    {
    char* binaryData = NULL;
    long readBytes = FileRead( toBase64, (unsigned char**)&binaryData );
    if( readBytes>0 )
      {
      int encodedLen = 0;
      char* encoded = EncodeToBase64( binaryData, readBytes, &encodedLen );
      if( encoded==NULL || encodedLen<=0 )
        Error( "Failed to EncodeToBase64 %ld bytes from %s", readBytes, toBase64 );
      fputs( encoded, stdout );
      fputs( "\n", stdout );
      FREE( encoded );
      FREE( binaryData );
      }
    }

  if( NOTEMPTY( fromBase64 ) )
    {
    char* asciiData = NULL;
    long readBytes = FileRead( fromBase64, (unsigned char**)&asciiData );
    if( readBytes>0 )
      {
      (void)TrimTail( asciiData );
      readBytes = strlen( asciiData );

      int err = ValidBase64String( asciiData, readBytes );
      if( err )
        Error( "Read %ld bytes from %s - not valid Base64 (error=%d)", readBytes, fromBase64, err );
      int decodedLen = 0;
      unsigned char* decoded = DecodeFromBase64( asciiData, readBytes, &decodedLen );
      if( decoded==NULL || decodedLen<=0 )
        Error( "Failed to Base64Decode %ld bytes from %s", readBytes, fromBase64 );
      char* hash = HashSHA256( decoded, decodedLen );
      if( EMPTY( hash ) )
        Error( "Failed to hash decoded Base64Decode data" );
      else
        printf( "Hash of file:\n%s\n", hash );
      FREE( hash );
      FREE( decoded );
      FREE( asciiData );
      }
    }

  if( NOTEMPTY( fromQP ) )
    {
    FILE* f = NULL;
    if( strcmp( fromQP, "-" )==0 )
      f = stdin;
    else
      f = fopen( fromQP, "r" );
    if( f==NULL )
      Error( "Failed to open quoted-printable file [%s]", fromQP );

    int nLines = 0;
    char** message = ReadFileIntoLinesArray( f, &nLines );
    fclose( f );

    if( message==NULL || nLines==0 )
      Error( "No content in quoted-printable file [%s]", fromQP );

    char* decoded = DecodeQuotedPrintableMessage( message, nLines, NULL );
    if( decoded==NULL )
      Error( "Failed to decode content of [%s]", fromQP );

    FreeLinesArray( message, nLines );

    fputs( "\n\n", stdout );
    fputs( decoded, stdout );
    fputs( "\n\n", stdout );

    free( decoded );
    }

  if( NOTEMPTY( fromBase64Text ) )
    {
    char* asciiData = strdup( fromBase64Text );
    (void)TrimTail( asciiData );
    int readBytes = strlen( asciiData );

    int err = ValidBase64String( asciiData, readBytes );
    if( err )
      Error( "[%s] is not valid Base64 (error=%d)", asciiData, err );
    int decodedLen = 0;
    unsigned char* decoded = DecodeFromBase64( asciiData, readBytes, &decodedLen );
    if( decoded==NULL || decodedLen<=0 )
      Error( "Failed to Base64Decode %ld bytes from %s", readBytes, fromBase64 );
    if( maskOutput )
      MaskNonPrintableChars( decoded );
    else
      printf( "Decoded: [%s]\n", decoded );
    FREE( decoded );
    FREE( asciiData );
    }

  if( NOTEMPTY( keywordsFile ) )
    FreeKeywords();

  FreeConfig();

  return 0;
  }
