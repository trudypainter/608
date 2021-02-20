#include<string.h>

void setup() {
  Serial.begin(115200);
  Serial.println("testing");
}

void loop() {
  //test case (taken from page or write your own!)
  Serial.println("Starting Test Case:");
  char in1[] = "ABCD";
  char in2[] = "12345678";
  char storage[300];
  memset(storage,0,sizeof(storage)); //fill array with all nulls
  interleaver(in1,in2,storage);
  Serial.println(storage);
  delay(500);
}

void interleaver(char* input_1, char* input_2, char* output) {

  //use a pointer for each string to add each char
  int pointer_1 = 0;
  int pointer_2 = 0;

  // run while both strings are NOT terminated
  while (input_1[pointer_1] != '\0'  || input_2[pointer_2] != '\0') {

   if (input_1[pointer_1] != '\0'){
    output[pointer_1 + pointer_2] = input_1[pointer_1];
//    Serial.println(input_1[pointer_1]);
    pointer_1++;
   }

   if (input_2[pointer_2] != '\0'){
    output[pointer_1 + pointer_2] = input_2[pointer_2];
//    Serial.println(input_2[pointer_2]);
    pointer_2++;
   }
  }
  
}
