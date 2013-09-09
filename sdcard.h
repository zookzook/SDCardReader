#ifndef __SDCARD_H
#define __SDCARD_H
/**
 * 
 */
 
#include "structs.h"

#define SD_CARD_SS_PIN 4 

struct SDCartInfo {  
  uint32_t volumeStartSector;
  uint16_t sectorsPerFat;
  uint8_t  sectorsPerCluster;
  uint32_t fatStartSector;   
  uint32_t rootDirSector;
  uint32_t dataStartSector;
  uint32_t fatSector;
  uint8_t  csPin;
};

struct SDFile {  
  uint16_t cursorInSector;
  uint32_t currentSector;
  uint32_t currentCluster;
  uint8_t  sectorCounter;
  uint32_t length;
  uint32_t filePosition;
};

#define BUFFER_LENGTH 512

extern uint8_t       lastStatus;
extern SDFile        file;
extern SDCartInfo    sdCardInfo;
extern byte          sdBuffer[512];
extern uint16_t      fatBuffer[256];

/**
 * SD-Karte auswählen
 */
void sd_selectSDCard();

/**
 * Auswahl aufheben
 */
void sd_deselectedSDCard();

/**
 * Sende einen SD-Card-Befehl
 * cmd der Befehl
 * arg der Parameter
 *
 * Wartet das Ergebnis ab und liefert es zurück
 */
uint8_t sd_doSDCardCommand(uint8_t cmd, uint32_t arg);

/**
 * Sende einen ASC-Command (escape for application specific command)
 */
uint8_t sd_doASCSDCardCommand(uint8_t cmd, uint32_t arg);

/**
 * Initialisierungsroutine für die SD-Karte
 */
bool sd_initSDCard( void );

/**
 * Liefert einen Sektor aus
 * sector: Sektornummer
 * dst   : Zielbuffer
 * len   : Länge, sollte zum Zielbuffer passen
 */
bool sd_readSectorFromSDCard(uint32_t sector, byte* dst );

/**
 * Lese alle notwendigen Informationen über die SD-Karte.
 */
bool sd_readSDCardInfos();
/**
 * Berechnet aus einem Cluster den Sektor.
 */
uint32_t sd_cluster2Sector( uint32_t cluster );

/**
 * Öffnet eine Datei
 */
bool sd_openFile( dir_t *f );

/**
 * Ersten Verzeichniseintrag aus dem Root-Verzeichnis laden
 */
bool sd_firstRootDirEntry();

/**
 * Nächsten Eintrag laden
 */
bool sd_nextRootDirEntry();

/**
 * Prüfung, ob das Ende des Root-Verzeichnis erreicht wurde.
 */
bool sd_nextRootDirEntryAvailable();

/**
 * Haben wir das Ende erreicht?
 */
bool sd_dataAvailable();

/**
 * wähle den nächsten Cluster aus und lade den Sektor
 */
bool sd_selectNextCluster();

/**
 * Lade ein nächstes Byte
 */
uint8_t sd_readByte();

bool sd_sdCardBegin( int csPin );

#endif
