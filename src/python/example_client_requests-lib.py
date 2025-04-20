# Short example of how the requests library can be
# used to send requests to the cosim server.

import itertools
import requests
import time


def main():
    url = "http://localhost:8484/jsonrpc"

    id = itertools.count(start=0, step=1)

    payload = {
        "method": "StartSim",
        "params": [],
        "jsonrpc": "2.0",
        "id": next(id),
    }
    requests.post(url, json=payload).json()

    time.sleep(0.5)

    payload = {
        "method": "GetVvcList",
        "params": [],
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"VVC list response: {response}")

    print("")
    print("Enable listening on UART_VVC 1 and AXISTREAM_VVC 1+3")
    payload = {
        "method": "SetVvcListenEnable",
        "params": {"vvc_type": "UART_VVC",
                   "vvc_id": 1,
                   "enable": True},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")

    payload = {
        "method": "SetVvcListenEnable",
        "params": {"vvc_type": "AXISTREAM_VVC",
                   "vvc_id": 1,
                   "enable": True},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    requests.post(url, json=payload).json()

    payload = {
        "method": "SetVvcListenEnable",
        "params": {"vvc_type": "AXISTREAM_VVC",
                   "vvc_id": 3,
                   "enable": True},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    requests.post(url, json=payload).json()

    time.sleep(0.5)

    ###########################################################################
    # Transmit and receive some bytes on UART VVCs 0 and 1
    ###########################################################################

    payload = {
        "method": "TransmitBytes",
        "params": {"vvc_type": "UART_VVC",
                   "vvc_id": 0,
                   "data": [10, 20, 30, 40, 50]},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")

    time.sleep(1.0)

    payload = {
        "method": "ReceiveBytes",
        "params": {"vvc_type": "UART_VVC",
                   "vvc_id": 1,
                   "num_bytes": 5,
                   "exact_length": False},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")

    ###########################################################################
    # Transmit and receive some bytes on AXISTREAM VVCs 0 and 1
    ###########################################################################

    payload = {
        "method": "TransmitBytes",
        "params": {"vvc_type": "AXISTREAM_VVC",
                   "vvc_id": 0,
                   "data": [10, 20, 30, 40, 50]},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")

    time.sleep(1.0)

    payload = {
        "method": "ReceiveBytes",
        "params": {"vvc_type": "AXISTREAM_VVC",
                   "vvc_id": 1,
                   "num_bytes": 5,
                   "exact_length": False},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")

    ###########################################################################
    # Transmit and receive a packet on AXISTREAM VVCs 2 and 3
    ###########################################################################

    payload = {
        "method": "TransmitPacket",
        "params": {"vvc_type": "AXISTREAM_VVC",
                   "vvc_id": 2,
                   "data": [11, 21, 31, 41, 51]},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")

    time.sleep(1.0)

    payload = {
        "method": "ReceivePacket",
        "params": {"vvc_type": "AXISTREAM_VVC",
                   "vvc_id": 3},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")

    payload = {
        "method": "TerminateSim",
        "params": [],
        "jsonrpc": "2.0",
        "id": next(id),
    }
    requests.post(url, json=payload).json()


if __name__ == "__main__":
    main()
