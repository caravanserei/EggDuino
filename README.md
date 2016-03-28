# Arnd's Eggduino
Modifications on Eggduino-style bots with Arduino Uno controller and a GRBL-shield.

IMPORTANT: In this repository you will find new and modified files only. These are not intended to be taken as stand-alone installation. I assume that you already have a basic installation (hopefully working for the most part) from the original sources of Inkscape, of the EggBot Inkscape extensions, of the Arduino IDE and of the Eggduino firmware itself, but either still struggle for a way to get this distributed stuff running together or are just interested in a way on how alterations and improvements could be made at all.

This contribution is based on COCKTAILYOGI's great Arduino-based Eggduino project and on a remix of GLASSWALKER's Revised Fully Printable Eggbot. The seed however was laid by Evil-Mad-Scientists and their EggBot and Brian Schmalz's EiBotBoard. The Eggduino approach seems to get a real kick now, because obviously there are no more EiBotBoards available worldwide any more - not even cloned ones. So you are more or less forced into an Arduino approach of which kind ever. 

You will find several modifications on software and Python scripts and some suggestions on hardware and 3D-printable parts. Meanwhile you can find so many derivates of Eggduinos, SphereBots and the like, that it is hard to tell who contributed what first, but I hope that I figured it out the right way:

The EggBot inventors of EVIL-MAD-SCIENTISTS for giving the idea and the basic tool-chain: http://wiki.evilmadscientist.com/Installing_software

The mechanics concept of a print-at-home-bot by GLASSWALKER: http://www.thingiverse.com/thing:20398

The software concept of EggDuino-on-Arduino by COCKTAILYOGI: https://github.com/cocktailyogi/EggDuino and http://www.thingiverse.com/thing:302148

And lots of contributors like BARTIBOR or FINDUS4FUN for their improvements.

Well, and here are those of mine:

![Eggduino](./Arnds_Eggduino_modified.JPG?raw=true "Eggduino")
Picture by Arnd Schaffert

25.03.2016 - Easter 2016 - Created this repository to share my experiences and modifications

- Hardware:
	- [Overview](./Eggduino_with_Arduino_and_GRBL-Shield.pdf) 	
	- Integration of a standard GRBL CNC-shield.
	- Lean solution for power supply from one source. Needs just one single wire to be soldered on PCB.
	- Easy access to A4988-1/16-step-drivers. 
	- X- and Z-channel are choosen intentionally as Egg- and Pen-motor-channels to free the two PWM-ports normally assigned as DIR and STEP of the Y-channel as PWM-output for servo and engraver.
	- Adaptor board for easy attachment of servo and engraver.
	- Optional wiring to read the main supply voltage.
	- Engraver control unit with a tiny PCB. Circuit and layout available as PDF.
- Software:
	- New assignements of I/O-ports to fit the GRBL-CNC-shield including user keys, servo- and engraver-PWM.
	- 0-100% PWM-control of the engraver as on EiBotBoard.
	- Check of main supply voltage (almost) as on EiBotBoard. This is a quick check only and no precise measurement.
	- New option to invert servo motion (0°->180° vs 180°->0°) for more flexibility with servo mounting position. Stored in EEPROM.
	- Added optional check of main supply voltage by activation of QC-command. Needs the appropriate hardware modification.
	- Activated servo up/down speed option ("slow down" is most important!) with a simple delay algorithm based on microseconds delay. Stored in EEPROM.
- 3D printable parts:
	- Two versions of a direct mount of ARDUINO UNO with GRBL-shield to frame.
	- Clip-on protection cover for the electronics (see main picture above).
	- Modified bearing holders to fit 6x19mm bearings (instead of 8x23mm) and 6mm threaded rods (instead of 8mm).
	- Egg-coupler for round (non-flatted) 5mm stepper axle. Intended to be used with a simple rubber ring as egg-coupler.
	- Modified pen-arm to fit a 9g-servo with 20mm body length and 23mm hole spacing.
	- Modified spring to fit the servo. I did not get along well with the fixture of the servo arm to the spring, so my spring is just a springy buffer.
- Scripts of the Inkscape extension:
	- Integration of servo inversion option into user interface. Check eggbot.py and eggbot.inx for details.
	- Deal with failing search of Arduino-based COM-Ports: Just search for the right eggs ;-)
	- Quick check of main supply to give a warning before printing when only USB-powered (this is an Arduino-specific problem).
	- Recommendation: Assign Eggduino to a fixed port COM88 (haha: EIghtyEIght ;-) on Windows. I highly recommend this method (to be done manually just once) to avoid conflicts with other Arduinos that might be attached at the same time, because all of them would have the same USB-name "Arduino Uno" and the same USB-PID/VID!
	- Added recognition for Ardunino Clones with CH340G USB-device. Now accepting EiBotBoard, Arduino Uno with 16u2 and Arduino Uno clones with CH340G. Check ebb_serial.py for details.
- Environment used:
	- Tested on Win7/64-Pro and Win7/32-Home only up to now
	- Arduino IDE 1.6.7 and IDE 1.6.8
	- Inkscape 0.91
	- Python 2.7.2
	- Arduino Uno R3 with Atmel 16u2 USB-device (USB-auto-reset currently NOT deactivated; doesn't bother me too much up to now)
	- Arduino Uno R3 with WCH CH340G USB-device
	- Protoneer GRBL CNC-shield V3.00
	- Pololu stepper shields with A4988 in 1/16th microstep mode

DISCLAIMER: This project was made for fun and for private purposes only and it comes as it is without any warranty or guaranteed properties. Use at your own risk including the risk of damaging all of your devices and loss of data.

Arnd Schaffert, March 2016

Munich, Germany




