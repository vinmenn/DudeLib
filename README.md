DudeLib
=======

Program Arduino with Arduino!

##Description
This library is a completely software solution for remote uploading of compiled sketches into connected Arduinos.
Dudelib can upload a sketch using standard optiboot bootloader if a normal serial line is used (with DTR line
to reset Arduino), or using a lightly modified optiboot you could send a sketch through a local RS485 network.

##How it works
The bootloader normally starts after a hw/sw reset. If the reset is sw (eg. watchdog) it jumps direcly to 
the main program, otherwise it waits a second for a stk500 command received through serial line and
then start over with main program.
DudeLib acts as avrdude and sends commands to the bootloader to reprogram flash memory with hex file (the
compiled sketch) stored in a SD card.
DudeLib parse intel hex file produced from avr gcc and sends correct commands to bootloader.
DudeLib could also read firmware and store in a file for other uses. (this function is not ready yet)

##Limitations
DudeLib has only a small sets of avrdude commands implemented and is tested only with standard avr328p arduinos (Uno and pro mini) with optiboot bootloader installed. Other combinations of cpu / bootloader are not tested. If you want to extend compatibility please make attention to the timing after reset because is critical (you will need some tests to find correct delay).
Please note that the only way to reset Arduino and pass through bootloader sequence is with an hw reset so is mandatory to connect an unused pin with reset. Also is important to NOT set that pin as output because in that way the cpu will remain stalledin a initialization loop.

##What you need
The library has been tested on Arduino mega with ethernet shield but it should go also on a standard Uno.
(Mega is useful because has 3 serial ports so you could debug easily).
You need an SD card shield to read target hex.
You also need a serial interface of some kind (RS232, RS485, xbee, ...)
You need the interface on both master (Arduino Mega) and target boards.
This is an example of connection of target board.:
<picture>

##Usage
###Simple serial connection (also wireless)
Connect your master and target boards with serial line interface.
If you want to try a completely software solution you need to wire reset pin of target board with a free pin.
<image>
* Load dudetest.ino in the examples into target board.
* Load dudemaster.ino into master Arduino with another instance of Arduino ide 
Open serial console on the master.
* Reset manually target board (with button) and check that welcome message is sent to the master 
(and forwarded to console).
* Send a "s" command to check syncronization . if successful you should see target board restart and then 
the welcome message again.
* Send a "i" command to check hw infomation: Dudelib has been tested on avr328p only (Id: 0x1E95F)
* Returns to other ide with dudetest and update welcome message with new version (to recognize that firmware
is different). 
* Before compilation click on File -> Preferences and then select "Show output of compilation".
* Then click on "Verify" (we don't wan to send directly to the board) and take note of compilation directory.
* Go to the compilation directory and copy produced hex file to a microsd card but rename into "dudetest.hex".
* Send a "w" command to write new firmware to the target board, after upload you should see 
 the number of byes written and then the welcome message with new version from target board.

##Rs485
If you want to use RS485 you have to do extra steps to make it works:
You need a pin for TxEnable input of Rs485 transceiver chip. (for example pin 9)
Connect the chip as in this example:
![Rs485 connections](https://github.com/vinmenn/DudeLib/blob/master/images/Rs485%20connections.png)
For the target board you need also to use a modified bootloader to handle extra pin:
You could find the special version of optiboot here: [SodaqMoja/optiboot](https://github.com/SodaqMoja/optiboot)
It worked easily and avoided me to work also on the bootloader side.
* Copy the directory into Arduino\hardware\arduino\bootloaders\optiboot_rs485 
* Add this parameter to omake.bat present into modified optiloader directory
```
BAUD_RATE=19200 RS485=<TXENABLE PIN>
```
* Compile bootloader with txenable pin from commandline:
```
omake.bat atmega328
```
* Add a new section into boards.txt to point on new firmware:
```
uno2.name=Arduino with RS485 bootloader
uno2.upload.protocol=arduino
uno2.upload.maximum_size=30720
uno2.upload.speed=19200
uno2.bootloader.low_fuses=0xff
uno2.bootloader.high_fuses=0xda
uno2.bootloader.extended_fuses=0x05
uno2.bootloader.path=optiboot_rs485
uno2.bootloader.file=optiboot_atmega328.hex
uno2.bootloader.unlock_bits=0x3F
uno2.bootloader.lock_bits=0x0F
uno2.build.mcu=atmega328p
uno2.build.f_cpu=16000000L
uno2.build.core=arduino
uno2.build.variant=standard 
```
* Reprogram bootloader using another Arduino as ISP or though some ICSP interfaces (you could find instructions
on tons of articles starting from Arduino website.)
See for example [here](http://arduino.cc/en/Guide/ArduinoISP)

Now you have a bootloader compatible with RS485. 
At this point you could use the procedure already described and program an Arduino through a RS485 line.

If you want to try directly to program directly from arduino ide using a rs485 line you cold also connect a cheap
USB/RS485 adapter (like [these](http://www.ebay.co.uk/bhp/usb-to-rs485-converter)) and upload a sketch pressing reset on target Arduino a moment after end of compilation. This is a bit difficult but you could have success after few tries. 


Cheers. 






