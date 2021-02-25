#include <mpu6050_esp32.h>
#include<math.h>
#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h


const uint8_t LOOP_PERIOD = 10; //milliseconds
uint32_t primary_timer = 0;
float x, y, z; //variables for grabbing x,y,and z values

float old_acc_mag, older_acc_mag, avg_acc_mag, acc_mag; // used for creating a 3-way avg

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
}

void loop() {
  imu.readAccelData(imu.accelCount);
  x = imu.accelCount[0] * imu.aRes; //store convert x val in g's
  y = imu.accelCount[1] * imu.aRes; //store convert y val in g's
  z = imu.accelCount[2] * imu.aRes; //store convert z val in g's
  //Serial printing:
  char output[40];

  three_way_avg();

  sprintf(output, "%4.2f,%4.2f,%4.2f", ); //render numbers with %4.2 float formatting
  Serial.println(output); //print to serial for plotting

//  sprintf(output, "%4.2f,%4.2f,%4.2f", x, y, z); //render numbers with %4.2 float formatting
//  Serial.println(output); //print to serial for plotting

  
  //redraw for use on LCD (with new lines):
  sprintf(output, "%4.2f  \n%4.2f  \n%4.2f  ", x, y, z); //render numbers with %4.2 float formatting
  tft.setCursor(0, 0, 4);
  tft.println(output);
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}

void three_way_avg(){
  acc_mag = sqrt((x*x) + (y*y) + (z*z))
  avg_acc_mag = ( acc_mag + old_acc_mag + older_acc_mag)/3.0;
  old_acc_mag = avg_acc_mag;
  older_acc_mag = old_acc_mag;
}
