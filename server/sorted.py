#!/usr/bin/python
import time
import serial
import time
import csv
import os.path

filename = 'climate_log.csv'

interval_secs = 60

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


fields = ['t1','h1','t2','h2','r1','r2','a0','a1','time','U']
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
                        (key,val)=it.split('=')
		        if key in fields:
			    current[key] = val
                        if key in ['W','R','U']:
                            print outbuff
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



#time.sleep(5)

#ser.write("W=080AF0AF1ABI47C"+ '\n')
#time.sleep(0.5)
#get_reply()
#ser.write("R=080"+ '\n')
time.sleep(5.5)
#get_reply()

ser.write("U=" + str(int(time.time())) + '\n')
time.sleep(0.2)
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
            write_csv(filename)
       # time.sleep(starttime-time.time())
        time.sleep(0.1)
        get_reply()


