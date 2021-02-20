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

uint8_t display_hours;
uint8_t display_minutes;
uint8_t display_seconds;
uint8_t minute_passed;

int x;
int y;
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
  
  delay(200);
  
  // check for whether the button is pressed
  int unpushed = digitalRead(BUTTON);
  if (unpushed){
      //executed if pin is HIGH (voltage of 3.3V) 
     //update draw states based on whether the button is pushed
     if (draw_state == 1) {
      draw_state = 2;
     } else if (draw_state == 3) {
      draw_state = 0;
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
  
}

void update_time(){
  //timeout check to update the current time
  if ((millis() - time_since_update) > 60000) {
    Serial.println("** GETTING NEW TIME UPDATE **");
     get_universal_time();
  }

  //set the display time variables
  display_seconds = seconds + ((millis() - time_since_update)/1000) - minute_passed;
  if (display_seconds > 59) {
    display_seconds = 0;
    minute_passed = 60;
    display_minutes++;
  }
  if (display_minutes > 59) {
    display_minutes = 0;
    display_hours++;
  }
  if (display_hours > 12) {
    display_hours = 1;
  }
}



/*----------------------------------
   FUNCTIONS TO DRAW WATCH FACE
*/


void draw_watch() {

    // DIGITAL
    if (draw_state==0 || draw_state==1){
      tft.fillScreen(TFT_BLUE);
      tft.drawString("DIGITAL",0,0,1); 

      sprintf(digital_display, "%d : %d : %d", display_hours, display_minutes, display_seconds);
      tft.drawString(digital_display, 0, 10, 1);

    //ANALOG
    }else if(draw_state==2 || draw_state==3){
      tft.fillScreen(TFT_BLUE);
      tft.drawString("ANALOG", 0, 0, 1); 

      draw_analog();
    }
}

void draw_analog() {
  
  // draw seconds
  y = (second_hand * cos(pi-(2*pi)/60 * display_seconds)) + center;
  x = (second_hand * sin(pi-(2*pi)/60 * display_seconds)) + center;
  tft.drawLine(center, center, x, y, TFT_WHITE);

  // draw minutes
  y = (minute_hand * cos(pi-(2*pi)/60 * display_minutes)) + center;
  x = (minute_hand * sin(pi-(2*pi)/60 * display_minutes)) + center;
  tft.drawLine(center, center, x, y, TFT_WHITE);

  // draw hours
  y = (hour_hand * cos(pi-(2*pi)/12 * display_hours)) + center;
  x = (hour_hand * sin(pi-(2*pi)/12 * display_hours)) + center;
  tft.drawLine(center, center, x, y, TFT_WHITE);

  // draw clock circle
  tft.drawCircle(center, center, 30, TFT_WHITE);
}
