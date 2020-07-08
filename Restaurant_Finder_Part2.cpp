/* --------------------------------------------------
*  Name: YongQuan Zhang, Haoyang Cheng
*  ID: 1515873, 1583512
*  CMPUT 275, Winter 2020
*  Assignment #1 part #2: Restaurant Finder
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
// Keep tracking the rating of the selected restaurant.
int rate = 1;
// This variable will record the total number of restaurant.
uint32_t totalrestnum=0;
// This variable will keep tracking the current 
// page number we are on.
int count = 0;
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
  tft.drawRect(DISPLAY_WIDTH-60,0,60,DISPLAY_HEIGHT/2,TFT_RED);
  tft.drawRect(DISPLAY_WIDTH-60,DISPLAY_HEIGHT/2,60,DISPLAY_HEIGHT/2,TFT_GREEN);
  tft.fillRect(DISPLAY_WIDTH-60+1,1,60-2,DISPLAY_HEIGHT/2-2,TFT_WHITE);
  tft.fillRect(DISPLAY_WIDTH-60+1,DISPLAY_HEIGHT/2+1,60-2,DISPLAY_HEIGHT/2-2,TFT_WHITE);
  tft.drawChar(445,60,49,TFT_BLACK,TFT_WHITE,2);
  tft.drawChar(445,190,'Q',TFT_BLACK,TFT_WHITE,2);
  tft.drawChar(445,210,'S',TFT_BLACK,TFT_WHITE,2);
  tft.drawChar(445,230,'O',TFT_BLACK,TFT_WHITE,2);
  tft.drawChar(445,250,'R',TFT_BLACK,TFT_WHITE,2);
  tft.drawChar(445,270,'T',TFT_BLACK,TFT_WHITE,2);
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

  // Line 178 to 217 is for moveing to the next patch of map.
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
  else if(screen_x > 420 && screen_y < DISPLAY_HEIGHT/2){
    return 2;
  }
  else if(screen_x > 420 && screen_y > DISPLAY_HEIGHT/2){
    return 3;
  }
  delay(200);
}
void update_touchcreen(int &sort_num){
  // This function will update the tft display based on 
  // different sorting number.
  if(processTOUCHSCREEN() == 2){
    rate++;
    if(rate > 5){
      rate = 1;
    }
    tft.drawChar(445,60,rate+48,TFT_BLACK,TFT_WHITE,2);
    delay(200);
  }
  if(processTOUCHSCREEN() == 3){
    sort_num++;
    if(sort_num > 2){
      sort_num = 0;
    }
    if(sort_num == 0){
      tft.drawChar(445,190,'Q',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,210,'S',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,230,'O',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,250,'R',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,270,'T',TFT_BLACK,TFT_WHITE,2);
    }
    else if(sort_num == 1){
      tft.drawChar(445,190,'I',TFT_BLACK,TFT_WHITE,2);
    }
    else if(sort_num == 2){
      tft.drawChar(445,190,'B',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,210,'O',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,230,'T',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,250,'H',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,270,' ',TFT_BLACK,TFT_WHITE,2);
    }
  delay(200);
  }
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
  distance = abs(restaurant_x - X)+abs(restaurant_y-Y);
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
// Insertion sorting, from the code in assignment description.
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
int pivot(RestDist *a, int n, int pi){
  // From part2 description, a function that could adjust
  // high and low indexes.
  swap(&a[pi],&a[n-1]);
  int low = 0;
  int high = n-2;
  while(low <= high){
    if(a[low].dist <= a[n-1].dist){
      low ++;
    }
    else if(a[high].dist>a[n-1].dist){
      high--;
    }
    else{
      swap(&a[low],&a[high]);
    }
  }
  swap(&a[low],&a[n-1]);
  return low;
}
void qsort(RestDist *array,int n){
  // A recursion function from part2 description and
  // lecture slides.
  if(n <= 1){
    return;
  }
  int pi = n/2;
  int new_pi = pivot(array, n, pi);
  qsort(array, new_pi);
  qsort(array+new_pi, n-new_pi);
}
// This function is for highlighting the selected line and 
// unhighlighted the previous line.
void HIGHLIGHT(int res_index, char res_name[]) {
  tft.setCursor(0, 15*(res_index-count*21));
  if (res_index == highlightedString) {
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }
  else{
    // Change the previous highlighted string to normal.
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  tft.println(res_name);
}
void Nextlist(){
  // This function is for moving to the next list of restaurants.
  // It will make the list "scrollable". Also, this function could
  // deal with special cases such as when you move to the 1st
  // or last restaurant of the list.
  restaurant names;
  int numResOnLastPage;
  int totalpage = ceil(totalrestnum/21.0);
  // Move to next page.
  if(highlightedString >= (count+1)*21){
    count++;
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextWrap(false);
    // When we are at the last page.
    if(count == totalpage-1){
      numResOnLastPage = totalrestnum - (count)*21;
      for(int k=highlightedString;
        k<highlightedString+numResOnLastPage;k++){
        getRestaurantFast(rest_dist[k].index,&names);
        HIGHLIGHT(k,names.name);
      }
    }
    // When we are not at the last page.
    else{
      for(int k=highlightedString;k<highlightedString+21;k++){
        getRestaurantFast(rest_dist[k].index,&names);
        HIGHLIGHT(k,names.name);
      }
    }
  }
  // Move to the previous page.
  else if(highlightedString < count*21 && highlightedString >=0){
    count--;
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextWrap(false);
    for(int k=highlightedString-20;k<=highlightedString;k++){
      getRestaurantFast(rest_dist[k].index,&names);
      HIGHLIGHT(k,names.name);
    }
  }
  // Special case: when we are at 1st restaurant of 1st page.
  if(highlightedString < 0){
    numResOnLastPage = totalrestnum - (totalpage-1)*21;
    highlightedString = totalrestnum-1;
    count = totalpage-1;
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextWrap(false);
    for(int k=highlightedString-numResOnLastPage;
      k<=highlightedString;k++){
      getRestaurantFast(rest_dist[k].index,&names);
      HIGHLIGHT(k,names.name);
    }
  }
  // Special case: when we are at the last restaurant at last page.
  else if(highlightedString > totalrestnum-1){
    highlightedString = 0;
    count = 0;
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextWrap(false);
    for(int k=highlightedString;k<highlightedString + 21;k++){
      getRestaurantFast(rest_dist[k].index,&names);
      HIGHLIGHT(k,names.name);
    }
  }
}
void joyScrollList(){
  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);
  restaurant names;
  if ((yVal-JOY_CENTER) > JOY_DEADZONE) {
    getRestaurantFast(rest_dist[highlightedString].index,&names);
    highlightedString += 1;
    if(highlightedString < (count+1)*21 &&
     highlightedString < totalrestnum){
      HIGHLIGHT(highlightedString-1,names.name);
      // Change the previous highlighted line to normal.
      getRestaurantFast(rest_dist[highlightedString].index,&names);
      // Highlight selected line.
      HIGHLIGHT(highlightedString,names.name);
    }
  }
  else if (yVal < JOY_CENTER - 100) {
    getRestaurantFast(rest_dist[highlightedString].index,&names);
    highlightedString -= 1;
    if(highlightedString >= count*21){
      HIGHLIGHT(highlightedString+1,names.name);
      getRestaurantFast(rest_dist[highlightedString].index,&names);
      HIGHLIGHT(highlightedString,names.name);
    }
  }
  delay(100);
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
      int rate_res = max(floor((location.rating+1)/2.0),1);
      if(rate_res >= rate){
        int32_t bigmap_x = lon_to_x(location.lon);
        int32_t bigmap_y = lat_to_y(location.lat);
        int32_t actual_x = bigmap_x-yegMiddleX;
        int32_t actual_y = bigmap_y-yegMiddleY;
        // Make sure the restaurant we draw is inside the display
        // range.
        if((actual_x >= 0 && actual_x <= 420) &&
          (actual_y >=0 && actual_y <= 320)){
          tft.fillCircle(actual_x,actual_y,3,TFT_BLACK);
        }
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
  // From line 587 to 599 is for the restaurant that is not in the
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
void redraw_map_after_joy_press(int sort_num){
    tft.fillScreen(TFT_BLACK);
    lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                  0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
    redrawCursor(TFT_RED);
    tft.drawRect(DISPLAY_WIDTH-60,0,60,DISPLAY_HEIGHT/2,TFT_RED);
    tft.drawRect(DISPLAY_WIDTH-60,DISPLAY_HEIGHT/2,60,DISPLAY_HEIGHT/2,TFT_GREEN);
    tft.fillRect(DISPLAY_WIDTH-60+1,1,60-2,DISPLAY_HEIGHT/2-2,TFT_WHITE);
    tft.fillRect(DISPLAY_WIDTH-60+1,DISPLAY_HEIGHT/2+1,60-2,DISPLAY_HEIGHT/2-2,TFT_WHITE);
    tft.drawChar(445,60,rate+48,TFT_BLACK,TFT_WHITE,2);

    if(sort_num == 0){
      tft.drawChar(445,190,'Q',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,210,'S',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,230,'O',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,250,'R',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,270,'T',TFT_BLACK,TFT_WHITE,2);
    }
    else if(sort_num == 1){
      tft.drawChar(445,190,'I',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,210,'S',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,230,'O',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,250,'R',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,270,'T',TFT_BLACK,TFT_WHITE,2);      
    }
    else if(sort_num == 2){
      tft.drawChar(445,190,'B',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,210,'O',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,230,'T',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,250,'H',TFT_BLACK,TFT_WHITE,2);
      tft.drawChar(445,270,' ',TFT_BLACK,TFT_WHITE,2);
    }
    delay(200);
}


int main() {
	setup();
  restaurant rest;
  restaurant rest2;
  bool mode = 0;
  bool printListorNot = false;
  int sort_num = 0;
  uint32_t time_start;
  uint32_t time_end;
  uint32_t time_interval;
  while (true) {
    int aboverating_index = 0;
    if(mode == 0){
      draw_restaurant_dot();
      processJoystick();
      update_touchcreen(sort_num);
      // reset the highlighted string index to 0 so next time you
      // press it.
      count = 0;
      highlightedString = count*21;
      totalrestnum = 0;
    }
    else if(mode == 1){
      if(printListorNot){
        // To avoid a weird error, details are in README.
        count = 0;
        highlightedString = count*21;
        totalrestnum = 0;
        for(int j = 0;j < 1066; j++){
          getRestaurantFast(j, &rest2);
          uint32_t ans = Manhattan(&rest2,
           yegMiddleX+cursorX, yegMiddleY+cursorY);
          if(max(floor((rest2.rating+1)/2.0),1) >= rate){
            totalrestnum++;
            rest_dist[aboverating_index].index = j;
            rest_dist[aboverating_index].dist = ans;
            aboverating_index++;
          }
        }
        // sort_num = 0 is for quick sorting.
        if(sort_num == 0){
          time_start = millis();
          qsort(rest_dist,aboverating_index+1);
          time_end = millis();
          time_interval = time_end - time_start;
          Serial.print("Qsort ");
          Serial.print(totalrestnum);
          Serial.print(" restaurants: ");
          Serial.print(time_interval);
          Serial.println(" ms");
        }
        // sort_num = 1 is for insertion sorting.
        else if(sort_num == 1){
          time_start = millis();
          isort(rest_dist,aboverating_index+1);
          time_end = millis();
          time_interval = time_end - time_start;
          Serial.print("Isort ");
          Serial.print(totalrestnum);
          Serial.print(" restaurants: ");
          Serial.print(time_interval);
          Serial.println(" ms");
        }
        // sort_num = 2 is for both sorting.
        else if(sort_num == 2){
          time_start = millis();
          qsort(rest_dist,aboverating_index+1);
          time_end = millis();
          time_interval = time_end - time_start;
          Serial.print("Qsort ");
          Serial.print(totalrestnum);
          Serial.print(" restaurants: ");
          Serial.print(time_interval);
          Serial.println(" ms");
          aboverating_index = 0;
          for(int j = 0;j < 1066; j++){
            getRestaurantFast(j, &rest2);
            uint32_t ans = Manhattan(&rest2, 
              yegMiddleX+cursorX, yegMiddleY+cursorY);
            if(max(floor((rest2.rating+1)/2.0),1) >= rate){
              rest_dist[aboverating_index].index = j;
              rest_dist[aboverating_index].dist = ans;
              aboverating_index++;
            }
          }
          time_start = millis();
          isort(rest_dist,aboverating_index+1);
          time_end = millis();
          time_interval = time_end - time_start;
          Serial.print("Isort ");
          Serial.print(totalrestnum);
          Serial.print(" restaurants: ");
          Serial.print(time_interval);
          Serial.println(" ms");
        }
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
      Nextlist();
      if(pressJoy() == 1){
        locate_selected_restaurant();
        redraw_map_after_joy_press(sort_num);
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