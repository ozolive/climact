#!/usr/bin/python
import time
import serial
import time
import csv
import os.path
import struct

filename = 'climate_log.csv'

interval_secs = 18

BAUD_RATE=9600
#BAUD_RATE=115200

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
    port=None,
    baudrate=BAUD_RATE,
    parity=serial.PARITY_ODD,
    stopbits=serial.STOPBITS_TWO,
    bytesize=serial.SEVENBITS,
    timeout=1,
)
ser.port = '/dev/ttyUSB0'
ser.open()
ser.setDTR(False)
while(ser.inWaiting() > 0):
    junk = ser.read(1)
#ser.isOpen()

#print 'Enter your commands below.\r\nInsert "exit" to leave the application.'


fields = ['t1','h1','t2','h2','r1','r2','a0','a1','a2','a3','time','U']
current = {}


outbuff = ''
def get_reply():
    global outbuff
    while((ser.inWaiting() > 0)):
        while ser.inWaiting() > 0:
            c = ser.read(1)
            if c == "\n":
              if outbuff != '':
                #print "Got line: ",outbuff
                if not ('=' in outbuff):
                    if outbuff !='':
                       print outbuff
                for it in outbuff.split(","):
                    if (it !='') and ('=' in it):
                        print it;
                        try:
                            (key,val)=it.split('=')
                            if key in fields:
                                current[key] = val
                            if key in ['W','R','U']:
                                #print outbuff
                                pass
                        except ValueError:
                            print "Erreur: recu" ,it
                outbuff=''
                break
            outbuff += c
        time.sleep(16/BAUD_RATE)


input=1


def write_csv(fname):
    
    if os.path.isfile(fname):
        fd = open("climate_log.csv", "ab")
        cli = csv.writer(fd)
    else:
        fd = open("climate_log.csv", "wb")
        cli = csv.writer(fd)
        cli.writerow(fields)

    out = []
    for field in fields:
        if field in current:
            out.append(current[field])
        else:
            out.append(None)

    cli.writerow(out)
    fd.close()




O_TEMP1 = 0
O_TEMP2 = 1
O_HUM1 = 2
O_HUM2 =  3
O_ROS1 =  4
O_ROS2 =  5
O_MOI1 =  6
O_MOI2 = 7
O_PWR = 8
O_NOW = 9



COND_LT = (1<<0) # Lower than
COND_BT = (1<<1) # Bigger than than
COND_AND = (1<<2) # Logical And        -- then both ops are  other conditions ?
COND_OR = (1<<3) # Logical Or
COND_T2 = (1<<4) # Is type 2 ?
COND_MOD = (1<<4) # Is modulo ?
COND_LV = (1<<6) # Left operand is a var (est-ce toujours le cas ?)
COND_RV = (1<<7) # Right operand is a var


rules_count=0;
start_adress = 128;


def swap32(i):
    return struct.unpack("<I", struct.pack(">I", i))[0]

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
   time.sleep(0.1)
   get_reply()
   get_reply()

def write_rules():
    time.sleep(5)
    write_rule(make_rule(COND_BT|COND_MOD,O_NOW,2,8))
    write_rule(make_rule(COND_LT|COND_MOD,O_NOW,5,8))
    write_rule(make_rule(COND_AND,0,1))

    for i in range(0, 5):
        write_rule(make_rule(0,0,0))

    print "Wrote ",rules_count,"rules."

def blank_rules():
    time.sleep(5)
    for i in range(0, 8):
       write_rule('FFFFFFFFFFFF')


#ser.write("W=080AF0AF1ABI47C"+ '\n')
#time.sleep(0.5)
#get_reply()

def read_rules():
    ser.write("R=080"+ '\n')
    time.sleep(0.5)
    get_reply()
    ser.write("R=080"+ '\n')
    time.sleep(0.5)
    get_reply()
    ser.write("R=0a0"+ '\n')
    time.sleep(0.5)
    get_reply()

time.sleep(5)
get_reply()
ser.write("U=" + str(int(time.time())) + '\n')
time.sleep(0.2)
write_rules()
#blank_rules()
read_rules()
get_reply()



starttime  = time.time() + 8
while 1 :
        get_reply()
        # send the character to the device
        if(starttime<time.time()):
            current = {}
            ser.write("s" + '\n')
            time.sleep(0.1)
            get_reply()
            ser.write("t" + '\n')
            time.sleep(0.1)
            get_reply()
            for i in range(0, 6):
               ser.write("a="+str(i) + '\n')
               time.sleep(0.1)
               get_reply()
            # let's wait one second before reading output (let's give device time to answer)
            time.sleep(0.1)
            current['time']=int(starttime)
            print time.time() - starttime
            starttime += interval_secs
            ser.write("U"+ '\n')
            time.sleep(0.1)
            get_reply()
            ser.write("E"+ '\n')
            time.sleep(0.1)
            get_reply()
            write_csv(filename)
       # time.sleep(starttime-time.time())
        time.sleep(0.1)
        get_reply()


