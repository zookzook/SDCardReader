#include <SPI.h>
#include "structs.h"
#include "sdcard.h"

uint8_t       lastStatus;
SDFile        file;
SDCartInfo    sdCardInfo;
byte          sdBuffer[512];
uint16_t      fatBuffer[256];

/**
 * SD-Karte auswählen
 */
void sd_selectSDCard() {
    digitalWrite(sdCardInfo.csPin, LOW);
}

/**
 * Auswahl aufheben
 */
void sd_deselectedSDCard() {  
    digitalWrite(sdCardInfo.csPin, HIGH);
}

/**
 * Sende einen SD-Card-Befehl
 * cmd der Befehl
 * arg der Parameter
 *
 * Wartet das Ergebnis ab und liefert es zurück
 */
uint8_t sd_doSDCardCommand(uint8_t cmd, uint32_t arg) {

  uint8_t result= 0x00;

  // send command
  SPI.transfer(cmd | 0x40);

  // send argument
  for (int8_t s = 24; s >= 0; s -= 8) 
    SPI.transfer(arg >> s);

  // send CRC
  uint8_t crc = 0XFF;
  if (cmd == CMD0) 
    crc = 0X95;  // correct crc for CMD0 with arg 0
  if (cmd == CMD8) 
    crc = 0X87;  // correct crc for CMD8 with arg 0X1AA
  SPI.transfer(crc);

  // wait for response
  for (uint8_t i = 0; ((result = SPI.transfer(0xFF)) & 0x80) && i != 0xFF; i++);
  
  lastStatus= result;
  
  return result;
}

/**
 * Sende einen ASC-Command (escape for application specific command)
 */
uint8_t sd_doASCSDCardCommand(uint8_t cmd, uint32_t arg) {
    sd_doSDCardCommand(CMD55, 0);
    return sd_doSDCardCommand(cmd, arg);
}

/**
 * Initialisierungsroutine für die SD-Karte
 */
bool sd_initSDCard( void ) {
  
  uint8_t result= 0x00;
  
  // Aktuell haben wir keinen FAT-Sektor geladen. Das markieren wir hiermit:
  sdCardInfo.fatSector= 0;
  
  // unter 400khz setzen
  SPI.setClockDivider( SPI_CLOCK_DIV128 );

  // ca. 74 Zyklen warten
  for (uint8_t i = 0; i < 10; i++) 
    SPI.transfer(0xFF);
  
  sd_selectSDCard();
  
  // CMD0 mit timeout aufrufen
  uint32_t t0 = millis();
  
  while( (result = sd_doSDCardCommand(CMD0, 0)) != R1_IDLE_STATE) {
    if( (millis() - t0) > SD_INIT_TIMEOUT ) {
      break;
    } // if 
  } // while
  
  // alles ok, dann ACMD41 aufruf, womit die Karte initialisiert wird.
  if( result == R1_IDLE_STATE) {    
    uint32_t t0 = millis();
    while ((result = sd_doASCSDCardCommand(ACMD41, 0)) != R1_READY_STATE) {
      // check for timeout
      if( (millis() - t0) > SD_INIT_TIMEOUT ) {
        break;
      } // if 
    } // whilte
  } // if 
  
  sd_deselectedSDCard();  

  SPI.setClockDivider( SPI_CLOCK_DIV4 );  

  return result == R1_READY_STATE;
}

/**
 * Liefert einen Sektor aus
 * sector: Sektornummer
 * dst   : Zielbuffer
 * len   : Länge, sollte zum Zielbuffer passen
 */
bool sd_readSectorFromSDCard(uint32_t sector, byte* dst ) {
  
  bool result       = false;
  uint8_t  cardValue= 0x00;
  uint32_t  sdAddr  = sector * 512; // sector << 9;

  sd_selectSDCard();
  
  uint32_t t0= millis();
  while( (cardValue = sd_doSDCardCommand(CMD17, sdAddr)) != R1_READY_STATE) {
    if( (millis() - t0) > SD_READ_TIMEOUT ) {
      break;
    } // if 
  } // while
    
  // Block lesen
  if( cardValue == R1_READY_STATE ) {
    
    // warte bis daten vorliegen...
    uint32_t t0 = millis();     
    while((cardValue = SPI.transfer(0xFF)) == 0xFF) {
      if( (millis() - t0) > SD_READ_TIMEOUT) {
        break;
      } // if 
    } // while
  } // if
  
  // liegen die Daten nun vor?
  if( cardValue == DATA_START_BLOCK) {      

    uint16_t i= 0;
    for( i= 0; i < 512; i++ ) {
      dst[i] = SPI.transfer(0xFF);
    } // while

    // skip crc
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
    result= true;
  } // if 
  else
    result= false;

  sd_deselectedSDCard();  
  
  return result;
}

/**
 * Lese alle notwendigen Informationen über die SD-Karte.
 */
bool sd_readSDCardInfos() {

  bool result= false;
  // MBS lesen
  if( sd_readSectorFromSDCard( 0, sdBuffer ) ) {
    
    mbr_t *first= (mbr_t*)sdBuffer;   
    // merke den logischen Startsektor, diesen lesen wir nun.
    sdCardInfo.volumeStartSector = first->part[0].firstSector;   
    if( sd_readSectorFromSDCard( sdCardInfo.volumeStartSector, sdBuffer ) ) {        
      
      fbs_t *fbs= (fbs_t*)sdBuffer;
      bpb_t *bpb = &fbs->bpb;
        
      sdCardInfo.sectorsPerCluster = bpb->sectorsPerCluster;
      sdCardInfo.sectorsPerFat     = bpb->sectorsPerFat16 ? bpb->sectorsPerFat16 : bpb->sectorsPerFat32;
      sdCardInfo.fatStartSector    = sdCardInfo.volumeStartSector + bpb->reservedSectorCount;
      sdCardInfo.rootDirSector     = sdCardInfo.fatStartSector + bpb->fatCount * sdCardInfo.sectorsPerFat;
      sdCardInfo.dataStartSector   = sdCardInfo.rootDirSector + ((32 * bpb->rootDirEntryCount + 511)/512);
      result= true;
    } // if
  } // if 
  
  return result;
}

/**
 * Berechnet aus einem Cluster den Sektor.
 */
uint32_t sd_cluster2Sector( uint32_t cluster ) {
  
  uint32_t result= cluster - 2;
  result = result * sdCardInfo.sectorsPerCluster;
  result = result + sdCardInfo.dataStartSector;
        
  return result;
}

/**
 * Öffnet eine Datei
 */
bool sd_openFile( dir_t *f ) {  
  file.cursorInSector= 0;
  file.filePosition  = 0;
  file.length        = f->fileSize;
  file.sectorCounter = 0;
  file.currentCluster= f->firstClusterLow;
  file.currentSector = sd_cluster2Sector( file.currentCluster );  
  return sd_readSectorFromSDCard( file.currentSector, sdBuffer );
}

/**
 * Ersten Verzeichniseintrag aus dem Root-Verzeichnis laden
 */
bool sd_firstRootDirEntry() {
  file.currentSector = sdCardInfo.rootDirSector;  
  return sd_readSectorFromSDCard( file.currentSector, sdBuffer );
}

/**
 * Nächsten Eintrag laden
 */
bool sd_nextRootDirEntry() {  
  file.currentSector++;  
  return sd_readSectorFromSDCard( file.currentSector, sdBuffer );
}

/**
 * Prüfung, ob das Ende des Root-Verzeichnis erreicht wurde.
 */
bool sd_nextRootDirEntryAvailable() {
  return file.currentSector + 1 < sdCardInfo.dataStartSector;
}

/**
 * Haben wir das Ende erreicht?
 */
bool sd_dataAvailable() {
  return file.filePosition < file.length;
}

/**
 * wähle den nächsten Cluster aus und lade den Sektor
 */
bool sd_selectNextCluster() {
  
  bool result         = true;
  uint16_t nextCluster= file.currentCluster; // * 2;
  uint16_t sector     = nextCluster >> 8; // / 256;
  uint16_t offset     = nextCluster & 0xFF; //% 256;

  if( sdCardInfo.fatStartSector + sector != sdCardInfo.fatSector ) {
    sdCardInfo.fatSector= sdCardInfo.fatStartSector + sector;
    result= sd_readSectorFromSDCard( sdCardInfo.fatSector, (byte*)fatBuffer );
  } // if 

  if( result ) {
    file.currentCluster= fatBuffer[offset];
    file.currentSector = sd_cluster2Sector( file.currentCluster );      
    file.sectorCounter = 0;
    result             = sd_readSectorFromSDCard( file.currentSector, sdBuffer );      
  } // if 
  
  return result;
}

/**
 * Lade ein nächstes Byte
 */
uint8_t sd_readByte() {
  
  if( file.cursorInSector >= BUFFER_LENGTH ) {        
    file.cursorInSector= 0;
    file.sectorCounter+= 1;
    if( file.sectorCounter >= sdCardInfo.sectorsPerCluster ) {
      sd_selectNextCluster();
    }
    else {
      file.currentSector+= 1;
      sd_readSectorFromSDCard( file.currentSector, sdBuffer );
    } // else
  } // if 
  
  file.filePosition++;
  
  return sdBuffer[file.cursorInSector++];
}

bool sd_sdCardBegin( int csPin ) {

  // set the slaveSelectPin as an output:
  pinMode( csPin, OUTPUT);  
  digitalWrite( csPin, HIGH );  
  sdCardInfo.csPin= csPin;
  SPI.begin();   
  return sd_initSDCard() && sd_readSDCardInfos();  
}


