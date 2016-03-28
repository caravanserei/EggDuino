# ebb_serial.py
# Serial connection utilities for EiBotBoard
# https://github.com/evil-mad/plotink
# 
# Intended to provide some common interfaces that can be used by 
# EggBot, WaterColorBot, AxiDraw, and similar machines.
#
# Version 0.1, Dated January 8, 2016.
#
#
# The MIT License (MIT)
# 
# Copyright (c) 2016 Evil Mad Scientist Laboratories
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import serial
import time #just for the EggDuino-patches
import inkex
import gettext

#AS5
#Global flag for differences between EggBots(1) and EggDunios(2) and so on. 0 by default.
EBBtype = 0
#AS4
#new feature introduced for Eggduino: check voltage as soon as possible after opening port (just give a user warning)
def checkVoltage( comPort):
	global EBBtype
	if (comPort is not None):
		mainVoltage = "OK"
		strVersion=query( comPort, 'QC\r')	#Voltage Query
		#extract the two voltages from telegram
		voltageI=int(strVersion.split(',')[0])
		voltageU=int(strVersion.split(',')[1])
		#inkex.errormsg(str(voltageI) + ' ' + str(voltageU))#DEBUG
		
		#2 Eggduino: first value can be ignored (just a dummy), second one equals about 725...750 with 9...12V at VIN
		#1 EiBotBoard: first value can be ignored, second one equals about 575-600 with 9...12V at VIN
		#0 if neither is detected, suppress this error message completely
		if ( ((voltageU < 700) and (EBBtype == 2)) or ((voltageU < 550) and (EBBtype == 1)) ):
			inkex.errormsg( gettext.gettext ( 'Insufficient power supply voltage.\r' + 'Please check mains adaptor.\r' ) )
			mainVoltage = None
		return mainVoltage

def version():
	return "0.1"	# Version number for this document

def findPort():	
	global EBBtype
	#Find a single EiBotBoard or Eggduino connected to a USB port.
	try:
		from serial.tools.list_ports import comports
	except ImportError:
		comports = None		
		return None
	#import inkex
	if comports:
		comPortsList = list(comports())
		EBBport = None
		EBBtype = 0
		#Alteration for EggDuino should either take a fixed port (OR must rely on a patched 16u2)
		#--> Take the easy way and be content with a (manually) fixed port number if an ARDUINO (clone) is detected:
		for port in comPortsList:
			if port[1].startswith("EiBotBoard"):  #1st search for original EiBotBoard
				EBBport = port[0] 	#Success; EBB found by name match.
				EBBtype = 1 #EggBot
				break	#stop searching-- we are done.
		if EBBport is None:
			#procede only if nothing was found yet
			for port in comPortsList:
				if port[2].startswith("USB VID:PID=04D8:FD92"):
					EBBport = port[0] #Success; EBB found by VID/PID match.
					EBBtype = 1	#EggBot			
					break	#stop searching-- we are done.				
		if EBBport is None:
			#if no EiBotBoard was found up to here search for an Eggduino
			for port in comPortsList:
				if port[1].startswith("Arduino Uno"):  #1st search for any Arduino
					#if port[2].startswith("USB VID:PID=2341:0043"): #2nd search for an Arduino Uno with 16u2 -> omit this search!
						if port[0].startswith("COM88"):	#3rd check whether this is found on COM-EIghty-EIght where you have manually assigned it to before! This is done to not connect to OTHER Arduinos that might be attached at the same time  
							EBBport = port[0] #Success; alternative EBB found by assignment match.
							EBBtype = 2 #Eggduino
							break	#stop searching-- we are done.	
		if EBBport is None:
			#if neither an EggBot (EiBotBoard) nor an Eggduino (Arduino Uno with 16u2) was found up to here take a last chance and search for an Arduino CLONE with a CH340G USB-chip
			for port in comPortsList:
				if port[1].startswith("USB-SERIAL CH340"):  #1st search for a cloned Arduino
					#if port[2].startswith("USB VID:PID=1A86:7523"): #2nd search for an Arduino Uno clone with CH340G -> omit this search!
						if port[0].startswith("COM88"):	#3rd check whether this is found on COM-EIghty-EIght where you have manually assigned it to before! This is done to not connect to OTHER Arduinos that might be attached at the same time  
							EBBport = port[0] #Success; alternative EBB found by assignment match.
							EBBtype = 2 #Eggduino
							break	#stop searching-- we are done.	
		#inkex.errormsg( 'Comport detected: ' + port[1] + ' on: ' + port[0] + ' Type: ' + str(EBBtype) ) #DEBUG
	return EBBport
			
def testPort( comPort ):
	'''
	Return a SerialPort object
	for the first port with an EBB (EiBotBoard; EggBot controller board).
	YOU are responsible for closing this serial port!
	'''		
	if comPort is not None:
		try:
			serialPort = serial.Serial( comPort, timeout=5 ) # 1->5 second timeout for very long actions!
			#Additions for EggDuino START
			serialPort.setRTS() #AS3
			serialPort.setDTR(False) #AS3
			time.sleep(1.75) # wait for Arduino to finish booting
			serialPort.flushInput()
			serialPort.flushOutput()
			time.sleep(0.1)
			#Additions for EggDuino END
			serialPort.write( 'v\r' )
			strVersion = serialPort.readline()			
			#doublecheck now if the found device really behaves like an Eggsomewhat
			if strVersion and strVersion.startswith( 'EBB' ):
				#do a first time check for sufficient motor supply (just throw a user warning)
				checkVoltage(serialPort)
				return serialPort
			#if not: shut down whatever we have found	
			closePort(comPort)
		except serial.SerialException:
			pass
		return None
	else:
		return None

def openPort():
	#inkex.errormsg( 'Init Comport now') #DEBUG
	foundPort = findPort() #search for active ports which much likely have an EggBot or an Eggduino connected
	serialPort = testPort( foundPort ) #open this port for further communication
	if serialPort:
		return serialPort
	return None
	
def closePort(comPort):
	if comPort is not None:
		try:
			comPort.close()
		except serial.SerialException:
			pass

def query( comPort, cmd ):
	if (comPort is not None) and (cmd is not None):
		response = ''
		try:
			comPort.write( cmd )
			response = comPort.readline()
			unused_response = comPort.readline() #read in extra blank/OK line
		except:
			inkex.errormsg( gettext.gettext( "Error reading serial data." ) )
		return response
	else:
		return None

def command( comPort, cmd ):
	if (comPort is not None) and (cmd is not None):
		try:
			comPort.write( cmd )
			response = comPort.readline()
			if ( response != 'OK\r\n' ):
				if ( response != '' ):
					inkex.errormsg( 'After command ' + cmd + ',' )
					inkex.errormsg( 'Received bad response from EBB: ' + str( response ) + '.' )
				else:
					inkex.errormsg( gettext.gettext( 'EBB Serial Timeout.') )
		except:
			pass 
			
	


