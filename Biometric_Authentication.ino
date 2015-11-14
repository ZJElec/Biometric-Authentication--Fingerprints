// Biometric Authentication "Vending Machine" /Safe Box
#include <msp430.h>
#include "msp430g2553.h"
#include <Adafruit_Fingerprint.h>         // library for finger sensor
#include <SoftwareSerial.h>               // library for software serial communication
// servo header files
#include <Wire.h>                         
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();  // define pwm variable for servo driver
int getFingerprintIDez();                                 // define function of reading finger prints from the finger sensor
uint8_t getFingerprintEnroll(uint8_t id);                 // define function of enroll of the finger sensor
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
SoftwareSerial mySerial(P1_1, P1_2);                      // set P1.1 and P1.2 to communication
#define SERVOMIN  262 // this is the 'minimum' pulse length count (out of 4096),servo                // Note: this group of setting would tell the servo to turn 90 degree each time
#define SERVOMAX  488 // this is the 'maximum' pulse length count (out of 4096),servo
uint8_t servonum = 14;                                                                               // we are using the 14th and the 15th servo driver pins for the servo
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
const int buttonPin = P2_4;     // the number of the pushbutton pin
int buttonState = 0;            // variable for reading the pushbutton status
int led = P2_1;                 // define led pin
int led2= P2_2;                 // define another led pin
int flag=1;                     // flag of turning the servo
int flag0=0;                    // flag of enroll
char c='5';                     // variable starts from 5,(because we want id # larger than 5 be ids that are able to turn servs)
int breakoutflag=0;             // flag that break out the enroll mode
int deleteflag=0;               // flag of delete (if this flag is set, then goes to delete one finger mode)
int deletefingerflag=0;         // flag of delete one finger
int deleteallflag=0;            // flag of delete all fingers at once
int deleteallfingerflag=0;      // flag of delete (if this flag is set, then goes to delete all finger at once mode)
int notmatchflag=0;             // if finger not match (in the enroll mode)
uint8_t idnumber=5;             // idnumber starts from 5 (id number between 0 to 4 are reserved for master user only)
int Counter;                    // counter for the timer interrupt service routine
void setup()  
{
  pinMode(led, OUTPUT);          // set led pin to output mode
  pinMode(led2,OUTPUT);          // set led 2 to output mode
  analogFrequency(20000);        // set the frequency of PWM to be 20 kHz
  TACCTL0 = CCIE;                             // CCR0 interrupt enabled
  TACTL = TASSEL_2 + MC_1 + ID_3;             // SMCLK/8, upmode
  TACCR0 =  20000;                            // 16M/8/20000 = 100 Hz 
  pinMode(buttonPin, INPUT);     // set button pin to input mode
  Serial.begin(9600);           // begin serial, baud rate: 9600 bps
  Serial.println("fingertest");
  // servo setup
  pwm.begin();                     // set up servo driver pwm
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
  
  // set the data rate for the sensor serial port
  finger.begin(57600);
  if (finger.verifyPassword()) {                      // check if the system finds the sensor
    Serial.println("Found fingerprint sensor!");     // if find
  } else {
    Serial.println("Did not find fingerprint sensor :(");    // if not find
    while (1);                                                // if not find, then goes to a while loop
  }
  Serial.println("Waiting for valid finger...");             // if find, then waiting for a finger to press on the sensor
}

void loop()                     // run over and over again
{
  getFingerprintIDez();                                       // call the sensing finger print function every loop
  if(flag0==1){                                               // if enroll flag is set
    Serial.print("ID #"); Serial.println(c);                // for the certain id number, print the id on the serial monitor (for debug purpose), c starts from 5, means, starting to enroll fingers from id number 5, since id number 0 to 4 are reserved for the master user only.
    uint8_t id = 0;                                           // define variable id
          id *= 10;                                           
          id += c - '0';                                      // let id equals variable c, id is in uint8 type
   digitalWrite(led,HIGH);                                    // Turn on the led indicating the enroll mode starts
   buttonState = digitalRead(buttonPin);                      // read the button pin state
   if (buttonState ==HIGH ){                                  // if the button is pressed
     breakoutflag=1;                                          // set the break out of enroll mode flag
   }
   if ( breakoutflag==1){                                     // if the breakout flag is set 
     flag0=0;                                                 // clear the enroll flag (means exiting the enroll mode)
     digitalWrite(led, LOW);                                  // meanwhile, turns off the led, indicating enroll mode is exit 
     breakoutflag=0;                                          // clear the break out flag for future use
   }
    else {                                                    // if breakout flag is not set (button not pressed)
       Serial.print("Enrolling ID #");                       // then starts the enroll process
       Serial.println(id);                
       while (!  getFingerprintEnroll(id) );                  // waiting to get the id number, sense a finger,sense this finger again, store the finger 
       if (notmatchflag == 0){                                // if "sense the finger again" step succeeded
         c = c + 1;                                           // then update the variable (id nubmer) c, for a next finger to enroll
       }
       else{                                                  // if "sense the finger again" step failed
         c = c + 0;                                           // id number c does not change, begins a next finger enroll using the same id number
         notmatchflag =0;                                     // clear the flag for future use
       }
       digitalWrite(led,LOW);                                 // turn off the led, indicating enroll process ends
    }
  }
  
  else if(deleteflag==1){                                   // check if the delete mode flag is set
        digitalWrite(led,HIGH);                             // if set, turn on the led
        while (deletefingerflag == 1){                      // while delete finger flag is set
              buttonState = digitalRead(buttonPin);         //  read the buttonstate
              if (buttonState ==HIGH ){                     // if it is high (button pressed)
                  breakoutflag=1;                           // set the break out flag (means to exit the delete mode)
              }
              if ( breakoutflag==1){                        // if break out flag is set
                  deleteflag = 0;                           // clear the delete flag (exit the delete mode)
                  digitalWrite(led, LOW);                   // meanwhile, turn off the led
                  breakoutflag = 0;                         // clear the break out flag for future use
                  deletefingerflag=0;                       // clear the delete finger flag for future use
              }      
              digitalWrite(led2,HIGH);                      // if button not pressed, turn on led 2, indicating begins of delete mode
              getFingerprintIDez();                         // get the finger print of the finger we want to delete
              if (finger.fingerID >=5){                    // if the finger we want to delete has a id larger or equal to 5 (in case, the master's finger is deleted)
              deleteFingerprint(finger.fingerID);          // call the delete function to delete the finger id and its finger print
              } 
          }
          deletefingerflag=1;                              // set the delete finger flag to continue delete finger until the button is pressed
          digitalWrite(led2,LOW);                          // turn off led 2 indicating the finger is deleted
  }
  
  else if (deleteallflag == 1){                            // if delete all flag is set, entering delete all finger id that is larger or equal to 5 at once. Caution: this mode would delete all fingers that could open the gate, except the master's 
       digitalWrite(led,HIGH);                             // turn on the led
        while (deleteallfingerflag == 1){                  // while delete all finger flag is set
              buttonState = digitalRead(buttonPin);        // read the button state
              if (buttonState ==HIGH ){                    // if the button is pressed
                  breakoutflag=1;                          // set the break out flag
              } 
              if (breakoutflag==1){                        // if break out flag is set
                  deleteallflag = 0;                       // clear the delete all flag (means to exit the mode)
                  digitalWrite(led, LOW);                  // meanwhile, turn off the led
                  breakoutflag = 0;                        // clear breakout flag for future use
                  deleteallfingerflag=0;                   // clear the delete all finger flag for future use
              }      
              digitalWrite(led2,HIGH);                     // turn on led 2 indicating delete all mode starts
              getFingerprintIDez();                        // call the finger print sensing function
              if (finger.fingerID == 3){                   // if the finger ID is 3 (Note: Because it is the delete all mode, the master has to use two fingers to enter this mode, in case of accident)
                for (idnumber =5; idnumber <= 162; idnumber++){                 // for loop of idnunbers ,start from 5
                  deleteFingerprint(idnumber);                                  // delete all the finger prints in this range
                }
                idnumber = 5;                              // reset idnumber to 5 for future use
                deleteallflag = 0;                         // clear the flag
                deleteallfingerflag=0;                     // clear the flag
                digitalWrite(led,HIGH);                    // all finger prints deleted successfully
              }
              else {                                       // if the finger is not the correct finger, needs to go all over again
                  deleteallfingerflag=1;                   // keep the flag set
                  digitalWrite(led2,LOW);                  // turn off led 2 indicating start again
              }
        }
  }
  
  else if(flag == 1){                                      // the flag for servo control, if this flag is set, then goes to door open, door close algorithm
    for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX; pulselen++) {          // setup pwm of the servo driver (control the range of angle the servo rotates) to control the servo
          pwm.setPWM(servonum, 0, pulselen);                                       // open the door (90 degree rotation)
    }
    Serial.println("Door1 Open");
 
    for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN; pulselen--) {          
      pwm.setPWM(servonum, 0, pulselen);                                           // close the door (90 degree rotation in the inverse direction)
    }
    Serial.println("Door1 Close");
    servonum ++;                                           // servo number plus one, indicating now controling the second servo
    for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX; pulselen++) {           // same as above
          pwm.setPWM(servonum, 0, pulselen);
    }
    Serial.println("Door2 Open");
    for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN; pulselen--) {
      pwm.setPWM(servonum, 0, pulselen);
    }
    Serial.println("Door2 Close");
    servonum = 14;                                        // reset the servo number for future use. Note: we are connecting one servo on the 14th pin and another servo on the 15th pin on the servo driver.
    flag=0;                                               // clear the flag making sure only one parts come out of the vending machine each time.
    }
   
   __bis_SR_register(LPM0_bits + GIE);                    // Set the system to low power mode. The finger sensor and the servo driver are power consuming.
}

// Finger sensor function
// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();                         // set p as the variable of the finger print image
  if (p != FINGERPRINT_OK)  return -1;                   // any failure searching will return this function -1

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // if found a match
  Serial.print("Found ID #"); Serial.print(finger.fingerID); // print this finger print's id number on serial monitor (note: debugging use; real system does not need this function) 
  if(finger.fingerID>=5){                                 // if this certain finger print's id number is equal to or larger than 5
    flag=1;                                               // means this finger's function is open the door and get one part from the machine. Set flag to 1
    deletefingerflag=0;                                   // this flag only has function when the master wants to exit from delete one finger mode
  }
  if(finger.fingerID ==0){                                // if its id number is 0
    flag0=1;                                              // means this is the master's enroll mode finger; set the flag0 to 1 to enter the enroll mode
  }
  if(finger.fingerID == 1){                               // if its id number is 1
    deleteflag=1;                                         // means this is the master's delete one finger mode; set the deleteflag to 1
    deletefingerflag=1;                                   // meanwhile, set the delete finger flag to 1 to enter this mode
  }
  if (finger.fingerID == 2){                              // if its id number is 2
    deleteallflag=1;                                      // means this is the delete all finger prints mode; set the delete all flag to 1 to enter this mode
    deleteallfingerflag=1;                                // meanwhile, set the delete all finger flag to 1.
  }
  return finger.fingerID;                                 // this function returns the id number of the finger. (if searching failure, it would return -1) 
}

// servo function
void setServoPulse(uint8_t n, double pulse) {
  double pulselength;                                      // define variable pulselength, this variable is used to control the range of angle the servo rotates 
  pulselength = 1000000;                                   // 1,000,000 us per second
  pulselength /= 60;                                       // 60 Hz
  Serial.print(pulselength); Serial.println(" us per period"); 
  pulselength /= 4096;                                     // 12 bits of resolution
  Serial.print(pulselength); Serial.println(" us per bit"); 
  pulse *= 1000;
  pulse /= pulselength;                                   
  Serial.println(pulse);
  pwm.setPWM(n, 0, pulse);                                 // set up the pwm for the servo driver, n is the servo index (which servo is being controlled), pulse is the actual variable that makes the servo rotate.
}

// Enroll function
uint8_t getFingerprintEnroll(uint8_t id) {                // enroll a new finger print function
  uint8_t p = -1;
  Serial.println("Waiting for valid finger to enroll");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:                                 // if the sensor senses a finger print successfully
      Serial.println("Image taken");                        
      break;
    case FINGERPRINT_NOFINGER:                          // below are some errors indicating this process fails
      Serial.println(".");
      break;                                            // break out the while loop
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);                           // convert the image
  switch (p) {
    case FINGERPRINT_OK:                            // if the image sensed above is converted successfully
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:                     // below are some errors indicating this process failed
      Serial.println("Image too messy");
      return p;                                     // return p indicating which error occurred.
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");                         // after convert image successfully, need to remove finger for confirmation
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  p = -1;
  Serial.println("Place same finger again");              // indicates putting the same finger to confirm the enroll
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:                                  // if the image is taking successfully
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:                            // below are some errors indicating this process failed
      Serial.print(".");
      break;                                              // break out the while loop
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);                           // convert the image
  switch (p) {
    case FINGERPRINT_OK:                            // if the image is converted successfully
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:                     // below are some errors indicating this process failed
      Serial.println("Image too messy");
      return p;                                     // return p indicating which error occurred.
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  
  // OK converted!
  p = finger.createModel();                                // create data record for this certian finger print
  if (p == FINGERPRINT_OK) {                               // if the enrolling finger and the confirming finger is the same finger, then prints are matched successfully
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {          // below are some errors indicating this process failed
    Serial.println("Communication error");
    return p;                                              // return p indicating which error occurred.
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    notmatchflag=1;
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  p = finger.storeModel(id);                          // store the data of this finger print
  if (p == FINGERPRINT_OK) {                          // if the data is stored successfully
    Serial.println("Stored!"); 
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {     // below are some errors indicating this process failed
    Serial.println("Communication error");
    return p;                                         // return p indicating which error occurred.
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}

//Delet function
uint8_t deleteFingerprint(uint8_t id) {                   // this function is used to delete a finger id
  uint8_t p = -1;
  
  p = finger.deleteModel(id);                            // call the delete function in the header file directly

  if (p == FINGERPRINT_OK) {                             // if delte the id specified successfully, return nothing; deleting mode over
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {        // below are some errors indicating this process failed
    Serial.println("Communication error");
    return p;                                            // return p indicating which error occurred.
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }   
}

// Timer Interrupt Service Routine
__attribute__((interrupt(TIMER0_A0_VECTOR)))
static void TA0_ISR(void) { 
       if(Counter==5) {                                       // delay(50) 
                  __bis_status_register_on_exit(LPM0_bits);   // wake up from LPM0 mode
                  Counter=0;                                  // Clear the timer counter to start the next iteration
                  TACCR0=20000;
       }

       else {
    	   Counter++;                                         // update the Counter
       }
  }






