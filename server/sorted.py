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

    for port_prefix in ('COM','/dev/ttyUSB','/dev/ttyACM'):
        for port_num in range(0,20):
            port_name = port_prefix + str(port_num)
            if ((port_prefix == 'COM') or (os.path.exists(port_name))):
                ser.port = port_name
                try:
                    ser.open()
                except serial.serialutil.SerialException:
                    pass
                if ser.isOpen():
                    print "Opened port",ser.port
                    break;
        else:
            continue
        break

    if ser.isOpen():   
#        ser.setDTR(False)
#        time.sleep(2)
#        ser.setDTR(True)
        while(ser.inWaiting() > 0):
            junk = ser.read(1)

        ser.write("U=" + str(int(time.time())) + '\n')
        return True
    else:
        print "Couldnt open any port..."
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

fields = ['t1','h1','t2','h2','r1','r2','a0','a1','a2','a3','time','U','a4','a5']
current = {}


outbuff = ''
def get_reply():
    global outbuff
    try:
      while((ser.inWaiting() > 0)):
        while ser.inWaiting() > 0:
            c = ser.read(1)
            if c == "\n":
              outbuff = outbuff.replace("\n","") 
              if outbuff != '':
                        print "Got line: ",outbuff
#                if not ('=' in outbuff):
 #                   if outbuff !='':
                      #  thatar = outbuff.split(':')
                        if 'EV:' in outbuff:
                            write_ev(event_log,outbuff.replace('EV:',''))
                        if 'ST:' in outbuff:
                            write_log(climate_log,outbuff.replace('ST:',''))
                        write_all(outbuff)
#                for it in outbuff.split(","):
#                    if (it !='') and ('=' in it):
#                        print it;
#                        try:
#                            (key,val)=it.split('=')
#                            if key in fields:
#                                current[key] = val
#                            if key in ['W','R','U']:
#                                #print outbuff
#                                pass
#                        except ValueError:
#                            print "Erreur: recu" ,it
                        outbuff=''
                        break
            outbuff += c
        time.sleep(16/BAUD_RATE)
    except IOError as e:
        print "Reading", outbuff
        print "ERROR", "IO",e
        reset_port()
    except OSError as e:
        print "Reading", outbuff
        print "ERROR", "OS",e
        reset_port()


input=1


def write_csv(fname):
    
    if os.path.isfile(fname):
        fd = open(fname, "ab")
        cli = csv.writer(fd)
    else:
        fd = open(fname, "wb")
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

def write_ev(fname,outstr):
#    print "Writing", outstr,"To",fname
    
    if os.path.isfile(fname):
        fd = open(fname, "ab")
    else:
        fd = open(fname, "wb")
        fd.write("time,rules_result,type,p1,p2,p3\n")
    fd.write(outstr + "\n")
    fd.close()

def write_log(fname,outstr):
#    print "Writing", outstr,"To",fname
    
    if os.path.isfile(fname):
        fd = open(fname, "ab")
    else:
        fd = open(fname, "wb")
        fd.write("time,U,t1,h1,t2,h2,r1,r2,a0,a1,a2,a3,a4,a5,wt,ws,wi\n")
    fd.write(str(int(time.time())) + ',' + outstr + "\n")
    fd.close()


fd_all = open('allin.log', "wb")
def write_all(outstr):
   global fd_all
   fd_all.write(outstr)
   fd_all.write('\n')
#    print "Writing", outstr,"To",fname



starttime  = time.time() + 8
while 1 :
        get_reply()
        # send the character to the device
        if(starttime<time.time()):

            get_reply()
            ser.write("U=" + str(int(time.time())) + '\n')
            time.sleep(0.2)
            get_reply()
            time.sleep(0.2)
            get_reply()
            ser.write("P=3"+'\n')
            time.sleep(0.2)

            ser.write("D"+'\n')
            time.sleep(0.2)
            get_reply()
            get_reply()
            current = {}
#            ser.write("s" + '\n')
#            time.sleep(0.1)
#            get_reply()
#            ser.write("t" + '\n')
#            time.sleep(0.1)
#            get_reply()
#            for i in range(0, 6):
#               ser.write("a="+str(i) + '\n')
#               time.sleep(0.1)
#               get_reply()
#            # let's wait one second before reading output (let's give device time to answer)
#            time.sleep(0.1)
#            current['time']=int(starttime)
            print time.time() - starttime
            starttime += interval_secs
#            ser.write("U"+ '\n')
#            time.sleep(0.1)
#            get_reply()
#            ser.write("E"+ '\n')
#            time.sleep(0.1)
#            get_reply()
#            write_csv(filename)
#       # time.sleep(starttime-time.time())
        time.sleep(0.1)
        get_reply()


