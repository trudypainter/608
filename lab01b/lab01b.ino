#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.

/*#########################################
  Simple example script demonstrating:
  Connection with an Access Point (AP)
  Creation of client object
  Connection of client object to host
  Sending of GET request with several arguments
  verbose printing of HTTP response from host
  Simple parsing of HTTP response

  We'll use the open API found at:
  http://numbersapi.com
  Intended for 6.08 Spring 2021 Lab 01B
*/

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 5000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

uint32_t loop_controller; //used for timing
uint32_t last_time; //used for timing

char network[] = "116 berkshire floor 3";
char password[] = "caterpillar420";

const int BUTTON = 5;

/* Having network issues since there are 50 MIT and MIT_GUEST networks?. Do the following:
    When the access points are printed out at the start, find a particularly strong one that you're targeting.
    Let's say it is an MIT one and it has the following entry:
   . 4: MIT, Ch:1 (-51dBm)  4:95:E6:AE:DB:41
   Do the following...set the variable channel below to be the channel shown (1 in this example)
   and then copy the MAC address into the byte array below like shown.  Note the values are rendered in hexadecimal
   That is specified by putting a leading 0x in front of the number. We need to specify six pairs of hex values so:
   a 4 turns into a 0x04 (put a leading 0 if only one printed)
   a 95 becomes a 0x95, etc...
   see starting values below that match the example above. Change for your use:
   Finally where you connect to the network, comment out 
     WiFi.begin(network, password);
   and uncomment out:
     WiFi.begin(network, password, channel, bssid);
   This will allow you target a specific router rather than a random one!
*/
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.


void setup() {
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  Serial.begin(115200); //begin serial
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
      uint8_t* cc = WiFi.BSSID(i);
      for (int k = 0; k < 6; k++) {
        Serial.print(*cc, HEX);
        if (k != 5) Serial.print(":");
        cc++;
      }
      Serial.println("");
    }
  }
  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);

  uint8_t count = 0; //count used for Wifi check times
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
  randomSeed(analogRead(A0)); //"seed" random number generator

  pinMode(BUTTON, INPUT_PULLUP);
  int api_request_num = 0;
}

void request_numbers(int num) {
  //formulate GET request...first line:
    sprintf(request_buffer, "GET http://numbersapi.com/%d/trivia HTTP/1.1\r\n", num);
    strcat(request_buffer, "Host: numbersapi.com\r\n"); //add more to the end
    strcat(request_buffer, "\r\n"); //add blank line!
    //submit to function that performs GET.  It will return output using response_buffer char array
    do_http_GET("numbersapi.com", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    Serial.println(response_buffer); //print to serial monitor
    
    tft.fillScreen(TFT_BLACK); //black out TFT Screen
    tft.setCursor(0, 0, 1);
    tft.println(response_buffer);
}

/*-----------------------------------
  Generate a request to the numbersapi server for a random number
  Display the response both on the TFT and in the Serial Monitor
*/

int api_request_num = 0;

bool pushes_started = false;
bool pushed_down = false;

void loop() {

  //check for whether the button is pressed
  int unpushed_read = digitalRead(BUTTON);

  if (unpushed_read) {

    // the most recent button state was pushed down
    if (pushed_down) {
      pushed_down = false;
    }

    // check for if there has been a sequence of pushes
    if (pushes_started) {
      // if button press period is timed out, request api
      if ((millis() - last_time) > 1000) { 
        Serial.println("CALLING API");
        request_numbers(api_request_num);
  
        //reset vars
        api_request_num = 0;
        pushes_started = false;
      }
    }
  }

  // button is being pushed down
  else{
    pushes_started = true;

    // if most recent state was unpushed
    if (not pushed_down) {
      pushed_down = true;
      api_request_num ++;
      Serial.println(api_request_num);
    }
    
    //remember last time pushed for calling GET request
    last_time = millis(); 
  }
  delay(100);  
}


/*----------------------------------
   char_append Function:
   Arguments:
      char* buff: pointer to character array which we will append a
      char c:
      uint16_t buff_size: size of buffer buff

   Return value:
      boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
   do_http_GET Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP GET request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
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
      if (
() - count > response_timeout) break;
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
