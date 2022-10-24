import logging
import asyncio

import sys

from aiocoap import *

logging.basicConfig(level=logging.INFO)

async def put():
    context = await Context.create_client_context()

    payload = '{"id": "' + sys.argv[1] + '", "value": "200"}'
    payload = payload.encode("UTF-8")
    request = Message(code=PUT, payload=payload, uri="coap://127.0.0.1/value")

    response = await context.request(request).response

    print('Result: %s\n%r'%(response.code, response.payload))

if __name__ == "__main__":
    asyncio.run(put())
