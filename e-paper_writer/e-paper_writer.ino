/*
   e-paper writer (c) juh 2025

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.

   Includes MIT licenced code by J-M-L Jackson from the Arduino Forum
   https://forum.arduino.cc/t/uploading-various-byte-streams-to-an-esp32-using-espasncwebserver/1233455

   I heavily extended J-M-L's code with e-paper uploads, QR-code upload, PNG file management
     and more with many code snippets elicited by my natural and fully biodegradable
     intelligence from the GPT 4o ai model.

   Warning: The current state of this is "it works for me", not more. The result
   looks good to me, but the code reflects little planning and structure in the
   development process, it needs a lot of cleanup, streamlining, and optimization.

   See README.md for compilation and usage.

*/

/* ============================================
  Bellow is the original license for parts used from J-M-L Jackson from the Arduino Forum:
  https://forum.arduino.cc/t/uploading-various-byte-streams-to-an-esp32-using-espasncwebserver/1233455

  code is placed under the MIT license
  Copyright (c) 2024 J-M-L
  For the Arduino Forum : https://forum.arduino.cc/u/j-m-l

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  ===============================================
*/

#include "SPIFFS.h"

// install qrcode.h by ricmoo from the library manager and follow these steps
// to make it compile for ESP32: https://github.com/ricmoo/QRCode/issues/35#issuecomment-1179311130
#include "qrcoderm.h" // yes, this is the correct include after the above modification

#include <vector>
#include <algorithm> // for file list sorting

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include <PNGdec.h> // for png decoding
#include "esp_system.h"  // For ESP-IDF functions (display free RAM)

#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>


/*
   ==================== Start of user configuration ====================
*/

// (1) GxEPD2 e-paper config

// Best try to make your display work with some of the GxEPD2 examples, first, and put that config here if all works fine.

//ESPP32 Devkitc_V4
//GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> display(GxEPD2_213_Z98c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEY0213Z98 122x250, SSD1680
// The above uses the default/hardware SPI MOSI/MISO pins for ePaper SDA and SCL, other pins as defined.

// ESP32 C3 mini
GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> display(GxEPD2_213_Z98c(/*CS=*/ 7, /*DC=*/ 8, /*RST=*/ 9, /*BUSY=*/ 10)); // GDEY0213Z98 122x250, SSD1680
// The above uses the default/hardware SPI MOSI/MISO pins for ePaper SDA and SCL, other pins as defined.


// (2) Put your display's dimensions here.
//     We need these values as defines for the HTML string literal in html.h,
//     so we cannot use the display object's methods to access them.

#define EPAPER_WIDTH 250
#define EPAPER_HEIGHT 122
const uint8_t display_rotation = 1; // 1 or 3 for landscape; use 0 or 2 if you want width and height switched


// (3) WiFi config

#include "../../wifi_credentials.h" // use this file or delete this line and put your credentials here:
// const char* WIFI_SSID = "YourSSID";
// const char* WIFI_PASSWORD = "YourPassword";


// (4) Font config for text upload

// Replace or add the fonts you want to use below.
// Select from this list (see [*]note below if you get compile errors):
// https://github.com/olikraus/u8g2/wiki/fntlistallplain#u8g2-font-list
// Increase the array size if you want more, only memory is the limit.
// Note that some of these fonts might come with specific licensing conditions.

const uint8_t* fonts[9] = {
  /* [font1] */ u8g2_font_HelvetiPixel_tr, // identical with u8g2_font_Wizzard_tr
  /* [font2] */ u8g2_font_BitTypeWriter_tr,
  /* [font3] */ u8g2_font_t0_13_tr,
  /* [font4] */ u8g2_font_Terminal_te, // [*] needs updated U8g2_for_Adafruit_GFX.h
  /* [font5] */ u8g2_font_helvR14_tf,
  /* [font6] */ u8g2_font_t0_22b_tf,
  /* [font7] */ u8g2_font_iconquadpix_m_all,
  /* [font8] */ u8g2_font_mystery_quest_32_tr,  // [*] needs updated U8g2_for_Adafruit_GFX.h
  /* [font9] */ u8g2_font_luBS14_tf
};
/*
   [*]A note on missing U8g2 fonts ("... was not declared in this scope" compiler error)
   It seems U8g2_for_Adafruit_GFX is not up to date so it makes not all U8g2 fonts
   available to Adafruit_GFX, and thus to GxEPD. To fix, either remove those fonts from
   the list above or follow these steps:

   1. Install U8g2 library and find it in your ../Arduino/libraries/ folder.
   2. From the src/clib/ folder, get the u8g2_fonts.c (NOT: u8g2_font.c) file and copy
      it to the src folder of the U8g2_for_Adafruit_GFX library, it replaces the original.
   3. Open said file and change the #include directive from "u8g2.h" to "u8g2_fonts.h"
   4. From this repository's extra folder copy U8g2_for_Adafruit_GFX.h to the src folder 
      of the U8g2_for_Adafruit_GFX library, it replaces the original.
   5. Save and recompile.
*/

/*
   ==================== End of user configuration ====================
*/

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
AsyncWebServer server(80);

bool updateDisplay = FALSE; // flag to signal the main loop it should update the display, otherwise the update might invoke the watchdog


/*********************************************************************************

   Web server root page

 *********************************************************************************/

#include "html.h" // not a library, but our hosted page, needs to be in this folder

String webContentTemplateProcessor(const String& var)
{
  if(var == "EPAPER_WIDTH")  return String(EPAPER_WIDTH);
  if(var == "EPAPER_HEIGHT") return String(EPAPER_HEIGHT);
  return String();
}

void handleRootPage(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html, webContentTemplateProcessor);
}


/*********************************************************************************

   PNG stuff

 *********************************************************************************/

PNG png;
const char* imageBufferFileName = "/_imageBuffer.bin";
File imageBuffer;

// callbacks for PNG.decode()

void * myOpen(const char *filename, int32_t *size) {
  imageBuffer = SPIFFS.open(filename, "r");
  *size = imageBuffer.size();
  return &imageBuffer;
}
void myClose(void *handle) {
  if (imageBuffer) imageBuffer.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!imageBuffer) return 0;
  return imageBuffer.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!imageBuffer) return 0;
  return imageBuffer.seek(position);
}


/*********************************************************************************

   File manager

 *********************************************************************************/

String getFileListJSON() {
  std::vector<String> filenames;
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    filenames.push_back(String(file.name()));
    file = root.openNextFile();
  }
  file.close();  // Close file after use
  root.close();  // Close root directory
  std::sort(filenames.begin(), filenames.end());
  String json = "[";
  for (size_t i = 0; i < filenames.size(); ++i) {
    if (i > 0) json += ",";
    json += "\"" + filenames[i] + "\"";
  }
  json += "]";
  return json;
}

void handleFileListResponse(AsyncWebServerRequest *request) {
  request->send(200, "application/json", getFileListJSON());
}


void handleDisplaySelectedFile(AsyncWebServerRequest *request) {
  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing 'file' parameter");
    return;
  }
  String fileName = request->getParam("file")->value();
  String filePath = "/" + fileName;
  if (!SPIFFS.exists(filePath)) {
    request->send(404, "text/plain", "Image not found");
    return;
  }
  request->send(SPIFFS, filePath, "image/png");
}

void handleDeleteSelectedFile(AsyncWebServerRequest *request) {
  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }
  String path = request->getParam("file")->value();
  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  if (!SPIFFS.exists(path)) {
    request->send(404, "text/plain", "File not found");
    return;
  }
  if (SPIFFS.remove(path)) {
    request->send(200, "text/plain", "File deleted");
  } else {
    request->send(500, "text/plain", "Failed to delete file");
  }
}

/*********************************************************************************

   Image upload from file manager

 *********************************************************************************/

int usf_brightness = 128; // globals so PNGDrawAtkinson() callback can reach them
int usf_redness = 192;

void handleUploadSelectedFile(AsyncWebServerRequest *request) {
  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }
  String path = request->getParam("file")->value();

  String usf_rendering = "nearest";
  if (request->hasParam("rendering")) {
    usf_rendering = request->getParam("rendering")->value();
    if (usf_rendering != "nearest" && usf_rendering != "dithered") {
      request->send(400, "text/plain", "Invalid 'rendering' parameter");
      return;
    }
  }

  if (request->hasParam("brightness")) {
    usf_brightness = request->getParam("brightness")->value().toInt();
    usf_brightness = constrain(usf_brightness, 0, 255);
  }

  if (request->hasParam("redness")) {
    usf_redness = request->getParam("redness")->value().toInt();
    usf_redness = constrain(usf_redness, 0, 255);
  }

  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  if (!SPIFFS.exists(path)) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  Serial.printf("handleUploadSelectedFile: %s\n", path.c_str());
  int rc;
  if (usf_rendering == "nearest") {
    rc = png.open(path.c_str(), myOpen, myClose, myRead, mySeek, PNGDrawNearest);
  } else if (usf_rendering == "dithered") {
    rc = png.open(path.c_str(), myOpen, myClose, myRead, mySeek, PNGDrawAtkinson);
  }

  if (rc == PNG_SUCCESS) {
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    Serial.printf("free RAM: %d, getBufferSize: %d, Transparent color: %d \n", esp_get_free_heap_size(), png.getBufferSize(), png.getTransparentColor());
    //    if (png.getPixelType() == PNG_PIXEL_TRUECOLOR_ALPHA) {
    int rc = png.decode(0, 0); // no private data needed, no flags
    Serial.printf("Decode complete with return code: %d\n", rc);
    if (usf_rendering == "dithered") {
      flushRemainingLines();
    }
    updateDisplay = TRUE; // signal main loop() to update the display, here it would invoke the watchdog
    request->send(200, "text/plain", "File uploaded");
    //    } else {
    //      Serial.printf("Erorr. PNG has wrong getPixelType (%d) \n", png.getPixelType());
    //      request->send(500, "text/plain", "PNG has wrong getPixelType");
    //    }
    png.close();
  } else {
    Serial.println("PNG open failed.");
    request->send(500, "text/plain", "Failed to upload file");
  }
}

void handleDownloadSelectedFile(AsyncWebServerRequest *request) {
  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing 'file' parameter");
    return;
  }
  String filename = request->getParam("file")->value();
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }
  if (!SPIFFS.exists(filename)) {
    request->send(404, "text/plain", "File not found");
    return;
  }
  request->send(SPIFFS, filename, "application/octet-stream");
}


void handleFreeFlash(AsyncWebServerRequest *request) {
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  size_t free = total - used;
  String response = String(free);
  request->send(200, "text/plain", response);
}


/*********************************************************************************

   PNG upload (nearest and dithered)

 *********************************************************************************/

// callback
void PNGDrawNearest(PNGDRAW *pDraw)
{
  uint16_t usPixels[display.width()];
  //uint8_t ucMask[display.width() / 8];
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
  for (int16_t x = 0; x < display.width(); x++) {  // a bitmap transfer would probably be more efficient, but this is so much simpler
    uint8_t r = (usPixels[x] >> 11) & 0x1F;   // 5 bits red
    uint8_t g = (usPixels[x] >> 6) & 0x1F;    // 5 bits green (ignore lsb)
    uint8_t b = usPixels[x] & 0x1F;           // 5 bits blue

    const uint8_t redTolerance   = 31 - (usf_redness >> 3); // how much red must exceed g/b
    const uint8_t whiteTolerance = usf_brightness >> 3;  // how close to 31 all channels must be
    uint16_t c = GxEPD_BLACK; // default to black
    if ((r - g >= redTolerance) && (r - b >= redTolerance)) {
      c = GxEPD_RED;
    }
    else if (abs(r - 31) <= whiteTolerance &&
             abs(g - 31) <= whiteTolerance &&
             abs(b - 31) <= whiteTolerance) {
      c = GxEPD_WHITE;
    }

    display.drawPixel(x, pDraw->y, c);
  }
  // ### todo: interpret alpha mask
  //  if (png.getAlphaMask(pDraw, ucMask, 255)) { // if any pixels are opaque, draw them
  //    spilcdWritePixelsMasked(&lcd, pPriv->xoff, pPriv->yoff + pDraw->y, (uint8_t *)usPixels, ucMask, pDraw->iWidth, DRAW_TO_LCD);
  //  }
} /* PNGDrawNearest() */


// draw dithered //

float gray[3][EPAPER_WIDTH];     // grayscale luminance buffer
float redness[3][EPAPER_WIDTH];  // red-channel buffer
typedef uint8_t PixelMask[EPAPER_WIDTH];
PixelMask blackMask;  // result of grayscale dithering
PixelMask redMask;    // result of red-channel dithering

int currentLine = 0;

void drawProcessedLine(int y, int bufferIndex) {
  for (int x = 0; x < display.width(); x++) {
    if (redMask[x]) {
      display.drawPixel(x, y, GxEPD_RED);
    } else if (blackMask[x]) {
      display.drawPixel(x, y, GxEPD_BLACK);
    } else {
      display.drawPixel(x, y, GxEPD_WHITE);
    }
  }
}

void PNGDrawAtkinson(PNGDRAW* pDraw) {
  // Thresholds for dithering (tweak to taste)
  int grayscaleThreshold = 255 - usf_brightness;   // below = black, above = white
  int rednessThreshold = 255 - usf_redness;    // above = red pixel

  uint16_t usPixels[display.width()];
  int y = pDraw->y;
  int lineIndex = currentLine % 3;
  int nextLine1 = (currentLine + 1) % 3;
  int nextLine2 = (currentLine + 2) % 3;

  // Load current scanline in RGB565
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

  // Step 1: Convert to grayscale and redness levels
  for (int x = 0; x < display.width(); x++) {
    uint16_t p = usPixels[x];
    uint8_t r = ((p >> 11) & 0x1F) << 3;
    uint8_t g = ((p >> 5) & 0x3F) << 2;
    uint8_t b = (p & 0x1F) << 3;

    gray[lineIndex][x] = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    int redVal = (int)r - max((int)g, (int)b);
    redness[lineIndex][x] = redVal > 0 ? redVal : 0;
  }

  // Step 2: Atkinson dithering for grayscale
  for (int x = 0; x < display.width(); x++) {
    float oldPixel = gray[lineIndex][x];
    float newPixel = (oldPixel < grayscaleThreshold) ? 0 : 255;
    float err = oldPixel - newPixel;
    gray[lineIndex][x] = newPixel;
    blackMask[x] = (newPixel == 0);

    float q = err / 8.0;

    if (x + 1 < display.width())     gray[lineIndex][x + 1] += q;
    if (x + 2 < display.width())     gray[lineIndex][x + 2] += q;
    if (currentLine + 1 < display.height()) {
      if (x > 0)                     gray[nextLine1][x - 1] += q;
      gray[nextLine1][x]     += q;
      if (x + 1 < display.width())  gray[nextLine1][x + 1] += q;
    }
    if (currentLine + 2 < display.height()) {
      gray[nextLine2][x]     += q;
    }
  }

  // Step 3: Atkinson dithering for red channel
  for (int x = 0; x < display.width(); x++) {
    float oldPixel = redness[lineIndex][x];
    float newPixel = (oldPixel > rednessThreshold) ? 255 : 0;
    float err = oldPixel - newPixel;
    redness[lineIndex][x] = newPixel;
    redMask[x] = (newPixel == 255);

    float q = err / 8.0;

    if (x + 1 < display.width())     redness[lineIndex][x + 1] += q;
    if (x + 2 < display.width())     redness[lineIndex][x + 2] += q;
    if (currentLine + 1 < display.height()) {
      if (x > 0)                     redness[nextLine1][x - 1] += q;
      redness[nextLine1][x]     += q;
      if (x + 1 < display.width())  redness[nextLine1][x + 1] += q;
    }
    if (currentLine + 2 < display.height()) {
      redness[nextLine2][x]     += q;
    }
  }

  // Step 4: Draw completed line from 2 lines ago
  if (currentLine >= 2) {
    int drawLine = currentLine - 2;
    int drawIndex = drawLine % 3;
    drawProcessedLine(drawLine, drawIndex);
  }

  currentLine++;
}

// At the end of decoding, flush remaining 2 buffered lines:
void flushRemainingLines() {
  for (int i = 0; i < 2; i++) {
    int y = display.height() - 2 + i;
    int bufferIndex = y % 3;
    drawProcessedLine(y, bufferIndex);
  }
  currentLine = 0;
}


/*********************************************************************************

   Canvas management

 *********************************************************************************/

void handleCanvasBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

  if (index == 0) { // FIRST CALL
    //Serial.printf("\nIncoming PNG: %u index,  %u len, %u bytes\n", index, len, total);
    //imageBuffer = new uint8_t[total + 1]; // allocate buffer
    imageBuffer = SPIFFS.open(imageBufferFileName, "w"); // w = overwrite if exists
    display.fillScreen(GxEPD_WHITE); // clear ePaper display
  }
  //Serial.printf("%u : %u : %u : \n", index, len, total);
  //memcpy(imageBuffer + index, data, len); // store current chunk of image data in buffer
  imageBuffer.write(data, len);

  if (index + len == total) { // END CALL, WE ARE DONE
    imageBuffer.close(); // close for writing
    //Serial.printf("\nPNG transfer complete (%u bytes)\n", total);
    // for (size_t i = 0; i < total; i++) Serial.printf("%02x", imageBuffer[i]); // output raw data as HEX
    // Serial.println();

    int rc = png.open(imageBufferFileName, myOpen, myClose, myRead, mySeek, PNGDrawNearest);

    if (rc == PNG_SUCCESS) {
      //Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      //Serial.printf("free RAM: %u, getBufferSize: %u, Transparent color: %u \n", esp_get_free_heap_size(), png.getBufferSize(), png.getTransparentColor());
      if (png.getPixelType() == PNG_PIXEL_TRUECOLOR_ALPHA) {
        // do the decode. We do did not set a decode buffer, so decode() will invoke the callback for each line of the image
        // the callback will do the actual drawing of the image
        int rc = png.decode(0, 0); // no private data needed, no flags
        //Serial.printf("Decode complete with return code: %d\n", rc);
        updateDisplay = TRUE; // signal main loop() to update the display, here it would invoke the watchdog
      } else {
        //Serial.printf("Erorr. PNG has wrong getPixelType (%d) \n", png.getPixelType());
      }
      png.close();
    } else {
      //Serial.println("PNG open failed.");
    }
  }
}

void handleCanvasResponse(AsyncWebServerRequest * request) {
  Serial.println("End of PNG request.");
  request->send(200);
}

/* save canvas to given file name */
void handleSaveCanvasBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

  if (index == 0) { // FIRST CALL
    Serial.printf("\nIncoming PNG: %u index,  %u len, %u bytes\n", index, len, total);
    String filename = "/default.png"; // fallback
    if (request->hasParam("filename")) {
      filename = "/" + request->getParam("filename")->value();
    }
    Serial.printf("Saving to: %s\n", filename.c_str());
    imageBuffer = SPIFFS.open(filename, "w"); // w = overwrite if exists
  }
  Serial.printf("%u : %u : %u : \n", index, len, total);
  //memcpy(imageBuffer + index, data, len); // store current chunk of image data in buffer
  imageBuffer.write(data, len);

  if (index + len == total) { // END CALL, WE ARE DONE
    imageBuffer.close(); // close for writing
    Serial.printf("\nPNG transfer complete (%u bytes)\n", total);
  }
}

void handleSaveCanvasResponse(AsyncWebServerRequest * request) {
  Serial.println("End of PNG request.");
  request->send(200);
}


/*********************************************************************************

   Save file to flash

 *********************************************************************************/

void handleSaveFile(AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {   // multiple calls possible

  if (index == 0) { // FIRST CALL
    Serial.printf("\nReceiving file %s\n", filename.c_str());
    imageBuffer = SPIFFS.open(("/" + filename).c_str(), "w"); // w = overwrite if exists ### implement file management
  }

  Serial.printf("\n index=%u; len=%u; final=%u\n", index, len, final);
  imageBuffer.write(data, len); // store data

  if (final) { // END CALL, WE ARE DONE
    imageBuffer.close();  // close for writing
  }
}

void handleSaveFileResponse(AsyncWebServerRequest * request) {
  Serial.println("End of File request.");
  request->send(200);
}


/*********************************************************************************

   Text upload management

 *********************************************************************************/

void renderMarkedUpText(const String &text) {
  uint16_t fg = GxEPD_BLACK;
  uint16_t bg = GxEPD_WHITE;
  const uint8_t *font = fonts[0];
  String align = "left";
  int lineSpacing = 2;

  u8g2Fonts.setFont(font);
  u8g2Fonts.setForegroundColor(fg);
  u8g2Fonts.setBackgroundColor(bg);

  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();

  std::vector<String> lines;
  int start = 0;
  int end = 0;

  while ((end = text.indexOf('\n', start)) != -1) {
    lines.push_back(text.substring(start, end));
    start = end + 1;
  }
  if (start < text.length()) lines.push_back(text.substring(start));

  // Check for global background tag at the start of the first line
  if (!lines.empty()) {
    String &firstLine = lines[0];
    int i = 0;
    while (firstLine[i] == '[') {
      int close = firstLine.indexOf(']', i);
      if (close == -1) break;
      String tag = firstLine.substring(i + 1, close);
      if (tag == "BLACK") bg = GxEPD_BLACK;
      else if (tag == "WHITE") bg = GxEPD_WHITE;
      else if (tag == "RED")   bg = GxEPD_RED;
      else break;
      i = close + 1;
    }
    display.fillScreen(bg);
  }

  int y = 1;

  for (auto &line : lines) {
    int i = 0;
    // parse [tags] at beginning
    while (line[i] == '[') {
      int close = line.indexOf(']', i);
      if (close == -1) break;
      String tag = line.substring(i + 1, close);

      // font
      if (tag.startsWith("font")) {
        int index = tag.substring(4).toInt();
        if (index >= 1 && index <= sizeof(fonts) / sizeof(fonts[0])) {
          font = fonts[index - 1];
        }
      }
      // alignment
      else if (tag == "left" || tag == "center" || tag == "right") {
        align = tag;
      }
      // inline fg color at line start
      else if (tag == "black") fg = GxEPD_BLACK;
      else if (tag == "white") fg = GxEPD_WHITE;
      else if (tag == "red")   fg = GxEPD_RED;
      // line spacing
      else if (tag.toInt() >= 0) {
        lineSpacing = tag.toInt();
      }

      u8g2Fonts.setForegroundColor(fg);
      u8g2Fonts.setBackgroundColor(bg);
      i = close + 1;
    }

    u8g2Fonts.setFont(font);
    lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();

    // Strip tags for alignment width calculation
    String visibleText;
    int tempIndex = i;
    while (tempIndex < line.length()) {
      if (line[tempIndex] == '[') {
        if (tempIndex + 1 < line.length() && line[tempIndex + 1] == '[') {
          visibleText += '[';
          tempIndex += 2;
          continue;
        }
        int close = line.indexOf(']', tempIndex);
        if (close != -1) {
          String tag = line.substring(tempIndex + 1, close);
          if (tag == "black" || tag == "white" || tag == "red" ||
              tag == "BLACK" || tag == "WHITE" || tag == "RED" ||
              tag == "left" || tag == "center" || tag == "right" ||
              tag.startsWith("font") || tag.toInt() >= 0) {
            tempIndex = close + 1;
            continue;
          }
        }
      } else if (line[tempIndex] == ']' && tempIndex + 1 < line.length() && line[tempIndex + 1] == ']') {
        visibleText += ']';
        tempIndex += 2;
        continue;
      }
      visibleText += line[tempIndex++];
    }
    int textWidth = u8g2Fonts.getUTF8Width(visibleText.c_str());
    int x = 0;
    if (align == "center") x = (display.width() - textWidth) / 2;
    else if (align == "right") x = display.width() - textWidth;

    int cursorX = x;
    int cursorY = y + u8g2Fonts.getFontAscent();
    u8g2Fonts.setCursor(cursorX, cursorY);

    // Improved inline tag parsing at any position
    while (i < line.length()) {
      if (line[i] == '[') {
        if (i + 1 < line.length() && line[i + 1] == '[') {
          u8g2Fonts.print('[');
          i += 2;
          continue;
        }
        int close = line.indexOf(']', i);
        if (close != -1) {
          String tag = line.substring(i + 1, close);

          // foreground colors
          if (tag == "black") fg = GxEPD_BLACK;
          else if (tag == "white") fg = GxEPD_WHITE;
          else if (tag == "red")   fg = GxEPD_RED;

          u8g2Fonts.setForegroundColor(fg);
          u8g2Fonts.setBackgroundColor(bg);
          i = close + 1;
          continue;
        }
      } else if (line[i] == ']' && i + 1 < line.length() && line[i + 1] == ']') {
        u8g2Fonts.print(']');
        i += 2;
        continue;
      }

      // Normal character
      u8g2Fonts.print(line[i++]);
    }

    y += lineHeight + lineSpacing;
  }
}



String incomingText;

void handleTextBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

  if (index == 0) {
    Serial.printf("\nText Body: %u bytes\n", total); // FIRST CALL
    incomingText = "";
  }
  incomingText += String((const char*)data).substring(0, len);

  if (index + len == total) {
    Serial.printf("\nText transfer complete (%u bytes)\n", total); // END CALL, WE ARE DONE

    renderMarkedUpText(incomingText);
    updateDisplay = TRUE; // signal main loop() to update the display, here it would invoke the watchdog
  }
}

void handleTextResponse(AsyncWebServerRequest * request) {
  Serial.println("End of Text request.");
  request->send(200);
}

/*********************************************************************************

   QR management

 *********************************************************************************/

void handleQRBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

  if (index == 0) {
    Serial.printf("\nQR Text Body: %u bytes\n", total); // FIRST CALL
    incomingText = "";
  }
  incomingText += String((const char*)data).substring(0, len);

  if (index + len == total) {
    Serial.printf("\nQR text transfer complete (%u bytes)\n", total); // END CALL, WE ARE DONE

    if (!request->hasParam("version")
        or !request->hasParam("ecc")
        or !request->hasParam("mode")
        or !request->hasParam("pixelsize"))
    {
      request->send(400, "text/plain", "Missing QR upload parameter");
      return;
    }

    QRCode qrcode;
    uint8_t qrversion = request->getParam("version")->value().toInt();
    uint8_t ecc;
    switch (request->getParam("ecc")->value()[0]) {
      case 'L': ecc = ECC_LOW; break;
      case 'M': ecc = ECC_MEDIUM; break;
      case 'Q': ecc = ECC_QUARTILE; break;
      case 'H': ecc = ECC_HIGH; break;
      default:  ecc = ECC_LOW;
    }
    const uint8_t qrmodulesize = request->getParam("pixelsize")->value().toInt(); // pixels per module
    Serial.printf("\nQR version=%u ecc=%u modulesize=%u\n", qrversion, ecc, qrmodulesize);
    // int8_t qrcode_initText(QRCode *qrcode, uint8_t *modules, uint8_t version, uint8_t ecc, const char *data);
    uint8_t qrcodeData[qrcode_getBufferSize(qrversion)];
    qrcode_initText(&qrcode, qrcodeData, qrversion, ecc, incomingText.c_str());
    display.fillScreen(GxEPD_WHITE);
    for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
        if (qrcode_getModule(&qrcode, x, y)) {
          uint8_t gx = display.width() / 2 - qrcode.size * qrmodulesize / 2 + x * qrmodulesize;
          uint8_t gy = display.height() / 2 - qrcode.size * qrmodulesize / 2 + y * qrmodulesize;
          display.fillRect(gx, gy, qrmodulesize, qrmodulesize, GxEPD_BLACK);
        }
      }
    }
    updateDisplay = TRUE; // signal main loop() to update the display, here it would invoke the watchdog
  }
}

void handleQRResponse(AsyncWebServerRequest * request) {
  Serial.println("End of QR upload request.");
  request->send(200);
}

/*********************************************************************************

   Setup()

 *********************************************************************************/

void setup() {

  Serial.begin(115200);

  // SPIFFS

  if (!SPIFFS.begin(true)) {  // mounting without auto-format
    Serial.println("SPIFFS Mount Failed!");
    return;
  } else {
    Serial.println("SPIFFS Mounted Successfully!");
  }

  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  Serial.printf("Total SPIFFS Size: %d bytes\n", totalBytes);
  Serial.printf("Used SPIFFS Space: %d bytes\n", usedBytes);
  Serial.printf("Free SPIFFS Space: %d bytes\n", totalBytes - usedBytes);

  // Web Server

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    while (true) yield();
  }
  Serial.println("Connected to WiFi");
  Serial.print("Open URL http://"); Serial.println(WiFi.localIP());

  server.on("/",               HTTP_GET,    handleRootPage);
  server.on("/saveUserFile",   HTTP_POST,   handleSaveFileResponse, handleSaveFile);
  server.on("/uploadCanvas",   HTTP_POST,   handleCanvasResponse, nullptr, handleCanvasBody);
  server.on("/saveCanvas",     HTTP_POST,   handleSaveCanvasResponse, nullptr, handleSaveCanvasBody);
  server.on("/uploadText",     HTTP_POST,   handleTextResponse, nullptr, handleTextBody);
  server.on("/uploadQR",       HTTP_POST,   handleQRResponse, nullptr, handleQRBody);
  server.on("/files",          HTTP_GET,    handleFileListResponse);
  server.on("/free-flash",     HTTP_GET,    handleFreeFlash);
  server.on("/get-image",      HTTP_GET,    handleDisplaySelectedFile);
  server.on("/delete",         HTTP_DELETE, handleDeleteSelectedFile);
  server.on("/uploadselected", HTTP_PUT,    handleUploadSelectedFile);
  server.on("/downloadselected", HTTP_GET,  handleDownloadSelectedFile);

  server.begin();

  // e-paper

  display.init(115200, true, 2, false);
  u8g2Fonts.begin(display); // connect u8g2 procedures to Adafruit GFX
  delay(100);
  display.setFullWindow(); // full window mode is the initial mode, set it anyway
  display.setRotation(display_rotation);
  display.fillScreen(GxEPD_WHITE);

}



void loop() {
  if (updateDisplay) {
    display.display(false); // full update
    updateDisplay = FALSE;
  }
}
