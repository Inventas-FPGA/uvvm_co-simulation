from tinyrpc.protocols.jsonrpc import JSONRPCProtocol
from tinyrpc.transports.http import HttpPostClientTransport
from tinyrpc import RPCClient
import time


def main():
    rpc_client = RPCClient(
        JSONRPCProtocol(),
        HttpPostClientTransport('http://localhost:8484/jsonrpc'))

    rpc_client.call(method="StartSim", args=None, kwargs=None)

    time.sleep(0.5)

    response = rpc_client.call(method="GetVvcList", args=None, kwargs=None)
    print(f"VVC list response: {response}")

    response = rpc_client.call(method="SetVvcListenEnable", args=None,
                               kwargs={"vvc_type": "UART_VVC",
                                       "vvc_id": 1,
                                       "enable": True},
                               one_way=False)

    response = rpc_client.call(method="SetVvcListenEnable", args=None,
                               kwargs={"vvc_type": "AXISTREAM_VVC",
                                       "vvc_id": 1,
                                       "enable": True},
                               one_way=False)

    time.sleep(0.5)

    response = rpc_client.call(method="TransmitBytes", args=None,
                               kwargs={"vvc_type": "UART_VVC",
                                       "vvc_id": 0,
                                       "data": [10, 20, 30, 40]},
                               one_way=False)
    print(f"response = {response}")

    time.sleep(1.0)

    response = rpc_client.call(method="ReceiveBytes",
                               args=None,
                               kwargs={"vvc_type": "UART_VVC",
                                       "vvc_id": 1,
                                       "length": 4,
                                       "all_or_nothing": False},
                               one_way=False)
    print(f"response = {response}")


if __name__ == '__main__':
    main()
