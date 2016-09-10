#!/usr/bin/python
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
                    if (key == "LEQUEL") or (key == "LAQUELLE"):
                        key = "LEQUEL " + last_key
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
avars = ['a0','a1','a2','a3']


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
    "layout": Layout(title="hello world", xaxis=dict(
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
        type='date'  
    ))
})


pdata=[]
hdata=[]
adata=[]

for var in tvars:
    adata.append(Scatter(x=x, y=trace_var(var)))
#time_plot(pdata)

for var in hvars:
    adata.append(Scatter(x=x, y=trace_var(var)))
#time_plot(hdata)

for var in avars:
    adata.append(Scatter(x=x, y=trace_var(var,0.1)))
time_plot(adata)


