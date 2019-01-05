/*
 * Arduino Uno/Pro connected to Seeed Studio SD Card Shield
 * See also: http://wiki.seeedstudio.com/SD_Card_shield_V4.0/
 *
 * This controller sketch prepares a GCode file to be printed by
 * Marlin 3D printer firmware.
 * Put a testcube.gcode file on the SD card.
 *
 * SD card/Marlin pins used:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** SCK  - pin 13
 ** CS   - pin 4
 */

#include <SPI.h>
#include <SdFat.h>

void setup() {
  Serial.begin(115200);
  while(!Serial) { }
  Serial.println(F("Type any character to start"));
}

void loop() {
  // When we receive a character do the SD card magic switcheroo ^^
  if(Serial.read() > 0) {
    enum { PIN_SD_CS = 4 };
    SdFat sd;
    // TODO: update detect pin for SD card ejected
    beginSdCard(sd, PIN_SD_CS);
    getCardInfo(sd);
    listFiles(sd);
    modifyFile(sd);
    releaseSdCard(sd);
    // TODO: update detect pin for SD card inserted
    // TODO: instruct the printer to start a print
    Serial.println(F("Type any character to restart"));
    // Discard up any remaining characters.
    while(Serial.available()) {
      int dummyData = Serial.read();
    }
  }
  // Else forward SD card detect signal.
  // Use for example M20 to list the sd card.
}

void beginSdCard(SdFat &sd, int cs_pin) {
  // Initialize SdFat. Use half speed like the native library.
  // Change to SPI_FULL_SPEED for more performance.
  if(!sd.begin(cs_pin, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
  }
}

void getCardInfo(SdFat &sd) {
  uint32_t cardSize;
  Serial.println(F("Retrieving card info: "));
  cardSize = sd.card()->cardSize();
  if(cardSize == 0) {
    sd.errorHalt(F("could not retrieve card size"));
  }
  Serial.print(F("Card type: "));
  switch(sd.card()->type()) {
    case SD_CARD_TYPE_SD1:
    Serial.println(F("SD1"));
    break;
    case SD_CARD_TYPE_SD2:
    Serial.println(F("SD2"));
    break;
    case SD_CARD_TYPE_SDHC:
    if(cardSize < 70000000) {
      Serial.println(F("SDHC"));
    } else {
      Serial.println(F("SDXC"));
    }
    break;
    default:
    Serial.println(F("Unknown card type"));
  }
  Serial.print(F("Card size: "));
  Serial.print(0.000512 * cardSize);
  Serial.println(F(" MB (MB = 1,000,000 bytes)"));
}

void listFiles(SdFat &sd) {
  Serial.println(F("List of files on the SD card:"));
  sd.ls(LS_R);
}

void modifyFile(SdFat &sd) {
  const __FlashStringHelper * srcFileName = F("testcube.gcode");
  const char *dstFileName = "test.gco"; // DOS 8.3
  File srcFile;
  File dstFile;
  size_t nBytes;
  uint8_t dBuffer[64];
  size_t nCopied = 0;
  if(sd.remove(dstFileName)) {
    Serial.print(F("Removed file '"));
    Serial.print(dstFileName);
    Serial.println(F("'."));
  }
  srcFile = sd.open(srcFileName);
  if(srcFile) {
    Serial.print(F("Opened file '"));
    Serial.print(srcFileName);
    Serial.println(F("'."));
  } else {
    Serial.println(srcFileName);
    sd.errorHalt(F("File not found"));
  }
  dstFile = sd.open(dstFileName, FILE_WRITE);
  if(dstFile) {
    Serial.print(F("Copying file to '"));
    Serial.print(dstFileName);
    Serial.println(F("'."));
    while((nBytes = srcFile.read(dBuffer, sizeof(dBuffer))) > 0) {
      dstFile.write(dBuffer, nBytes);
      Serial.print(F("."));
      nCopied++;
      if((nCopied % 40) == 0) {
        size_t bytes = nCopied * sizeof(dBuffer);
        Serial.print(F(" copied: "));
        Serial.print(bytes);
        Serial.println(F(" Byte(s)"));
      }
    }
    Serial.print(F(" copied: "));
    Serial.print(nCopied * sizeof(dBuffer));
    Serial.print(F(" Byte(s)"));
    Serial.println();
    srcFile.close();
    dstFile.close();
    Serial.println(F("Done!"));
  }
}

void releaseSdCard(SdFat &sd) {
  // Release and tri-state SPI bus.
  sd.card()->spiStop();
  SPI.end();
  Serial.println(F("SD card released."));
}
