#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
const uint32_t PAUSE = 1000;
uint8_t draw_state;
const int BUTTON = 5;
const float pi = 3.14;

// TIMER VARS
uint32_t timer;
char universal_time[1000];
char digital_display[1000];
uint32_t time_since_update;

uint8_t hours;
uint8_t minutes;
uint8_t seconds;

// DISPLAY VARS
uint8_t display_hours;
uint8_t display_minutes;
uint8_t display_seconds;
uint8_t minute_passed;
uint8_t new_seconds;

bool update_title;
bool update_hours;
bool update_minutes;
bool update_seconds;

// ANALOG VARS
// all start at 60 because that is the center of the clock
int sx = 60;
int sy = 60;
int mx = 60;
int my = 60;
int hx = 60;
int hy = 60;
int second_hand = 24;
int minute_hand = 24;
int hour_hand = 16;
int center = 60;

// NETWORK VARS
char network[] = "116 berkshire floor 3";
char password[] = "caterpillar420";
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 5000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

void setup() {

  // initialize the watch display basics
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Getting time...",0,0,1); 
  Serial.begin(115200);

  pinMode(BUTTON, INPUT_PULLUP);

  draw_state = 0;

  // initial connection to wifi
  connect_to_wifi();

  // initial setting of universal time
  get_universal_time();

}

void loop() {
  
  delay(100);
  
  // check for whether the button is pressed
  int unpushed = digitalRead(BUTTON);
  if (unpushed){
      //executed if pin is HIGH (voltage of 3.3V) 
     //update draw states based on whether the button is pushed
     if (draw_state == 1) {
      draw_state = 2;

      //set all the time states to be updated
      update_title = true;
      update_seconds = true;
      update_minutes = true;
      update_hours = true;
     } else if (draw_state == 3) {
      draw_state = 0;
      
      //set all the time states to be updated
      update_title = true;
      update_seconds = true;
      update_minutes = true;
      update_hours = true;
     }
     
  }else{ 
      //executed if pin is LOW (voltage of 0V)
     //update draw states based on whether the button is pushed
     if (draw_state == 0) {
      draw_state = 1;
     } else if (draw_state == 2) {
      draw_state = 3;
     }
  }

  // update the display/universal time
  update_time();
  
  //call function to draw stuff
  draw_watch();

}

/*----------------------------------
   GET REQUEST FUNCTIONS FROM LABS
   
*/

void connect_to_wifi() {
  //do initial GET request and connect to server
  WiFi.begin(network, password);

  uint8_t count = 0;
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 6) { //can change this to more attempts
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);  //acceptable since it is in the setup function.
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
}


uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line! (end of response header)
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);  //empty in prep to store body
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

/*----------------------------------
   FUNCTIONS TO UPDATE TIME
   
*/

void get_universal_time() {

  sprintf(request_buffer, "GET /esp32test/currenttime HTTP/1.1\r\n");
  strcat(request_buffer, "Host: iesc-s3.mit.edu\r\n"); //add more to the end
  strcat(request_buffer, "\r\n"); //add blank line!

  do_http_GET("iesc-s3.mit.edu", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  Serial.println(response_buffer); //print to serial monitor

  //set universal time vars  
  memcpy(universal_time, &response_buffer[11], 8); //to format hh:mm:ss
  hours = atoi(universal_time);
  
  memmove(universal_time, universal_time+3, strlen (universal_time)); //move the universal time pointer to get ints
  minutes = atoi(universal_time);
  
  memmove(universal_time, universal_time+3, strlen (universal_time)); //move the universal time pointer to get ints
  seconds = atoi(universal_time);

  //initialize the display vars
  display_seconds = seconds;
  display_minutes = minutes;
  display_hours = hours;
  minute_passed = 0;
  
  //start timer to update
  time_since_update = millis();

  // set watch display update vars to true
  update_title = true;
  update_hours = true;
  update_minutes = true;
  update_seconds = true;
  
}

void update_time(){
  //timeout check to update the current time
  if ((millis() - time_since_update) > 60000) {
    Serial.println("** GETTING NEW TIME UPDATE **");
     get_universal_time();
  }

  //set the display time variables
  new_seconds = seconds + ((millis() - time_since_update)/1000) - minute_passed;
  update_seconds = (new_seconds > display_seconds || update_title);
  display_seconds = new_seconds;
  
  if (display_seconds > 59) {
    display_seconds = 0;
    minute_passed = 60;
    display_minutes++;
    update_minutes = true;
  }
  if (display_minutes > 59) {
    display_minutes = 0;
    display_hours++;
    update_hours = true;
  }
  if (display_hours > 24) {
    display_hours = 1;
    update_hours = true;
  }
}



/*----------------------------------
   FUNCTIONS TO DRAW WATCH FACE
*/


void draw_watch() {

    // DIGITAL
    if (draw_state==0 || draw_state==1){
      draw_digital();
      
    //ANALOG
    }else if(draw_state==2 || draw_state==3){ 
      draw_analog();
    }
}

void draw_digital() {

  if (update_title){
    tft.fillScreen(TFT_BLUE);
    tft.drawString("DIGITAL",0,0,1);

    //draw colons
    tft.fillCircle(15, 12, 1, TFT_WHITE);
    tft.fillCircle(15, 14, 1, TFT_WHITE);
    tft.fillCircle(35, 12, 1, TFT_WHITE);
    tft.fillCircle(35, 14, 1, TFT_WHITE);
    
    update_title = false;
  }

  if (update_hours) {
    tft.fillRoundRect(0, 10, 10, 10, 0, TFT_BLUE); 
    tft.drawNumber(display_hours, 0, 10, 1);
    update_hours = false;
  }
  if (update_minutes) {
    tft.fillRoundRect(20, 10, 10, 10, 0, TFT_BLUE); 
    tft.drawNumber(display_minutes, 20, 10, 1);
    update_minutes = false;
  }
  if (update_seconds) {
    tft.fillRoundRect(40, 10, 15, 15, 0, TFT_BLUE); 
    tft.drawNumber(display_seconds, 40, 10, 1);
    update_seconds = false;
  }
  

}

void draw_analog() {

  // title screen
  if (update_title){
    tft.fillScreen(TFT_BLUE);
    tft.drawString("ANALOG", 0, 0, 1);

    // draw clock circle
    tft.drawCircle(center, center, 30, TFT_WHITE);
  }
  
  if (update_seconds) {
    // draw seconds
    tft.drawLine(center, center, sx, sy, TFT_BLUE); // use past var
    sy = (second_hand * cos(pi-(2*pi)/60 * display_seconds)) + center;
    sx = (second_hand * sin(pi-(2*pi)/60 * display_seconds)) + center;
    tft.drawLine(center, center, sx, sy, TFT_WHITE);

    update_seconds = false;
    
    // to account for the seconds hand erasing the other clock hands
    // (since everything is drawn and redrawn)
    // a buffer is added because of the pixelated lines
    update_minutes = ((display_seconds - 10 < display_minutes && display_minutes < display_seconds) || update_title);
    update_hours = ((display_seconds - 10 < display_hours && display_hours < display_seconds) || update_title);
  }
  if (update_minutes) {
    // draw minutes
    tft.drawLine(center, center, mx, my, TFT_BLUE); // use past var
    my = (minute_hand * cos(pi-(2*pi)/60 * display_minutes)) + center;
    mx = (minute_hand * sin(pi-(2*pi)/60 * display_minutes)) + center;
    tft.drawLine(center, center, mx, my, TFT_WHITE);

    update_minutes = false;
  }
  if (update_hours) {
    // draw hours
    tft.drawLine(center, center, hx, hy, TFT_BLUE); // use past var
    hy = (hour_hand * cos(pi-(2*pi)/12 * display_hours)) + center;
    hx = (hour_hand * sin(pi-(2*pi)/12 * display_hours)) + center;
    tft.drawLine(center, center, hx, hy, TFT_WHITE);

    update_hours = false;
  }

  update_title = false;
  
}
