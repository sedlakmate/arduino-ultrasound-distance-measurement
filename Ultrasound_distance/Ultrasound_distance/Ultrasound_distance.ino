/**
 * This code is based on the <OCTOPUS 036A Segmentric LED Brick>'s sample library 
 * and the <HC-SR04 Ultrasonic Sensor>'s sample library. 
 * The merging process was carried out by Máté Szedlák, 2016.
 * 
 * License: GNU GENERAL PUBLIC LICENSE 3.0 
 * 
 * Description:
 *  Connect the ultrasonic sensor and the LED brick to the Arduino as per the
 *  hardware connections below. Run the sketch and open a serial
 *  monitor. The distance read from the sensor will be displayed
 *  in millimeters.
 *  
 *  If data is transferred as there is fresh result, the LED Brick will light more powerful 
 *  and each data transfer is signed by the blink of the built-in LED.
 *  
 *  If there is no active data transfer, the LED's brightness will change to low and
 *  the built-in LED will turn off.
 *  
 *  *******************************
 *  The distance is measured from the front face of the ultrasonic device.
 *  
 *  The calibration is done for the range 30 mm - 200 mm.
 *  Below 30 mm the results will be higher than should be.
 *  Over 200 mm is not tested.
 *  *******************************
 * 
 * Hardware Connections:
 *  Arduino | HC-SR04 
 *  -------------------
 *    5V    |   VCC  +330 Ohm   
 *    7     |   Trig +330 Ohm    
 *    8     |   Echo     
 *    GND   |   GND
 *    
 *  Arduino | OCTOPUS 036A 
 *  -------------------
 *    2     |   CLK     
 *    3     |   DIO +330 Ohm     
 *    3V    |   VCC +330 Ohm    
 *    GND   |   GND
 *  
 */

/* The lines marked with "***" belong to the 
 * OCTOPUS 036A Segmentric LED Brick. Otherwise can be deleted
 */
 
#include "TM1637.h"                         //   ***

// Pins
const int TRIG_PIN = 7;   // Ultrasound device 
const int ECHO_PIN = 8;   // Ultrasound device  

#define CLK 2             // LED display  
#define DIO 3             // LED display 

TM1637 tm1637(CLK,DIO);                     //   *** 

const int numReadings = 8;
const float threshold = 5;

float readings[numReadings];    // the readings from the analog input
int readIndex = 0;              // the index of the current reading
float sum = 0;                  // sum of readings
float average = 0;              // the average
float prevavg = 0;              // previous average
int count = 0;                  // counting readings' elements
int dropcount = 0;              // counting the measurement anomalies
int digitprint = 0;             // LED output       //   *** 

// Anything over 50 cm (2900 us pulse) is "out of range"
const unsigned int MAX_DIST = 2900;

void setup() {
  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  // Initialize built-in LED - Blinking signs active data transfer
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);           // Set built-in LED to default: Off  

  // We'll use the serial monitor to view the sensor output
  Serial.begin(9600);
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;  
  }

  tm1637.init();                                                       // *** 
  tm1637.set(4);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7; // *** 
  tm1637.point(POINT_ON);                                              // ***
}

void reset(){
  // If too many highly different measurement were recorded, 
  // the program drops all saved data and reset the variables.
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0; 
  }
  count = 0;
  readIndex = 0;
  sum = 0;
  average = 0;
  dropcount = 0;
  prevavg = 0;
}

void ledprint(int number){       //   *** The whole function belongs to the LED screen
  tm1637.clearDisplay();   
  int8_t digit[] = {0x7f,0x7f,0x7f,0x7f};  // Not showing any digit    
  boolean overthousand = false;
  if (number <= 9999){  
    if (number >= 1000) {
      digit[0] = number/1000;
      overthousand = true;
      number -= digit[0] * 1000;
    }   
    if (number >= 100 or overthousand) {
      digit[1] = number/100;
      number -= digit[1] * 100;
    }
    digit[2] = number / 10;
    number -= digit[2] * 10;
    digit[3] = number;  
  } else {    
      int8_t errorcode[] = {14, 0x7f, 0x7f, 6};
      tm1637.display(errorcode);
  }
  tm1637.display(digit);
}

void loop() {
  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float mm;

  // Hold the trigger pin high for at least 10 us
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait for pulse on echo pin
  while ( digitalRead(ECHO_PIN) == 0 );

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 1);
  t2 = micros();
  pulse_width = t2 - t1;

                   // Calculate distance in millimeters. The constants
                   // are found in the datasheet, and calculated from the assumed speed 
                   // of sound in air at sea level (~340 m/s).
                   // mm = round(pulse_width / 0.580)/10; // Original calculation method
  mm = round(pulse_width *1.79+6.76)/10; // Calibrated method
  // Print out results
  if ( pulse_width > MAX_DIST ) {
    // Serial.println("Out of range");
    tm1637.set(0);              //   *** 
    ledprint(digitprint);       //   *** 
  } else {
      if ((count < numReadings) or ((mm > average - threshold) and (mm < average + threshold))) {
        if (count < numReadings) {
          count += 1;
          } 
         // drop old data
        sum = sum - readings[readIndex];
        // add new data
        readings[readIndex] = mm;
        sum = sum + readings[readIndex];
        readIndex = readIndex + 1;
        if (readIndex >= numReadings) {
          // ...wrap around to the beginning:
          readIndex = 0;
          if (dropcount > 0) {
            dropcount -= 1;
          }
        }
        // Calcualting the average
        average = sum / numReadings;
        if ((count == numReadings) and (average < prevavg + threshold)
        and (average > prevavg - threshold)) {
          Serial.print(average);               
          Serial.println(",");
          //Serial.print(average);
          //Serial.println(",."); 
          tm1637.set(4);                    //   ***          
          digitprint =(int) (average*10.0); //   *** 
          ledprint(digitprint);             //   *** 
          digitalWrite(LED_BUILTIN, HIGH);  // Set built-in LED ON
        }
        prevavg = average;
      } else {
        tm1637.set(0);                      //   ***
        ledprint(digitprint);               //   ***
        if (count == numReadings){
          dropcount += 1;
          if (dropcount == 3){
            reset();
          }
        }
      }
  }
  
  // Wait at least 120ms before next measurement
  delay(120);
  digitalWrite(LED_BUILTIN, LOW);       // Set built-in LED to default: Off
}



//                        ENDOFTHESIS = true
