#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include "dome_image.h"
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
const uint32_t PAUSE = 1000;
uint32_t timer;
uint8_t draw_state;
const uint8_t NUM_DRAW_STATES = 5;
const int BUTTON = 5;

void setup(void) {
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  Serial.begin(115200);

  pinMode(BUTTON, INPUT_PULLUP);

  draw_state = 0;
}

void loop() {

//  delay(500);
  //check for whether the button is pressed
  int unpushed = digitalRead(BUTTON);
  
  if (unpushed){
      //executed if pin is HIGH (voltage of 3.3V)
     //code here for printing "unpushed" 
 
     //update draw states based on whether the button is pushed
     if (draw_state == 1) {
      draw_state = 2;
     } else if (draw_state == 3) {
      draw_state = 0;
     }
     
  }else{ 
      //executed if pin is LOW (voltage of 0V)
     //code here to print "pushed"
     
     //update draw states based on whether the button is pushed
     if (draw_state == 0) {
      draw_state = 1;
     } else if (draw_state == 2) {
      draw_state = 3;
     }

  }

  //call function to draw stuff
  draw_stuff();
  
}


/*--------------------
 * draw_stuff Function:
 * 
 * Arguments:
 *    None
 * Return Values:
 *    None
 * Five-state State machine that cycles through various drawings on TFT screen.
 * Uses global variables draw_state (to represent state), and timer, to keep track of transition times
 * Only draws image on change of state!
 * State 0: Simple message!
 * State 1: Draw a one-bit monochrome image of MIT Dome and a title in MIT Colors
 * State 2: Draw Black background with a filled and empty circle of various sizes
 * State 3: Draw yellow background with two rectangles
 * State 4: Draw a bunch of lines green lines on a white backdrop using a for loop
 */

void draw_stuff(){
  if (millis()-timer > PAUSE){ //has PAUSE milliseconds elapsed since last change?
    timer = millis(); //store time of switch for future comparison
    
    if (draw_state==0 || draw_state==1){
      tft.fillScreen(TFT_BLACK); 
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("The first message",0,0,1);
      
    }else if(draw_state==2 || draw_state==3){
      tft.setTextColor(TFT_RED, TFT_LIGHTGREY);
      tft.fillScreen(TFT_LIGHTGREY);
      tft.drawString("THE SECOND MESSAGE", 40, 0, 2); //viewable on Screen   
    }
  }
}
