#include <SoftwareSerial.h>

SoftwareSerial mySerial(12, 13); // RX, TX - debug
// TODO - remove software serial println's - they may interfere with normal operation!

const int ledPin =  13;      // the number of the LED pin
// This motor should not have a delay lower then 6000ms!
const int RA_motor_pin_1 =  4; // These are the pins for the stepper originally hooked up
const int RA_motor_pin_2 =  2; // to the GS-280 hand controller on an EQ3 mount for RA
const int RA_motor_pin_3 =  5; // Essentially randmly chosen, but contiguous.
const int RA_motor_pin_4 =  3; //  
// This motor should not have a delay lower then 6000ms!
const int DEC_motor_pin_1 =  8; // These are the pins for the homebrew DEC motor
const int DEC_motor_pin_2 =  9; // 
const int DEC_motor_pin_3 =  10; // 
const int DEC_motor_pin_4 =  11; //  
// Variables will change:
int ledState = LOW;             // ledState used to set the LED
int numcount = 0;
String val = "0";      // This will be used for ascii to integer conversion
unsigned long RA_step_number = 107374182 ; // max long divided by two, for tracking steps.
unsigned long DEC_step_number = 107374182 ; // max long divided by two, for tracking steps.
int RA_step_direction = 1;
int DEC_step_direction = 1;
char cmdEnd;
char cmdBuffer;
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean RA_tracking_Enabled = true; // Always track - Currently unused
boolean RA_steppingEnabled = true; // Non-default RA movement if we are doing something other than tracking
boolean DEC_steppingEnabled = false; // DEC movement is always optional
// the follow variables are a long because the time, measured in RA_Microseconds,
// will quickly become a bigger number than can be stored in an int.

// Init RA timer variables
long previousStepRA_Micros = 0;        // will store last time Stepper was moved
// 10416 gives 1rp60s, 
// 17186 gives 1rev per 100 seconds
// 34372 - 1 rev per 195 seconds?
int minimumRA_StepInterval = 6000; // Stepping delays lower than this generally fail
long initialRA_StepInterval = 46410; // Delay between steps, timed with arduino so accuracy may be lacking
int maximumRA_StepInterval = ((initialRA_StepInterval - minimumRA_StepInterval) + initialRA_StepInterval);
unsigned long RA_StepInterval = initialRA_StepInterval;
// Init DEC timer intervals
long previousStepDEC_Micros = 0;        // will store last time Stepper was moved
// 10416 gives 1rp60s, 
// 17186 gives 1rev per 100 seconds
// 34372 - 1 rev per 195 seconds?
int minimumDEC_StepInterval = 6000; // Stepping delays lower than this generally fail
long initialDEC_StepInterval = 12000; // Delay between steps, its a number that works
int maximumDEC_StepInterval = ((initialDEC_StepInterval - minimumDEC_StepInterval) + initialDEC_StepInterval);
unsigned long DEC_StepInterval = initialDEC_StepInterval;
//long autoDrifttimerStart = 0;
int SerialOutputInterval = 100000;
int SerialInputInterval = 10000;
long previousSerialIn_Micros = 0;  // last time serial buffer was polled
long previousSerialOut_Micros = 0;  // Last time serial data was sent via timer
char cmdSwitch;
char newTime;
String newDelay;
char numberArray[5];

void setup() {
  Serial.begin(9600); 
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  pinMode(RA_motor_pin_1, OUTPUT);
  pinMode(RA_motor_pin_2, OUTPUT);
  pinMode(RA_motor_pin_3, OUTPUT);
  pinMode(RA_motor_pin_4, OUTPUT);
  pinMode(DEC_motor_pin_1, OUTPUT);
  pinMode(DEC_motor_pin_2, OUTPUT);
  pinMode(DEC_motor_pin_3, OUTPUT);
  pinMode(DEC_motor_pin_4, OUTPUT);
  mySerial.begin(4800);
  mySerial.println("Hello, world?");
}


void parseLX200(String thisCommand)
{
 mySerial.println(thisCommand);
 mySerial.println(inputString);
 switch (inputString.charAt(0)) { // If this isnt 58 (:), something is wrong!
     case ':':
      switch (inputString.charAt(1)) {
        case 'S':// Set Stuff
          switch (inputString.charAt(2)) {
              case 'w': // Slew rate
                Serial.println(1);
                mySerial.println("set max slew");
              break;
          }// end :Sw
          break; //Case S Char2
        case 'R':// Rate Control - R
          switch (inputString.charAt(2)) {
          case 'C':
            RA_StepInterval = (initialRA_StepInterval *2);
            DEC_StepInterval = (initialDEC_StepInterval *2);
            Serial.println(" Set interval to half default");
          break;
          case 'G':
           RA_StepInterval = initialRA_StepInterval;
           DEC_StepInterval = initialDEC_StepInterval;
           Serial.println(" Set interval to default");
          break;      
          case 'M':
           RA_StepInterval = initialRA_StepInterval/2;
           DEC_StepInterval = initialDEC_StepInterval/2;
           Serial.println(" Set interval to DOUBLE SPEED");
          break;
          case 'S':
           RA_StepInterval = minimumRA_StepInterval;
           DEC_StepInterval = minimumDEC_StepInterval;
           Serial.println(" Set interval to FASTEST");
          break;
      } // CaseR Char2
        break; // End Rate Control
        case 'M':  // Movement Control - M
          switch (inputString.charAt(2)) {
          case 'w':
            RA_step_direction = 1;
            RA_steppingEnabled = true;
            // We really just need to speed things up
            RA_StepInterval = (initialRA_StepInterval /4);
            Serial.println("Move RA forwards (west)");
          break;
          case 'e':
            RA_step_direction = 1;
            RA_steppingEnabled = true;
            // We really just need to slow things down
            RA_StepInterval = (initialRA_StepInterval *4);
            Serial.println("Move RA backwards (east) ");
          break;
          case 'n':
            DEC_step_direction = 0;
            DEC_steppingEnabled = true;
            Serial.println("Move DEC forwards (north)");
          break;
          case 's':
            DEC_step_direction = 1;
            DEC_steppingEnabled = true;
            Serial.println("Move DEC backwards (south)");
          break;
          } // CaseM Char2
        break; // End movemont control
      case 'Q': // Stop Moving - Q
        RA_steppingEnabled = 1; // We still move RA 
        RA_StepInterval = initialRA_StepInterval; // We just set the speed so that stars should be "stationary" relative to everything else
        DEC_steppingEnabled = 0;
        Serial.println ("Stepping halted");
        break;
      case 'G': // Get Data
        switch (inputString.charAt(2)) {
          case 'Z': // Azimuth
            Serial.println("123*56#"); // Bogus placeholder FIXME
            mySerial.println("sent azimuth");
          case 'D': // Declination
            Serial.println("+23*56#"); // Bogus placeholder FIXME
            mySerial.println("sent declination");
          case 'S': // Sidereal Time 
            Serial.println("12:34:56#"); // Bogus placeholder FIXME
            mySerial.println("sent sidereal");
          break;
          case 'V':
            switch (inputString.charAt(3)) {
              case 'F':
                Serial.println("HomebrewStar"); // Bogus placeholder FIXME
                mySerial.println("HomebrewStar"); 
           break; 
            } // CaseGV Char3       
          break; // CaseG
        } // CaseG Char2
       break; // FC Break
      } // Ending First Character Loop
    } // Ending Init Character Loop
} // Ending Function

void stepRA_Motor(int thisStep)
{
    switch (thisStep) {
      case 0:    // 1010
      digitalWrite(RA_motor_pin_1, HIGH);
      digitalWrite(RA_motor_pin_2, LOW);
      digitalWrite(RA_motor_pin_3, HIGH);
      digitalWrite(RA_motor_pin_4, LOW);
      break;
      case 1:    // 0110
      digitalWrite(RA_motor_pin_1, LOW);
      digitalWrite(RA_motor_pin_2, HIGH);
      digitalWrite(RA_motor_pin_3, HIGH);
      digitalWrite(RA_motor_pin_4, LOW);
      break;
      case 2:    //0101
      digitalWrite(RA_motor_pin_1, LOW);
      digitalWrite(RA_motor_pin_2, HIGH);
      digitalWrite(RA_motor_pin_3, LOW);
      digitalWrite(RA_motor_pin_4, HIGH);
      break;
      case 3:    //1001
      digitalWrite(RA_motor_pin_1, HIGH);
      digitalWrite(RA_motor_pin_2, LOW);
      digitalWrite(RA_motor_pin_3, LOW);
      digitalWrite(RA_motor_pin_4, HIGH);
      break;
    } 
}

void stepDEC_Motor(int thisStep)
{
    switch (thisStep) {
      case 0:    // 1010
      digitalWrite(DEC_motor_pin_1, HIGH);
      digitalWrite(DEC_motor_pin_2, LOW);
      digitalWrite(DEC_motor_pin_3, HIGH);
      digitalWrite(DEC_motor_pin_4, LOW);
      break;
      case 1:    // 0110
      digitalWrite(DEC_motor_pin_1, LOW);
      digitalWrite(DEC_motor_pin_2, HIGH);
      digitalWrite(DEC_motor_pin_3, HIGH);
      digitalWrite(DEC_motor_pin_4, LOW);
      break;
      case 2:    //0101
      digitalWrite(DEC_motor_pin_1, LOW);
      digitalWrite(DEC_motor_pin_2, HIGH);
      digitalWrite(DEC_motor_pin_3, LOW);
      digitalWrite(DEC_motor_pin_4, HIGH);
      break;
      case 3:    //1001
      digitalWrite(DEC_motor_pin_1, HIGH);
      digitalWrite(DEC_motor_pin_2, LOW);
      digitalWrite(DEC_motor_pin_3, LOW);
      digitalWrite(DEC_motor_pin_4, HIGH);
      break;
    } 
}

void loop()
{
  // Main loop..
 // if (stringComplete) 
 // {
 //   parseLX200(inputString); // Interpret string and act
    // clear the string:
 //   inputString = "";
 //   stringComplete = false;
 // }
  
  unsigned long currentRA_Micros = micros(); 
  unsigned long currentDEC_Micros = micros(); 
  
  //  Timer for serial out if needed - unused currently
  if(micros() - previousSerialOut_Micros > SerialOutputInterval) 
  {
    previousSerialOut_Micros = micros();
    // do Serial Output stuff
    //Serial.println(RA_StepInterval);
  }
  

// Do Stepper Stuff
if (RA_steppingEnabled)  {  // But only if RA stepEnabled is true!   
     // Start RA step loop
 if ( currentRA_Micros > (previousStepRA_Micros+minimumRA_StepInterval)) {// Dont step too fast
    if((currentRA_Micros - previousStepRA_Micros) > RA_StepInterval) {// Has time elapsed to step?  
      previousStepRA_Micros = currentRA_Micros; //Reset step timer
        if (RA_step_direction == 0) {// moving forwards?
          RA_step_number++;
        }
        if (RA_step_direction == 1) {// Or backwards?
          RA_step_number--;
        }
        stepRA_Motor(RA_step_number % 4);
      //Serial.println(RA_step_number);
      }  
    }
} // End RA step choice loop

if (DEC_steppingEnabled) {
      // Start DEC step loop
 if ( currentDEC_Micros > (previousStepDEC_Micros+minimumDEC_StepInterval)) {// Dont step too fast
    if((currentDEC_Micros - previousStepDEC_Micros) > DEC_StepInterval) {// Has time elapsed to step?
      previousStepDEC_Micros = currentDEC_Micros; //Reset step timer
        if (DEC_step_direction == 1) {// moving up?
          DEC_step_number++;
        }
        if (DEC_step_direction == 0) {// Or down?
          DEC_step_number--;
        }
        stepDEC_Motor(DEC_step_number % 4);
      //Serial.println(DEC_step_number);
      }  
    }
  } // End DEC step choice loop

}// End main loop

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    mySerial.print("rcv: ");
    mySerial.println(inChar);
    // Is it the Ack character?
   if (inChar == 6) 
  { 
    mySerial.println("GotAck - SendingP");
    Serial.println("P"); // Polar Mode
    inputString = ""; //
    return;
  } 
    // add it to the inputString:
    inputString += inChar;
    //mySerial.println("Something");
    //mySerial.println(inChar);
    if (inputString.startsWith(":"))   
    {
      if (inputString.endsWith("#"))   
      {
      // Starts with :, ends with # - OK
      stringComplete = true;
      parseLX200(inputString); // Interpret string and act - do this now as waiting may may lead to another command arriving before we do
    // clear the string:
    inputString = "";
    stringComplete = false;
      }
    }
    else 
    {
    // If it doesnt start with !, not valid, throw it away..
    inputString = "";
    } 
   }

}


