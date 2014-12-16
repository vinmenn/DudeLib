 //-------------------------------------------------------------------------------------
// Name:		dudelib
// Description:	An avrdude compatible library
// Use this library to program target arduino boards through a "master" arduino
//
// History:    
//	16/12/2014 - Initial release on GitHub
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
#ifndef DUDELIB_H
#define DUDELIB_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <SD.h> 
#include "stk500.h"

#define atohex(high_nibble, low_nibble) ((char_to_hex(high_nibble) << 4) | char_to_hex(low_nibble))

//Avrdude settings
#define AVR_BAUDRATE        19200
#define RESET_DELAY         1300
#define MAX_TRIES           10
#define RX_WAIT_TIMEOUT     500
#define SETTLE_TIME      	  100 

//Target section: this is for Avr328p
#define TARGET_PAGESIZE  	  128
#define MAX_BUFFER          16

//SD library settings
#define SEL_PIN			        53 

//Error codes
#define SD_INIT_ERROR       0x01
#define NO_RESPONSE         0x02
#define HEX_FILE_NOT_FOUND  0x03
#define WRONG_DEVICE_TYPE   0x04
#define WRONG_HEX_FORMAT    0x05
#define ERROR_CHECKSUM      0x06

extern "C" {
	// callback function types
    typedef void (*resetCallback)();
} 

// Dude
class DudeLib {
	private:
		//load flash memory address
		uint8_t load_address(uint16_t address);
		//Write flash page
		uint8_t write_page(uint8_t* buffer, uint16_t length);
		//Read flash page
		uint8_t read_page(uint8_t* buffer, uint16_t length);
		//Leave program mode (restart target device)
		uint8_t leave_program_mode();
    //Send stk500 command to target device
    void send(uint16_t length);
    void send(uint8_t* buffer, uint16_t length);
    //Check empty response from target device
    uint8_t empty_response();
    //Check response with data from target device
    uint8_t response(uint8_t* buffer, uint8_t length, uint8_t start);
    //Receive data before timeout
    uint8_t receive();
		//convert ascii char to nibble value
    char char_to_hex(char a);
	
    HardwareSerial* _serial;
    uint8_t _tx_buf[MAX_BUFFER];
    uint8_t _page_buffer[TARGET_PAGESIZE];
    uint8_t _temp_buffer[TARGET_PAGESIZE];
    uint8_t _tx_enable;
    uint8_t _cs;
    uint8_t _error;
     
	public:
    uint8_t begin(HardwareSerial *serial, uint8_t tx_enable_pin, uint8_t cs_pin);
		//Check device connection
		uint8_t sync();
		
		//Get target device information
		uint8_t get_information(uint8_t* info);
		
		//program target arduino with hex file and optionally verify programmed firmware
		uint16_t program(char *filename);
		uint8_t verify(char *filename);
		
		// //read firmware from target arduino and write to out_file
		// uint8_t read(char *out_filename);
    
    //reset callback;
    resetCallback reset_device;
		
    //error status
    inline uint8_t get_error() { return _error; }
};

//---------------------------------------------------
//DudeLib constructor
//---------------------------------------------------
uint8_t DudeLib::begin(HardwareSerial *serial, uint8_t tx_enable_pin, uint8_t cs_pin) {
  _serial = serial;

  // AVRDUDE SECTION --------------------------------------
  //Set communications with target board
  _cs = cs_pin;
  _tx_enable = tx_enable_pin;
  _error = 0;
  
  if (_tx_enable) {
    pinMode(_tx_enable, OUTPUT);
    digitalWrite(_tx_enable, LOW); //Set RS485 in reception mode
  }
  
  // SD SECTION -------------------------------------------------
  // Check sd card
  pinMode(SEL_PIN, OUTPUT);
  
  // init SD card
  if (!SD.begin(_cs)) {
    _error = SD_INIT_ERROR;
    return false;
  }

  return true;
}

//---------------------------------------------------
// Check if device is connected
//---------------------------------------------------
uint8_t DudeLib::sync() {
  for(uint8_t i=0;i<MAX_TRIES;i++) {
    _tx_buf[0] = STK_GET_SYNC;
    _tx_buf[1] = CRC_EOP;
    send(2);
    if (empty_response())
      return true;
  }
  return false;
}

//---------------------------------------------------
// Return hw/sw versions and device signature
//---------------------------------------------------
uint8_t DudeLib::get_information(uint8_t* info) {
  
  if (!sync()) return false;

  for(uint8_t i=0;i<MAX_TRIES;i++) {
    //Hw & sw versions
    _tx_buf[0] = STK_GET_PARAMETER;
    _tx_buf[1] = 0x80 + i;
    _tx_buf[2] = CRC_EOP;
    send(3);
  
    if (!response(info, 1,  i)) {
      _error = NO_RESPONSE;
      return false;
    }
  }
       
  //Device signature
  _tx_buf[0] = STK_READ_SIGN;
  _tx_buf[1] = CRC_EOP;
  send(2);

  return response(info, 3, 3);
}

//---------------------------------------------------
// Program target device using hex file found on SD card
//---------------------------------------------------
uint16_t DudeLib::program(char *filename) {
  
  uint8_t length = 0;
  uint16_t address = 0, tmp_address = 0; 
  uint16_t totlen = 0;
  uint8_t b, p = 0, cksum = 0;
  uint8_t info[6];
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File file = SD.open(filename);

  // if the file is not available exit
  if (!file) {
	  _error = HEX_FILE_NOT_FOUND;
	  return false;
  }
  
  // Reset target device with external function
  if (reset_device)
    reset_device();

  if (!sync()) return false;

  // Read device information
  get_information(info);

  //Check device type: 1E95F
  // if (info[3] != 0x01 || info[4] != 0xE9 || info[5] != 0x5F) {
	  // _error = WRONG_DEVICE_TYPE;
	  // return false;
  // }
  
  //Read hex file decode data and send to target device
  // For hex format see: 
  // http://www.piclist.com/techref/fileext/hex/intel.htm
  while (file.available()) {
    if (file.read() != ':') {
		  _error = WRONG_HEX_FORMAT;
		  return false;
    }
	
    //data length
    length = atohex(file.read(), file.read());
    //length = ((char_to_hex(file.read()) << 4) | char_to_hex(file.read()));
    cksum = length;

    //address (16 bit)
	  //High byte
    b = atohex(file.read(), file.read());
    cksum += b;
    tmp_address = b;
	  //Low byte
    b = atohex(file.read(), file.read());
    cksum += b;
    tmp_address = (tmp_address << 8) + b;

    // record type
    b = atohex(file.read(), file.read());
    cksum += b;

    //data buffer
    for (uint8_t i=0; i < length; i++) {
		b = atohex(file.read(), file.read());
		cksum += b;
		_page_buffer[p++] = b;
		  
      //if pagebuffer is full send to target device
      if (p == TARGET_PAGESIZE) {
        // write flash page
        load_address(address>>1);
        write_page(_page_buffer, TARGET_PAGESIZE);
        // update pointers
        p = 0;
        address += TARGET_PAGESIZE;
      }
    }
		
    // check sum
		b = atohex(file.read(), file.read());
    cksum += b;
    if (cksum != 0) {
	    _error = ERROR_CHECKSUM;
      return false;
    }
		
    //End of line
    file.read(); //0x0D
    file.read(); //0x0A
	
    totlen += 13 + length*2;
  }
  //End of file
  //Send last buffer chunk to target device
  if (p > 0) {
    // write flash page
    load_address(address>>1);
    write_page(_page_buffer, p);
  }
  file.close();
    
  //End of programming sequence hope is all correct
  if (!leave_program_mode())
	  return 0;
  else
    return totlen;
}

//---------------------------------------------------
// Verify programmed device
//---------------------------------------------------
uint8_t DudeLib::verify(char *filename) {
  
  uint8_t length = 0;
  uint16_t address = 0, tmp_address = 0; 
  uint16_t totlen = 0;
  uint8_t b, x, p = 0, cksum = 0;

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File file = SD.open(filename);

  // if the file is not available exit
  if (!file) {
	  _error = HEX_FILE_NOT_FOUND;
	  return false;
  }

  // Reset target device with external function
  if (reset_device)
    reset_device();

  if (!sync()) return false;

  //Read hex file decode data and send to target device
  // For hex format see: 
  // http://www.piclist.com/techref/fileext/hex/intel.htm
  while (file.available()) {
    if (file.read() != ':') {
		  _error = WRONG_HEX_FORMAT;
		  return false;
    }
	
    //data length
    length = atohex(file.read(), file.read());
    cksum = length;

    //address (16 bit)
	  //High byte
    b = atohex(file.read(), file.read());
    cksum += b;
    tmp_address = b;
	  //Low byte
    b = atohex(file.read(), file.read());
    cksum += b;
    tmp_address = (tmp_address << 8) + b;

    // record type
    b = atohex(file.read(), file.read());
    cksum += b;

    //data buffer
    for (uint8_t i=0; i < length; i++) {
		  b = atohex(file.read(), file.read());
		  cksum += b;
		  _page_buffer[p++] = b;
		  
      //if pagebuffer is full send to target device
      if (p == (TARGET_PAGESIZE)) {
        //read flash page
        load_address(address>>1);
        DA QUI NON FUNZIONA
        read_page(_temp_buffer, TARGET_PAGESIZE);
        
        //Compare buffers
        for(x=0;x<TARGET_PAGESIZE;x++)
          Serial << x << "=" << _page_buffer[x] << "-" << _temp_buffer[x] << endl;
          if (_page_buffer[x] != _temp_buffer[x]) {
            leave_program_mode();
            return false;
          }
        // update pointers
        p = 0;
        address += TARGET_PAGESIZE;
      }
      
    }
		
    // check sum
		b = atohex(file.read(), file.read());
    cksum += b;
    if (cksum != 0) {
	    _error = ERROR_CHECKSUM;
      return false;
    }
		
    //End of line
    file.read(); //0x0D
    file.read(); //0x0A
	
    totlen += 13 + length*2;
  }
  //End of file
  //Send last buffer chunk to target device
  
  if (p > 0) {
    // write flash page
    load_address(address>>1);
    read_page(_page_buffer, p);
    //Compare buffers
    for(x=0;x<p;x++)
      if (_page_buffer[x]!=_temp_buffer[x]) {
        leave_program_mode();
        return false;
      }
  }
  
  file.close();
    
  //End of programming sequence hope is all correct
  leave_program_mode();
  
	return true;
}

//---------------------------------------------------
//Set destination address for flash read/write
//---------------------------------------------------
uint8_t DudeLib::load_address(uint16_t address) {
  _tx_buf[0] = STK_LOAD_ADDRESS;
  _tx_buf[1] = (address & 0xff);
  _tx_buf[2] = (address >> 8) & 0xff;
  _tx_buf[3] = CRC_EOP;
  send(4);
  
  return empty_response();
}

//---------------------------------------------------
// Write flash memory page
//---------------------------------------------------
uint8_t DudeLib::write_page(uint8_t* buffer, uint16_t length) {

  _tx_buf[0] = STK_PROG_PAGE;
  _tx_buf[1] = (length >> 8) & 0xff;
  _tx_buf[2] = (length & 0xff);
  _tx_buf[3] = 'F';
	
  send(4);
  send(buffer, length);
  _tx_buf[0] = CRC_EOP;
  send(1);
	
  return empty_response();
}

//---------------------------------------------------
// Read flash memory page
//---------------------------------------------------
uint8_t DudeLib::read_page(uint8_t* buffer, uint16_t length) {

  _tx_buf[0] = STK_READ_PAGE;
  _tx_buf[1] = (length >> 8) & 0xff;
  _tx_buf[2] = (length & 0xff);
  _tx_buf[3] = 'F';
  _tx_buf[4] = CRC_EOP;
  send(5);
  
	if (empty_response()) {
    _error = 0;
    for(uint16_t i=0;i<length;i++) {
        buffer[i] = receive();
        if (_error == NO_RESPONSE) return false; 
    }
  }
  Serial.print("*");
  return true;

}

//---------------------------------------------------
// Force target to leave program mode
//---------------------------------------------------
uint8_t DudeLib::leave_program_mode() {

  _tx_buf[0] = STK_LEAVE_PROGMODE;
  _tx_buf[1] = CRC_EOP;
  
  send(2);
	
  return empty_response();
	
}
//---------------------------------------------------
//Send buffer on dude port and manage RS485 TxEnable pin
//---------------------------------------------------
void DudeLib::send(uint16_t length) {
  send(_tx_buf, length);
}

//---------------------------------------------------
//Send buffer on dude port and manage RS485 TxEnable pin
//---------------------------------------------------
void DudeLib::send(uint8_t* buffer, uint16_t length) {
  if (_tx_enable) digitalWrite(_tx_enable, HIGH);

  _serial->write(buffer, length);
  _serial->flush();

  if (_tx_enable) digitalWrite(_tx_enable, LOW);
}

//---------------------------------------------------
//Check empty response from target device
//---------------------------------------------------
uint8_t DudeLib::empty_response() {
  return response(0, 0, 0);
}

//---------------------------------------------------
//Check response from target device
//---------------------------------------------------
uint8_t DudeLib::response(uint8_t* buffer, uint8_t length, uint8_t start) {

  delay(50);
  uint8_t i=0;
  for(i=0;i<MAX_TRIES;i++) {
    if (receive()== STK_INSYNC) break;
  }
  //flush
  if (i==MAX_TRIES) {
    while (_serial->available()>0) _serial->read();
    _error = NO_RESPONSE;
    return false;
  }
  
  if (length > 0)
    for(uint8_t i=0;i<length;i++) {
        buffer[start + i] = receive();
    }
      
  for(i=0;i<MAX_TRIES;i++) {
    if (receive()== STK_OK) break;
  }
  
  //flush
  if (i==MAX_TRIES) {
    while (_serial->available()>0) _serial->read();
    _error = NO_RESPONSE;
    return false;
  }
  
  return true;
}

//---------------------------------------------------
//Receive 1 byte from serial port before timeout
//---------------------------------------------------
uint8_t DudeLib::receive() {
  uint16_t t=0;
  
  while(t < RX_WAIT_TIMEOUT) {
    if (_serial->available()> 0) return _serial->read(); 
    t++;
  }
  _error = NO_RESPONSE;
  
  return 0xFF;
}

//---------------------------------------------------
// convert ascii char rapresentation of hex value
//---------------------------------------------------
char DudeLib::char_to_hex(char a) {
	if(a >= 'a') {
		return (a - 'a' + 0x0a);
	} else if(a >= 'A') {
		return (a - 'A' + 0x0a);
	} else if(a >= '0') {
		return(a - '0');
	}
	return a;
}

#endif  