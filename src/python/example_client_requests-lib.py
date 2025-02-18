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
    print("Enable listening on UART_VVC 1 and AXISTREAM_VVC 1")
    payload = {
        "method": "SetVvcListenEnable",
        "params": {"vvc_type": "UART_VVC",
                   "vvc_id": 1,
                   "enable": True},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    requests.post(url, json=payload).json()

    payload = {
        "method": "SetVvcListenEnable",
        "params": {"vvc_type": "UART_VVC",
                   "vvc_id": 1,
                   "enable": True},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    requests.post(url, json=payload).json()

    time.sleep(0.5)

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
                   "length": 5,
                   "all_or_nothing": False},
        "jsonrpc": "2.0",
        "id": next(id),
    }
    response = requests.post(url, json=payload).json()
    print(f"request = {payload}")
    print(f"response = {response}")


if __name__ == "__main__":
    main()
