#!/usr/bin/python
# coding: utf-8

import plotly
from plotly.graph_objs import Scatter, Layout
import plotly.plotly as py
import plotly.graph_objs as go

import datetime
import csv

lasttime=0
firsttime=0

def dt(u): return datetime.datetime.utcfromtimestamp(u)

def read_data(csvfile):
        global firsttime,lasttime
        cr = csv.reader(open(csvfile,"rb"))
	rownum = 0
	data = []
	times = []
	x = []
        bytime = {}
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
		    cold[header[colnum]] = col
		    colnum += 1
		data.append(cold)
                if 'time' in cold and int(cold['time']):
                    bytime[int(cold['time'])]=cold
                    times.append(int(cold['time']))
                    x.append(dt(float(cold['time'])))
                    if firsttime==0:
                        firsttime=long(cold['time']) * 1000
                    lasttime=long(cold['time']) * 1000

	#    print repr(cold)
	    rownum += 1
	return (header,data,bytime,times,x)

(header,data,bytime,times,x)=read_data('climate_log.csv')
print header



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
     'name': 'Degrees over Condensation temp IN',
     'unit': 'C',
     'legendgroup' : 'anal',
   },
  'plant_signal': {
     'name': 'Degree diff diff. Main signal..',
     'unit': 'C',
     'legendgroup' : 'anal',
   },




}

def trace_var(var,coeff=1.0):
    y=[]
    for t in times:
        if var in bytime[t] and bytime[t][var] and float(bytime[t][var]) >0.8:
            y.append(float(bytime[t][var]) * float(coeff))
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
            y.append(bytime[t][name])

        else:
            y.append(None)
    return y



adata.append(Scatter(x=x, y=trace_diff('temp_diff','t1','t2',1), legendgroup=leg_vars['temp_diff']['legendgroup'], name= leg_vars['temp_diff']['name']))
adata.append(Scatter(x=x, y=trace_diff('rosee_diff','r1','r2',1), legendgroup=leg_vars['rosee_diff']['legendgroup'], name= leg_vars['rosee_diff']['name']))
adata.append(Scatter(x=x, y=trace_diff('water_degrees_in','t1','r1',1), legendgroup=leg_vars['water_degrees_in']['legendgroup'], name= leg_vars['water_degrees_in']['name']))
adata.append(Scatter(x=x, y=trace_diff('water_degrees_out','t2','r2',1), legendgroup=leg_vars['water_degrees_out']['legendgroup'], name= leg_vars['water_degrees_out']['name']))


adata.append(Scatter(x=x, y=trace_diff('plant_signal','water_degrees_in','water_degrees_out',10), legendgroup=leg_vars['plant_signal']['legendgroup'], name= leg_vars['plant_signal']['name']))


time_plot(adata)
