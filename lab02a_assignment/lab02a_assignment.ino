#include <mpu6050_esp32.h>
#include<math.h>
#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h


uint8_t state; //state variable
int steps; //counting steps
float old_acc_mag, older_acc_mag; //maybe use for remembering older values of acceleration magnitude
float acc_mag = 0;  //used for holding the magnitude of acceleration
float avg_acc_mag = 0; //used for holding the running average of acceleration magnitude

const float ZOOM = 9.81; //for zooming plot (converts readings into m/s^2)...used for visualizing only

const uint8_t LOOP_PERIOD = 10; //milliseconds
uint32_t primary_timer = 0;
float x, y, z; //variables for grabbing x,y,and z values

// VARS FOR CHECK STEP
int upper_threshold = 10.5;
int lower_threshold = 8;
int peak_state = 0;

MPU6050 imu; //imu object called, appropriately, imu

void setup() {
  Serial.begin(115200); //for debugging if needed.
  delay(50); //pause to make sure comms get set up
  Wire.begin();
  delay(50); //pause to make sure comms get set up
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  tft.init(); //initialize the screen
  tft.setRotation(2); //set rotation for our layout
  primary_timer = millis();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  steps = 0; //initialize steps to zero!
}

void loop() {
  imu.readAccelData(imu.accelCount);
  x = imu.accelCount[0] * imu.aRes;
  y = imu.accelCount[1] * imu.aRes;
  z = imu.accelCount[2] * imu.aRes;

  //Serial printing:
  char output[80];

  acc_mag = sqrt((x*x) + (y*y) + (z*z));
  avg_acc_mag = ( acc_mag + old_acc_mag + older_acc_mag)/3.0;
  older_acc_mag = old_acc_mag;
  old_acc_mag = acc_mag;

  // check if step has been taken
  check_step();
  
  sprintf(output, "%4.2f,%4.2fz", ZOOM * acc_mag, ZOOM * avg_acc_mag); //render numbers with %4.2 float formatting
  Serial.println(output); //print to serial for plotting
  Serial.println(steps);
  
  //redraw for use on LCD (with new lines):
  //TOTALLY CHANGE WHAT YOU PRINT HERE. %d will place integer into string
  sprintf(output, "%4.2f  \n%4.2f  \n%4.2f", x, y, z); //render numbers with %4.2 float formatting
  tft.setCursor(0, 0, 4);
  tft.println(output);
  tft.println(steps);
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}

void check_step(){

  // check the current peak of the mag
  if (ZOOM * avg_acc_mag > 12){ // GRAPH IS PEAKING
    Serial.println("PEAKING HIGH");
    
    // most recent peak state was the opposite 
    if (peak_state == 0){
      peak_state = 1;
    }
  }

  else if (ZOOM * avg_acc_mag < 8) { // GRAPH IS **NOT** PEAKING
    Serial.println("DIPPING LOW");

    //check if most recent state was opposite
    if (peak_state == 1){
      Serial.println(" *** STEP REGISTERED");
      peak_state = 0;
      steps ++;
    }
  }
  
  
}
