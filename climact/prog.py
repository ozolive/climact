#!/usr/bin/python
import time
import serial
import time
import csv
import os.path
import struct

filename = 'climate_log_old.csv'
climate_log = 'climate_log.csv'
event_log = 'event_log.csv'

interval_secs = 180

BAUD_RATE=9600
#BAUD_RATE=115200

# configure the serial connections (the parameters differs on the device you are connecting to)
ser=None

def open_port():
    global ser
    ser = serial.Serial(
        port=None,
        baudrate=BAUD_RATE,
        parity=serial.PARITY_ODD,
        stopbits=serial.STOPBITS_TWO,
        bytesize=serial.SEVENBITS,
        timeout=1,
    )
    ser.port = '/dev/ttyACM12'
    try:
        ser.open()
    except serial.serialutil.SerialException:
        try:
            ser.port='/dev/ttyACM13'
            ser.open()
        except serial.serialutil.SerialException:
            pass
        pass

    if ser.isOpen():   
#        ser.setDTR(False)
#        time.sleep(2)
#        ser.setDTR(True)
        while(ser.inWaiting() > 0):
            junk = ser.read(1)
        return True
    else:
        return False


def reset_port():
    try:
        if ser.isOpen():
            ser.close()
    except:
        pass
    opened=False
    while not opened:
        opened = open_port()
        time.sleep(5)

    
open_port()

O_TEMP1 = 0
O_TEMP2 = 1
O_HUM1 = 2
O_HUM2 = 3
O_ROS1 = 4
O_ROS2 = 5
O_MOI1 = 6
O_MOI2 = 7
O_PWR = 8
O_NOW = 9



COND_LT = (1<<0) # Lower than
COND_BT = (1<<1) # Bigger than than
COND_AND = (1<<2) # Logical And        -- then both ops are  other conditions ?
COND_OR = (1<<3) # Logical Or
COND_T2 = (1<<4) # Is type 2 ?
COND_MOD = (1<<5) # Is modulo ?
COND_NOT = (1<<6) # Inverse conditin
COND_RV = (1<<7) # Right operand is a var


TYPE_LIGHT = 1 << 0
TYPE_WAIT  = 1 << 1   #// Device needs 30 min. wait between state change, e.g. HPS
TYPE_TEMP_UP  = 1 << 2   #// Device warms the place
TYPE_TEMP_DOWN =  1 << 3   #// Device cools the place
TYPE_HUM_UP  = 1 << 4   #// Device ups humidity level
TYPE_HUM_DOWN =  1 << 5   #// Device downs humidity level

rules_count=0;
start_adress = 128;
#relay_adress = 72;

def swap32(i):
    return struct.unpack("<I", struct.pack(">I", i))[0]

def make_relay(state,t,rule,lastchange):
   return "%02x%02x%02x%02x%08x" % (0,state,t,rule,swap32(lastchange))

def write_relay(relay):
   global relay_adress
   ser.write("W=%03x%s" % (relay_adress, relay)+ '\n')
   print "WR=%03x%s" % (relay_adress, relay)+ '\n'
   relay_adress += len(relay)/2
   print len(relay)/2
   time.sleep(0.1)
   #get_reply()


def make_rule(flags,left, right, mod=0):
   global rules_count
   sf = "%02x%02x%08x%08x" % (flags,left,swap32(mod),swap32(right))
   rules_count += 1;
   return sf

def write_rule(rule):
   global start_adress
   ser.write("W=%03x%s" % (start_adress, rule)+ '\n')
   print "WR=%03x%s" % (start_adress, rule)+ '\n'
   start_adress += len(rule)/2
   time.sleep(0.5)
   #get_reply()
   #get_reply()

def write_rules():
    #time.sleep(5)
    write_rule(make_rule(COND_BT|COND_MOD,O_NOW,3600*8,86400)) # 0
    write_rule(make_rule(COND_LT|COND_MOD,O_NOW,3600*20,86400)) # 1
    write_rule(make_rule(COND_AND,0,1))               # 2
    write_rule(make_rule(COND_BT|COND_MOD,O_NOW,0,3600*2)) # 3
    write_rule(make_rule(COND_LT|COND_MOD,O_NOW,2000,3600*2)) # 4
    write_rule(make_rule(COND_AND,3,4))               # 5
    write_rule(make_rule(COND_NOT|COND_AND,3,4))               # 6
    write_rule(make_rule(COND_AND,2,5))               # 7
    write_rule(make_rule(COND_AND,2,6))               # 8
#E=1111100100000000
    write_rule(make_rule(COND_AND|COND_NOT,0,1))               # 9
    write_rule(make_rule(COND_AND,2,9))               # 10
    for i in range(0, 5):
        write_rule(make_rule(0,0,0))

    print "Wrote ",rules_count,"rules."


def blank_rules():
    time.sleep(5)
    for i in range(0, 8):
       write_rule('FFFFFFFFFFFF')


def read_rules():
    ser.write("R=080"+ '\n')
    time.sleep(0.1)
    ser.write("R=080"+ '\n')
    time.sleep(0.1)
    ser.write("R=0a0"+ '\n')
    time.sleep(0.1)
    ser.write("R=040"+ '\n')
    time.sleep(0.1)
    ser.write("R=060"+ '\n')
    time.sleep(0.1)

time.sleep(0.2)
write_rules()
time.sleep(0.2)
relay_adress = 64;
write_relay(make_relay(0,TYPE_LIGHT|TYPE_TEMP_UP,7,0))
time.sleep(1.2)
write_relay(make_relay(0,TYPE_LIGHT|TYPE_TEMP_UP,8,0))
time.sleep(0.2)

ser.write("P=8" + '\n')
