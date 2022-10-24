from posixpath import basename
import paho.mqtt.client as mqtt
import asyncio

import glob 

import time

import pandas as pd

import seaborn as sns
import matplotlib.pyplot as plt

import os

client = mqtt.Client()

def detect(f):
    df = pd.read_csv(f,names=['time','value'])
    df = df[-1000:].reset_index()
    print(len(df))
    old = -1
    try:
        for r in range(len(df)):
            print(r)
            df.loc[r,'value_prev'] = old
            old = df.iloc[r]['value']
    except Exception as e:
        print(e)
    max_value = df['value'].max()
    min_value = df['value'].min()

    df.loc[:,'var'] = df['value'] - df['value_prev']
    df.loc[:,'idx'] = df.index.tolist()
    df['var'] = df['var'] * (df['idx'] / len(df))
    df.loc[:,'above'] = abs(df['var']) > (abs(max_value) - abs(min_value)) * 0.1
    
    print(df)
    print(sum(df['above']))
    ratio = sum(df['above']/len(df))
    if ratio > 0.4:
        client.publish(f,'ANOMALY')
    else:
        client.publish(f,'NORMAL')

def plot(f):
    df = pd.read_csv(f,names=['time','value'])
    sns.lineplot(data=df,x='time',y='value')
    plt.savefig(f'charts/{basename(f).split(".")[0]}.png')
    
async def main():
    while True:
        for f in glob.glob('datafiles/*'):
            try:
                print(f"Trying {f}")
                detect(f)
            except Exception as e:
                print(f"Problem with file {f}")
                print(e)
        time.sleep(10)

if __name__ == "__main__":
    client.connect("localhost", 1883, 60)
    
    if len(glob.glob('charts')) == 0:
        os.mkdir('charts')

    asyncio.run(main())