//-------------------------------------------------------------------------------------
// Name:		    dudemaster
// Description:	An avrdude emulator for arduino
// 
// History:    
//	16/12/2014 - Initial GitHub release
//	
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
#include <SD.h> 
#include <Streaming.h>
#include <DudeLib.h> 

#define VERSION           "0.0.1"
#define TX_EN_PIN         9
#define CS_PIN            4
#define HEX_FILE          "dudetest.hex"  //This is copied from build directory 

//Global variables
DudeLib dude(&Serial1, TXEN_PIN, CS_PIN);
uint8_t c, i;
uint16_t result;

void setup() { 
  //Set debug port
  Serial.begin(BOOT_BAUDRATE);	
  Serial << "Dudemaster program v." << VERSION << endl;
  
  // AVRDUDE SECTION --------------------------------------
  //Set communications with target board
  Serial1.begin(BOOT_BAUDRATE);	
  
  if (dude.error()== 0)
    Serial << "\tDudemaster ready." << endl;
  else
    Serial << "DudeLib error: " << dude.error() << endl;
}

void loop() { 
  // check for messages from target device
  for(int i=0;i<20;i++) {
    if (Serial1.available()>0) {
      c = Serial1.read();
      Serial.write(c);
    }
  }
  //Wait for commands from host
  if (Serial.available() > 0) {
    //if command is Uppercase send to target device
    c = Serial.read();
    if (c >= 'A' && c <='Z') {
      tx_buf[0] = c;
      send(tx_buf, 1);
    }
    
    //Execute host commands
    switch(c) {
      case 's':
        Serial << "sync target device => " << dude.sync_device() << endl;
        break;
      case 'i':
        Serial << "get target device information => ";
        if (dude.get_device_information(info)) {
          Serial << "Target device found: " << endl;
          Serial << "\tHardware version: " << info[0] << endl;
          Serial << "\tSoftware version: " << info[1] << "." << info[2] << endl;
          Serial << "\tDevice signature: 0x" <<_HEX(info[3]) <<""<< _HEX(info[4]) <<""<< _HEX(info[5]) << endl;
        }
        else  {
          Serial << "Target device not responding." << endl;
        }
        break;
      case 'w':
        Serial << "program device...";
        result =  dude.program_device(HEX_FILE) << endl;
        if (result) 
          Serial << result << " bytes written" << endl;
        else
          Serial << "program device failed." << endl;
        break;
      case 'v':
        Serial << "verify programmed device...";
        result =  dude.verify_device(HEX_FILE) << endl;
        if (result) 
          Serial << result << " program OK" << endl;
        else
          Serial << "program is not correct." << endl;
        break;
      case 'r':
        Serial << "read device program...";
        //result =  dude.read(HEX_FILE) << endl;
        if (result) 
          Serial << result << " bytes written" << endl;
        else
          Serial << "program read failed." << endl;
        break;
      case 'z':
        Serial << "reset device...";
        Serial1.print('R');
        break;
      case 'i':
        Serial << "get hello from device...";
        Serial1.print('I');
        break;
    }
  }
} 