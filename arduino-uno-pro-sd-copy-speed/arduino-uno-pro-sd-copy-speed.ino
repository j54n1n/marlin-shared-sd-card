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

enum Settings {
  PIN_SD_CS = 4,
  BUF_SIZE_DEFAULT = 64,
  BUF_SIZE_MIN = 1,
  BUF_SIZE_MAX = 512,
};

void setup() {
  Serial.begin(115200);
  while(!Serial) { }
  Serial.print(F("Type any character to start "));
  Serial.println(F("or indicate buffer size [1..512] bytes:"));
}

void loop() {
  // When we receive a character do the SD card magic switcheroo ^^
  if(Serial.available()) {
    size_t bufferSize = BUF_SIZE_DEFAULT;
    SdFat sd;
    long pBufSize = Serial.parseInt();
    if((pBufSize >= BUF_SIZE_MIN) && (pBufSize <= BUF_SIZE_MAX)) {
      bufferSize = pBufSize;
    }
    // TODO: update detect pin for SD card ejected
    beginSdCard(sd, PIN_SD_CS);
    getCardInfo(sd);
    listFiles(sd);
    modifyFile(sd, bufferSize);
    releaseSdCard(sd);
    // TODO: update detect pin for SD card inserted
    // TODO: instruct the printer to start a print
    Serial.print(F("Type any character to restart "));
    Serial.println(F("or indicate buffer size [1..512] bytes:"));
    // Discard up any remaining characters.
    while(Serial.available()) {
      int dummyData = Serial.read();
    }
  }
  // Else forward SD card detect signal.
  // Use for example M20 to list the sd card.
  // Open a file on sdcard for writing (streaming) with Gcode "M28".
  // Then send the Gcode file contents. There exists also a binary option.
  // Close file with "M29". Start print with "M23" and "M24"
}

void beginSdCard(SdFat &sd, int cs_pin) {
  // Initialize SdFat. Use half speed like the native library.
  // Change to SPI_FULL_SPEED for more performance.
  if(!sd.begin(cs_pin, SPI_FULL_SPEED)) { //SPI_HALF_SPEED)) {
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

void printCopySpeed(size_t bytes, bool restart = false) {
  static size_t lastBytes = 0;
  static unsigned long lastMillis = 0;
  if(restart == true) {
    lastBytes = 0;
    lastMillis = millis();
    return;
  }
  // Calculate throughput in kB/s.
  size_t currentBytes = bytes;
  size_t deltaBytes = currentBytes - lastBytes;
  unsigned long currentMillis = millis();
  unsigned long deltaMillis = currentMillis - lastMillis;
  size_t copySpeed_kB = deltaBytes / deltaMillis;
  
  Serial.print(F(" copied: "));
  Serial.print(bytes);
  Serial.print(F(" Byte(s) @ "));
  Serial.print(copySpeed_kB);
  Serial.println(F(" kB/s"));
}

void modifyFile(SdFat &sd, const size_t bufferSize) {
  const __FlashStringHelper * srcFileName = F("testcube.gcode");
  const char *dstFileName = "test.gco"; // DOS 8.3
  File srcFile;
  File dstFile;
  size_t nBytes;
  uint8_t * dBuffer = nullptr; //[896];
  size_t nCopied = 0;
  size_t nBytesCopied = 0;
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
    dBuffer = (uint8_t *)malloc(bufferSize);
    Serial.print(F("Copying file to '"));
    Serial.print(dstFileName);
    Serial.print(F("' using "));
    Serial.print(bufferSize);
    Serial.println(F(" byte(s) buffer"));
    printCopySpeed(0, true);
    while((nBytes = srcFile.read(dBuffer, bufferSize)) > 0) {
      dstFile.write(dBuffer, nBytes);
      //Serial.print(F("."));
      nCopied++;
      nBytesCopied += nBytes;
      if((nCopied % 40) == 0) {
        printCopySpeed(nBytesCopied);
      }
    }
    printCopySpeed(nBytesCopied);
    srcFile.close();
    dstFile.close();
    Serial.println(F("Done!"));
    free(dBuffer);
    dBuffer = nullptr;
  }
}

void releaseSdCard(SdFat &sd) {
  // Release and tri-state SPI bus.
  sd.card()->spiStop();
  SPI.end();
  Serial.println(F("SD card released."));
}
