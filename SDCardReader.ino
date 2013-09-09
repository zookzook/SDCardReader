#include <AudioShield.h>
#include <SPI.h>
#include "sdcard.h"

bool sdCardAccess= false;

void setup() {
 
  sdCardAccess= sd_sdCardBegin( SD_CARD_SS_PIN );
  VS1011.begin();  
  VS1011.SetVolume( 10, 10 );  
}

void loop() {
    if( sdCardAccess && sd_firstRootDirEntry() ) {      
        
      
      dir_t *d= (dir_t*)sdBuffer;
      /*
      for( uint8_t j=0; j < 20; j++ ) {
        Serial.print( "name:'" );
        Serial.print( j );
        for( uint8_t i= 0; i < 11; i++ )
          Serial.print( (char)d[j].name[ i ] );
        Serial.println( "'" );
        
      } */
                 
      if( sd_openFile( &d[13] ) ) {
        
        VS1011.UnsetMute();
        while( sd_dataAvailable() ) {          
          unsigned char b[32];
          uint8_t j= 0;
          while( sd_dataAvailable() && j < 32 ) {
            b[j]= sd_readByte();
            j++;
          }
          if( j > 0 )
            VS1011.Send32( b );
        } // while
              
        VS1011.Send2048Zeros();
        VS1011.SetMute();
      } // if
    } // if 
  
}

