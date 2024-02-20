## PicoEeprom64

Use a Raspberry Pi Pico to store saves that require using a 4Kilobit EEPROM for N64 games.

## How it Works

Connect from the cartridge slot Pin 21 (S_Dat) to GPIO pin 5 on the Pico. Also connect 3V3 to power and GND to ground. 
Removed the need for the clock pin.

## How to Use

Hit save in program, LED on Pico will turn on and off indicating save has been stored into the Picos EEPROM. Do not turn off the console while this LED is on.

*** You should wait a few seconds after saving to ensure data is written into the Flash Rom of the Pico. This code has not been thoroughly tested for all cases ***
