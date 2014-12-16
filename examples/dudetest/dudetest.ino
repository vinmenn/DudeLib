//-------------------------------------------------------------------------------------
// Name:		    dudetest
// Description:	Test program for dudelib library
//
// History:    
//	16/12/2014 - Initial GitHub release
//
// License
// "MIT Open Source Software License":
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in the
// Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//-------------------------------------------------------------------------------------

#define BOOT_BAUDRATE    19200
#define LED_PIN          13 
#define RESET_PIN        8
#define RESET_DELAY      500
#define USE_RS_485
#define TXEN_PIN         9
#define V_MSG            "Hello from dudetest v.0.0.1"

char msg[32];

void setup() { 
  //Init serial port at same spedd of bottloader
  Serial.begin(BOOT_BAUDRATE); 	
  
  //Init s485 pin if defined
#if defined(USE_RS_485)
  pinMode(TXEN_PIN, OUTPUT);
  digitalWrite(TXEN_PIN, LOW); //Set RS485 in reception mode
#endif
  //Debug led
  pinMode(LED_PIN, OUTPUT); 
  
  //Send hello message to master node
  strcpy(msg, V_MSG);
  send(msg);
  //all ok 
  blink(3);
}

void loop() { 
  char num[10];
  //reset watchdog
  if (Serial.available() > 0) {
    switch(Serial.read()){
      case 'H': //Send hello message
        send(V_MSG);
        break;
      case 'R': //Reset board with reset pin
        delay(RESET_DELAY);
        //Set value before setting output
        digitalWrite(RESET_PIN, LOW); 
        //Now pin could go low
        pinMode(RESET_PIN, OUTPUT); 
        break;
    }
  }
  //Program is running
  blink(1);
}

//Send buffer on dude port and manage RS485 TxEnable pin
void send(char* message) {
#if defined(USE_RS_485)
  digitalWrite(TXEN_PIN, HIGH); //TX
#endif 
  Serial.println(message);
  Serial.flush();
#if defined(USE_RS_485)
  digitalWrite(TXEN_PIN, LOW); //RX
#endif 
}
//simply blink led
void blink(uint8_t n) {
    for(uint8_t i=0;i<n;i++) {
      digitalWrite(LED_PIN, LOW);
      delay(250);
      digitalWrite(LED_PIN, HIGH);
      delay(250);
    }
} 

