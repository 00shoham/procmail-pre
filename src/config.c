#include "base.h"

_CONFIG* config = NULL;

void FreeConfig()
  {
  if( config==NULL )
    return;

  FreeKeywords();
  FreeMessage();

  if( NOTEMPTY( config->contentType ) )
    {
    FREE( config->contentType ) ;
    }
  if( NOTEMPTY( config->contentEncoding ) )
    {
    FREE( config->contentEncoding ) ;
    }

  free( config );
  config = NULL;
  }

void InitConfig()
  {
  FreeConfig();

  config = (_CONFIG*)SafeCalloc( 1, sizeof(_CONFIG), "CONFIG" );

  SetDefaultKeywords();
  SetDefaultMessage();

  config->mergeHeaderLines = 0;
  config->decodeHeaderLines = 0;
  config->searchEncodedTextAttachments = 0;
  config->searchRawMessage = 0;

  config->contentType = NULL;
  config->contentEncoding = NULL;
  }
