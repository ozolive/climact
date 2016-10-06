#!/usr/bin/python
# coding: utf-8

import plotly
from plotly.graph_objs import Scatter, Layout
import plotly.plotly as py
import plotly.graph_objs as go

import datetime
import csv
import math

lasttime=0
firsttime=0

interval_min = 180

def dt(u): return datetime.datetime.utcfromtimestamp(u)


csvfiles = [
#"climate_log.csv.1",
#"climate_log.csv.3",
#"climate_log.csv.35",
#"climate_log.csv.4",
#"climate_log.csv.5",
#"climate_log.csv.6",
"climate_log.csv",
]


bytime = {}
data = []
times = []
x = []
inserted = 0

def compute_vpd(temp,rh):
    KELVIN_0=273.15
    VPS_A=-1.88e4
    VPS_B=-13.1
    VPS_C=-1.5e-2
    VPS_D=8e-7
    VPS_E=-1.69e-11
    VPS_F=6.456
    
    VPS_A=-6096.9385
    VPS_B=21.2409642
    VPS_C=-2.711193e-2
    VPS_D=1.673952e-5
    VPS_E=0
    VPS_F=2.433502
 
    temp += KELVIN_0

    vps = math.e**((VPS_A/temp) + VPS_B + VPS_C*temp + VPS_D*temp*temp + VPS_E*temp*temp*temp + VPS_F*math.log(temp))
    vpd = vps - ((vps * rh) / 100.0)
    print temp, rh, vps, vpd
    return vpd


def insert_cold(cold):
    global firsttime,lasttime,data,bytime,times,x,inserted
    it_time = int(float(cold['time']))
    bytime[it_time]=cold
    times.append(it_time)
    x.append(dt(it_time))

    curtime=long(float(cold['time'])) * 1000
    if firsttime==0:
        firsttime=long(float(cold['time'])) * 1000
    else:
        if long(float(cold['time'])) * 1000 < firsttime:
            firsttime=long(float(cold['time'])) * 1000
    
    if lasttime==0:
        lasttime=long(float(cold['time'])) * 1000
    else:
        if long(float(cold['time'])) * 1000 > lasttime:
            lasttime=long(float(cold['time'])) * 1000

    inserted += 1


def read_data(csvfile):
        cr = csv.reader(open(csvfile,"rb"))
	rownum = 0
#	data = []
#	times = []
#	x = []
#        bytime = {}
        last_time = 0
        last_cold = {}
        print interval_min
        import pudb
#        pudb.set_trace()
	for row in cr:
	    if rownum == 0:
		header_tmp = row
                header = []
                for key in header_tmp:
                    header.append(key)
                    last_key = key
	    else:
		colnum = 0
		cold = {}
		for col in row:
                    try:
		        cold[header[colnum]] = col
		        colnum += 1
                    except IndexError as e:
                        print "IndexError on file", filename, "at row",rownum,':',e
                        pass
		data.append(cold)
                if 'time' in cold and cold['time']!= '' and cold['time']!= '' and int(float(cold['time'])):
                    it_time = int(float(cold['time']))
                    if last_time > 0:
                        if it_time - last_time > interval_min:
                            if not last_time in times:
                                insert_cold(last_cold)
                    if it_time - (lasttime/1000) > interval_min:
                        insert_cold(cold)

                    last_time = it_time
                    last_cold = cold

	#    print repr(cold)
	    rownum += 1
	#return (header,data,bytime,times,x)
	return


#(header,data,bytime,times,x)=read_data('climate_log.csv')
#print header

for filename in csvfiles:
    read_data(filename)

print "Considered",inserted,"rows."

tvars = ['t1','t2','r1','r2']
hvars = ['h1','h2']
avars = ['a1','a2','a3']

leg_vars = {
  't1': {
     'name': 'Inside temp (C)',
     'legendgroup' : 'inside',
   },
  'h1': {
     'name': 'Inside Rel. Humidity (%)',
     'legendgroup' : 'inside',
   },
  'r1': {
     'name': 'Point de rosée intérieur',
     'legendgroup' : 'inside',
   },
  't2': {
     'name': 'Outside temp (C)',
     'legendgroup' : 'outside',
   },
  'h2': {
     'name': 'Outside Rel. Humidity (%)',
     'legendgroup' : 'outside',
   },
  'r2': {
     'name': 'Point de rosée extérieur',
     'legendgroup' : 'outside',
   },
  'a0': {
     'name': 'Secheresse 0',
     'legendgroup' : 'moisture',
   },

  'a1': {
     'name': 'Secheresse 1',
     'legendgroup' : 'moisture',
   },
  'a2': {
     'name': 'Secheresse 2',
     'legendgroup' : 'moisture',
   },
  'a3': {
     'name': 'Secheresse 3',
     'legendgroup' : 'moisture',
   },
  'a4': {
     'name': 'Ampérage',
     'legendgroup' : 'elec',
   },
  'wt': {
     'name': 'Cumulated watering seconds',
     'legendgroup' : 'elec',
   },
  'temp_diff': {
     'name': 'IN - OUT Temperature difference.',
     'unit': 'C',
     'legendgroup' : 'anal',
   },
  'rosee_diff': {
     'name': 'IN - OUT Rosée. diff. Plant activity estimate.',
     'unit': 'C',
     'legendgroup' : 'anal',
   },
  'water_degrees_in': {
     'name': 'Degrees over Condensation temp IN.',
     'unit': 'C',
     'legendgroup' : 'anal',
   },
  'water_degrees_out': {
     'name': 'Degrees over Condensation temp OUT',
     'unit': 'C',
     'legendgroup' : 'anal',
   },
  'plant_signal': {
     'name': 'Degree diff diff. Main signal..',
     'unit': 'C',
     'legendgroup' : 'anal',
   },
  'vpd1': {
     'name': 'VPD IN',
     'unit': 'Pa',
     'legendgroup' : 'anal',
   },
  'vpd2': {
     'name': 'VPD OUT',
     'unit': 'Pa',
     'legendgroup' : 'anal',
   },







}

def trace_var(var,coeff=1.0):
    y=[]
    for t in times:
        if var in bytime[t] and bytime[t][var] and float(bytime[t][var]) >0.8:
            value = float(bytime[t][var]) * float(coeff)
            if value > 100 or value < -50:
                print "Overflow on ",var,"at",t,dt(t)
                y.append(None)
            else:
                y.append(value)
        else:
            y.append(None)
    return y

print firsttime, lasttime

def time_plot(data):
  plotly.offline.plot({
    "data": data,
    "layout": Layout(title="Climact Viewer", xaxis=dict(
    range=[firsttime,lasttime],
    rangeselector=dict(
            buttons=list([
        dict(count=1,
                     label='1d',
                     step='day',
                     stepmode='backward'),

        dict(count=1,
                     label='1w',
                     step='week',
                     stepmode='backward'),

                dict(count=1,
                     label='1m',
                     step='month',
                     stepmode='backward'),
                dict(count=6,
                     label='6m',
                     step='month',
                     stepmode='backward'),
                dict(count=1,
                    label='YTD',
                    step='year',
                    stepmode='todate'),
                dict(count=1,
                    label='1y',
                    step='year',
                    stepmode='backward'),
                dict(step='all')
            ])
        ),
        rangeslider=dict(),
        type='date',
        title='Time',  
    ),
    yaxis=dict(
    title='Celcius / Kelvin / % RH / % soil dryness'
      )
    )
})


pdata=[]
hdata=[]
adata=[]

for var in tvars:
    adata.append(Scatter(x=x, y=trace_var(var), legendgroup=leg_vars[var]['legendgroup'], name= leg_vars[var]['name']))
#time_plot(pdata)

for var in hvars:
    #adata.append(Scatter(x=x, y=trace_var(var)))
    adata.append(Scatter(x=x, y=trace_var(var), legendgroup=leg_vars[var]['legendgroup'], name= leg_vars[var]['name']))
#time_plot(hdata)

for var in avars:
#    adata.append(Scatter(x=x, y=trace_var(var,0.1)))
    adata.append(Scatter(x=x, y=trace_var(var,0.1), legendgroup=leg_vars[var]['legendgroup'], name= leg_vars[var]['name']))

def trace_diff(name,var1,var2,coeff=1):
    y=[]
    for t in times:
        if var1 in bytime[t] and bytime[t][var1] and var2 in bytime[t] and bytime[t][var2] and (float(bytime[t][var2]) >0.01 or float(bytime[t][var2]) <0.01):
            bytime[t][name] = (float(bytime[t][var1]) - (float(bytime[t][var2])))
            bytime[t][name] = bytime[t][name] * float(coeff)
            if bytime[t][name] > 100 or bytime[t][name] < -50:
                print "Diff Overflow",bytime[t][name]," on ",name,"at",t,dt(t)
                y.append(None)
            else:
                y.append(bytime[t][name])

        else:
            y.append(None)
    return y

def trace_vpd(name,var1,var2,coeff=1):
    y=[]
    for t in times:
        if var1 in bytime[t] and bytime[t][var1] and var2 in bytime[t] and bytime[t][var2] and (float(bytime[t][var2]) >0.01 or float(bytime[t][var2]) <0.01):
            bytime[t][name] = compute_vpd(float(bytime[t][var1]), float(bytime[t][var2]))
            bytime[t][name] = bytime[t][name] * float(coeff)
            if bytime[t][name] > 100 or bytime[t][name] < -50:
                print "Diff Overflow",bytime[t][name]," on ",name,"at",t,dt(t)
                y.append(None)
            else:
                y.append(bytime[t][name])

        else:
            y.append(None)
    return y





adata.append(Scatter(x=x, y=trace_diff('temp_diff','t1','t2',1), legendgroup=leg_vars['temp_diff']['legendgroup'], name= leg_vars['temp_diff']['name']))
adata.append(Scatter(x=x, y=trace_diff('rosee_diff','r1','r2',1), legendgroup=leg_vars['rosee_diff']['legendgroup'], name= leg_vars['rosee_diff']['name']))
adata.append(Scatter(x=x, y=trace_diff('water_degrees_in','t1','r1',1), legendgroup=leg_vars['water_degrees_in']['legendgroup'], name= leg_vars['water_degrees_in']['name']))
adata.append(Scatter(x=x, y=trace_diff('water_degrees_out','t2','r2',1), legendgroup=leg_vars['water_degrees_out']['legendgroup'], name= leg_vars['water_degrees_out']['name']))


adata.append(Scatter(x=x, y=trace_diff('plant_signal','water_degrees_in','water_degrees_out',5), legendgroup=leg_vars['plant_signal']['legendgroup'], name= leg_vars['plant_signal']['name']))
adata.append(Scatter(x=x, y=trace_vpd('vpd1','t1','h1',10), legendgroup=leg_vars['vpd1']['legendgroup'], name= leg_vars['vpd1']['name']))
adata.append(Scatter(x=x, y=trace_vpd('vpd2','t2','h2',10), legendgroup=leg_vars['vpd2']['legendgroup'], name= leg_vars['vpd2']['name']))


time_plot(adata)
