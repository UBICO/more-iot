import datetime
import logging

import asyncio

import aiocoap.resource as resource
import aiocoap

import glob
import os

import time

import json

import threading

import seaborn as sns
import matplotlib.pyplot as plt

import pandas as pd

class PutValue(resource.Resource):
    def __init__(self):
        super().__init__()
        self.timer_to_plot = 0
        self.data = pd.DataFrame()

    async def render_put(self, request):
        #print('PUT payload: %s' % request.payload)
        _j = ""
        _payload = request.payload

        #print(f"--- Payload - {len(request.payload)}")

        if type(_payload) == bytes:
            _payload = _payload.decode('UTF-8')    
        try:
            _j = json.loads(_payload)
        except:
            return aiocoap.Message(code=aiocoap.CHANGED, payload=b"{'code':'JSON Error'}")

        with open(f'datafiles/{_j["id"]}.csv','a+') as f:
            f.write(f'{datetime.datetime.now().timestamp()},{_j["value"]}\n')
            
        response = b'{"code":"OK"}'
        total = len(request.payload) + len(response) - len(_j['id'])
                
        self.data = self.data.append({'TIME': datetime.datetime.now().timestamp(), 'ID':_j['id'], 'VALUE':total}, ignore_index=True)

        self.plot_data()

        return aiocoap.Message(code=aiocoap.CHANGED, payload=response)

    def plot_data(self):
        fig,ax = plt.subplots(figsize=[20,10],nrows=1,ncols=2)

        if datetime.datetime.now().timestamp() - self.timer_to_plot > 5:
            print(self.data)
            print("-----")
            sns.boxplot(self.data, x='ID', y='VALUE', ax=ax[0])
            ax[0].set_xlabel("")
            ax[0].set_ylabel("Bytes")
            _df = self.data.groupby(['ID'], as_index=False).sum()
            print(_df)
            sns.barplot(_df, x='ID', y='VALUE', ax=ax[1])
            ax[1].set_xlabel("")
            ax[1].set_ylabel("Bytes")
            plt.savefig('charts/bytes.png')
            plt.close()
            self.timer_to_plot = datetime.datetime.now().timestamp()
            
        else:
            print(f"Timer is {self.timer_to_plot}, time now is {datetime.datetime.now().timestamp()}")
        print(self.data)


# logging setup
#logging.basicConfig(level=logging.INFO)
#logging.getLogger("coap-server").setLevel(logging.DEBUG)

async def main():
    # Resource tree creation
    root = resource.Site()

    root.add_resource(['.well-known', 'core'],
            resource.WKCResource(root.get_resources_as_linkheader))
    root.add_resource(['value'], PutValue())

    await aiocoap.Context.create_server_context(site=root, bind=("127.0.0.1",None))

    # Run forever
    await asyncio.get_running_loop().create_future()


if __name__ == "__main__":
    if len(glob.glob('datafiles')) < 1:
        os.mkdir('datafiles')
    asyncio.run(main())


