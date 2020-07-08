/* --------------------------------------------------
*  Name: YongQuan Zhang, Haoyang Cheng
*  ID: 1515873, 1583512
*  CMPUT 275, Winter 2020
*  Assignment #1 part #1: Restaurant Finder
*---------------------------------------------------*/

#define SD_CS 10
#define JOY_VERT  A9 // should connect A9 to pin VRx
#define JOY_HORIZ A8 // should connect A8 to pin VRy
#define JOY_SEL   53

#include <Arduino.h>

// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>

// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <MCUFRIEND_kbv.h>

// LCD and SD card will communicate using the Serial Peripheral Interface (SPI)
// e.g., SPI is used to display images stored on the SD card
#include <SPI.h>

// needed for reading/writing to SD card
#include <SD.h>

#include "lcd_image.h"
#include <TouchScreen.h>


MCUFRIEND_kbv tft;

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320
#define YEG_SIZE 2048

lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };

#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define CURSOR_SIZE 9
// touch screen pins, obtained from the documentaion
#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin
#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 60)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920
#define MINPRESSURE   10
#define MAXPRESSURE 1000
#define MAP_WIDTH  2048
#define MAP_HEIGHT  2048
#define LAT_NORTH  5361858l
#define LAT_SOUTH  5340953l
#define LON_WEST  -11368652l
#define LON_EAST  -11333496l
#define NUM_LINES 21
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// different than SD
Sd2Card card;
uint32_t blockNum=0;
uint32_t oldblock=0;
int yegMiddleX, yegMiddleY;
// the cursor position on the display
int cursorX, cursorY;
// Record the number of hilighed line.
int highlightedString = 0;

// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);

void setup() {
  init();

  Serial.begin(9600);

	pinMode(JOY_SEL, INPUT_PULLUP);

	//    tft.reset();             // hardware reset
  uint16_t ID = tft.readID();    // read ID from display
  Serial.print("ID = 0x");
  Serial.println(ID, HEX);
  if (ID == 0xD3D3) ID = 0x9481; // write-only shield
  
  // must come before SD.begin() ...
  tft.begin(ID);                 // LCD gets ready to work

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed! Is it inserted properly?");
		while (true) {}
	}
	Serial.println("OK!");

	tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  // draws the centre of the Edmonton map,
  // leaving the rightmost 60 columns black
	yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
	yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
	lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

  // initial cursor position is the middle of the screen
  cursorX = (DISPLAY_WIDTH - 60)/2;
  cursorY = DISPLAY_HEIGHT/2;

  redrawCursor(TFT_RED);
  // SD card initialization for raw reads
  Serial.print("Initializing SPI communication for raw reads...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println("failed! Is the card inserted properly?");
    while (true) {}
  }
  else {
    Serial.println("OK!");
  }
}
void redrawCursor(uint16_t colour) {
  tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, colour);
}
void processJoystick(){
  /* Move the cursor without leaving a trace behind it.
     Also will deal with moving to the next patch of map.
  Args: 
    None.
  Returns:
    None.
  */
  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);
  int buttonVal = digitalRead(JOY_SEL);
  int oldx = cursorX;
  int oldy = cursorY;
  if ((JOY_CENTER - yVal) > JOY_DEADZONE ){
    cursorY -= abs(JOY_CENTER - yVal)/100;
  }
  else if ((yVal-JOY_CENTER) > JOY_DEADZONE){
    cursorY += abs(JOY_CENTER - yVal)/100;
  }

  if ((xVal-JOY_CENTER) > JOY_DEADZONE){
    cursorX -= abs(JOY_CENTER - xVal)/100;
  }
  else if ((JOY_CENTER - xVal) > JOY_DEADZONE){
    cursorX += abs(JOY_CENTER - xVal)/100;
  }
  int old_yeg_x = yegMiddleX;
  int old_yeg_y = yegMiddleY;

  // Line 164 to 204 is for moveing to the next patch of map.
  if(cursorX - CURSOR_SIZE/2 < 0){
    yegMiddleX = yegMiddleX - 420;
    if(yegMiddleX < 0){yegMiddleX = 0;}
    if(yegMiddleX != old_yeg_x){
      lcd_image_draw(&yegImage, &tft, 
      yegMiddleX, yegMiddleY,
      0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
      cursorX = 420-CURSOR_SIZE/2;
    }
  }
  if(cursorX + CURSOR_SIZE/2 > 420){
    yegMiddleX = yegMiddleX + 420;
    if(yegMiddleX>MAP_WIDTH-420){yegMiddleX = MAP_WIDTH-420;}
    if(yegMiddleX != old_yeg_x){
      lcd_image_draw(&yegImage, &tft, 
      yegMiddleX, yegMiddleY,
      0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
      cursorX = 0;
    }
  }
 if (cursorY - CURSOR_SIZE/2 < 0){
    yegMiddleY = yegMiddleY-DISPLAY_HEIGHT;
    if(yegMiddleY < 0){yegMiddleY = 0;}
    if(yegMiddleY != old_yeg_y){
      lcd_image_draw(&yegImage, &tft, 
      yegMiddleX, yegMiddleY,
      0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
      cursorY = 320-CURSOR_SIZE/2;
    }
  }
 if (cursorY + CURSOR_SIZE/2 > 320){
    yegMiddleY=yegMiddleY+DISPLAY_HEIGHT;
    if(yegMiddleY > 2048-320){yegMiddleY = MAP_HEIGHT-320;}
    if(yegMiddleY != old_yeg_y){
      lcd_image_draw(&yegImage, &tft, 
      yegMiddleX, yegMiddleY,
      0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
      cursorY = 0;
    }
  }
  // 320 is the display height.
  if (constrain(cursorY - CURSOR_SIZE/2,0,320) == 0){
     cursorY = CURSOR_SIZE/2;
  }
  // Since we always start draw the cursor at the
  // left top corner so for the bottome and right edge, the
  // distance to the edge will be 9/2 + 1 = 5, for top and left
  // the distance will be 9/2 = 4. 
  else if (constrain(cursorY + CURSOR_SIZE/2,0,320) == 320){
    cursorY = 320-(CURSOR_SIZE/2+1);
  }
  // 420 is the display width(480) minus the black space(60).
  if (constrain(cursorX - CURSOR_SIZE/2,0,420) == 0){
    cursorX = CURSOR_SIZE/2;
  }
  else if (constrain(cursorX + CURSOR_SIZE/2,0,420) == 420){
    cursorX = 420-(CURSOR_SIZE/2+1);
  }
  int newx = cursorX;
  int newy = cursorY;
  if(oldx != newx || oldy != newy){
      lcd_image_draw(&yegImage, &tft,
        yegMiddleX + oldx - CURSOR_SIZE/2,
        yegMiddleY + oldy - CURSOR_SIZE/2,
        oldx - CURSOR_SIZE/2,
        oldy - CURSOR_SIZE/2, 
        CURSOR_SIZE, CURSOR_SIZE);
  }
  if(oldx != newx || oldy != newy){
    redrawCursor(TFT_RED);
  }
  delay(20);  
}

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};
// Make this as a global variable, so we could access to the same
// 8 bytes on memory.
restaurant restBlock[8];
struct RestDist{
  uint16_t index;
  uint16_t dist;
};
RestDist rest_dist[1066];

void getRestaurantFast(int restIndex, restaurant* restPtr) {

  blockNum = REST_START_BLOCK + restIndex/8;
    if(blockNum != oldblock){
      while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
        Serial.println("Read block failed, trying again.");
      }
    }
  *restPtr = restBlock[restIndex % 8];
  oldblock = blockNum;

}

int processTOUCHSCREEN(){
  /* Determine the coordinates of touch. Then determine whether
    we are touching the buttons or somewhere else.
  Args: 
    None.
  Returns:
    0: if we don't have enough pressure or we touch the screen too hard.
    1: if we touch screen.
  */
  TSPoint touch = ts.getPoint();
  pinMode(YP, OUTPUT); 
  pinMode(XM, OUTPUT); 

  if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {
    return 0;
  }
  int16_t screen_x = map(touch.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH-1, 0);
  int16_t screen_y = map(touch.x, TS_MINY, TS_MAXY, DISPLAY_HEIGHT-1, 0);
  if (screen_x < 420){
    return 1;
  }
  delay(200);
}

int16_t  lon_to_x(int32_t  lon) {
  return  map(lon , LON_WEST , LON_EAST , 0, MAP_WIDTH);
}


int16_t  lat_to_y(int32_t  lat) {
  return  map(lat , LAT_NORTH , LAT_SOUTH , 0, MAP_HEIGHT);
}

uint32_t Manhattan(restaurant *coord, int X, int Y){
  uint32_t distance;
  int32_t restaurant_x = lon_to_x(coord->lon);
  int32_t restaurant_y = lat_to_y(coord->lat);
  distance = abs(restaurant_x - X)+abs(restaurant_y - Y);
  return distance;
}

void swap(RestDist *rest_dist1, RestDist *rest_dist2){
  uint16_t temp_index;
  uint16_t temp_dist;
  temp_index = rest_dist1->index;
  temp_dist = rest_dist1->dist;
  rest_dist1->index = rest_dist2->index;
  rest_dist1->dist = rest_dist2->dist;
  rest_dist2->index = temp_index;
  rest_dist2->dist = temp_dist;
}

void isort(RestDist *rest_dist,int n){
  // sort an array, build based on the code in assignment description.
  int i = 1;
  while(i<n){
    int j = i;
    while(j>0 && 
      rest_dist[j-1].dist > rest_dist[j].dist){
      swap(&rest_dist[j-1],&rest_dist[j]);
      j = j-1;
    }
    i = i+1;
  }
}
// This function is for highlighting the selected line.
void HIGHLIGHT(int res_index, char res_name[]) {
  tft.setCursor(0, 15*res_index);
  if (res_index == highlightedString) {
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }
  else{
    // Change the previous highlighted string to normal.
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  tft.println(res_name);
}

void joyScrollList(){
  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);
  restaurant names;
  if ((yVal-JOY_CENTER) > JOY_DEADZONE &&
     (highlightedString < NUM_LINES-1)) {
    getRestaurantFast(rest_dist[highlightedString].index,&names);
    highlightedString += 1;
    // Change the previous highlighted line to normal.
    HIGHLIGHT(highlightedString-1,names.name);
    getRestaurantFast(rest_dist[highlightedString].index,&names);
    // Highlight selected line.
    HIGHLIGHT(highlightedString,names.name);
    delay(20);
  }
  else if ((yVal < JOY_CENTER - 100) && (highlightedString > 0)) {
    getRestaurantFast(rest_dist[highlightedString].index,&names);
    highlightedString -= 1;
    HIGHLIGHT(highlightedString+1,names.name);
    getRestaurantFast(rest_dist[highlightedString].index,&names);
    HIGHLIGHT(highlightedString,names.name);
    delay(20);
  }
}

void Changemode(bool &mode){
  if(mode == 0){
    mode = 1;
  }
  else if(mode == 1){
    mode = 0;
  }
  delay(200);
 }

void draw_restaurant_dot(){
  restaurant location;
  if(processTOUCHSCREEN() == 1){
    for(int i = 0; i < 1066; i++){
      // Fetch 1066 restaurant from the SD card and use their
      // longitude and latitude to draw dots on the map.
      getRestaurantFast(i,&location);
      int32_t bigmap_x = lon_to_x(location.lon);
      int32_t bigmap_y = lat_to_y(location.lat);
      int32_t actual_x = bigmap_x-yegMiddleX;
      int32_t actual_y = bigmap_y-yegMiddleY;
      if((actual_x >= 0 && actual_x <= 420) &&
        (actual_y >=0 && actual_y <= 320)){
        tft.fillCircle(actual_x,actual_y,3,TFT_BLACK);
      }
    }
  }
}

bool pressJoy(){
  int buttonVal = digitalRead(JOY_SEL);
  if(buttonVal == LOW){
    return 1;
  }
  else{
    return 0;
  }
  delay(200);
}

void locate_selected_restaurant(){
  restaurant selected;
  getRestaurantFast(rest_dist[highlightedString].index,&selected);
  int32_t bigmap_x = lon_to_x(selected.lon);
  int32_t bigmap_y = lat_to_y(selected.lat);
  yegMiddleX = bigmap_x - 420/2;
  yegMiddleY = bigmap_y - 320/2;
  // To avoid the case the yegMiddleX or Y is out off map.
  if(yegMiddleX < 0){yegMiddleX = 0;}
  if(yegMiddleX > 2048-420){yegMiddleX = 2048-420;}
  if(yegMiddleY < 0){yegMiddleY = 0;}
  if(yegMiddleY > 2048-320){yegMiddleY = 2048-320;}
  cursorX = bigmap_x - yegMiddleX;
  cursorY = bigmap_y - yegMiddleY;
  // From line 419 to 431 is for the restaurant that is not in the
  // map range.
  if (constrain(cursorY - CURSOR_SIZE/2,0,320) == 0){
     cursorY = CURSOR_SIZE/2;
  }
  else if (constrain(cursorY + CURSOR_SIZE/2,0,320) == 320){
    cursorY = 320-(CURSOR_SIZE/2+1);
  }
  if (constrain(cursorX - CURSOR_SIZE/2,0,420) == 0){
    cursorX = CURSOR_SIZE/2;
  }
  else if (constrain(cursorX + CURSOR_SIZE/2,0,420) == 420){
    cursorX = 420-(CURSOR_SIZE/2+1);
  }
}

int main() {
	setup();
  restaurant rest;
  restaurant rest2;
  bool mode = 0;
  bool printListorNot = false;
  while (true) {
    if(mode == 0){
      draw_restaurant_dot();
      processJoystick();
      // reset the highlighted string index to 0 so next time you
      // press it, it will be still thefirst line been highlighted.
      highlightedString = 0;
    }
    else if(mode == 1){
      if(printListorNot){
        for(int j = 0;j < 1066; j++){
          getRestaurantFast(j, &rest2);
          uint32_t ans = Manhattan(&rest2, yegMiddleX+cursorX, yegMiddleY+cursorY);
          rest_dist[j].index = j;
          rest_dist[j].dist = ans;
        }
        isort(rest_dist,1066);
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextWrap(false);
        for(int k=0;k<21;k++){
          getRestaurantFast(rest_dist[k].index,&rest);
          HIGHLIGHT(k,rest.name);
        }
        // This boolean value will maintain false until when you
        // press joysticker again.
        printListorNot = false;
      }
      joyScrollList();
      if(pressJoy() == 1){
        locate_selected_restaurant();
        tft.fillScreen(TFT_BLACK);
        lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                      0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
        redrawCursor(TFT_RED);
        mode = 0;
      }
    }
    int press = pressJoy();
    if(press == 1){
      Changemode(mode);
      printListorNot = true;
    }
  }
  Serial.end();
	return 0;
}
