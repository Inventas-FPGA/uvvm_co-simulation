from tinyrpc.protocols.jsonrpc import JSONRPCProtocol
from tinyrpc.transports.http import HttpPostClientTransport
from tinyrpc import RPCClient
import random
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
                                       "vvc_id": 0,
                                       "enable": True},
                               one_way=False)

    response = rpc_client.call(method="SetVvcListenEnable", args=None,
                               kwargs={"vvc_type": "AXISTREAM_VVC",
                                       "vvc_id": 1,
                                       "enable": True},
                               one_way=False)

    time.sleep(1.0)

    print("")
    print("Transmit with UART VVC")
    print("----------------------")
    response = rpc_client.call(method="TransmitBytes", args=None,
                               kwargs={"vvc_type": "UART_VVC",
                                       "vvc_id": 0,
                                       "data": [10, 20, 30, 40]},
                               one_way=False)
    print(f"response = {response}")

    time.sleep(10.0)

    print("")
    print("Receive with AXISTREAM VVC")
    print("----------------------")
    response = rpc_client.call(method="ReceiveBytes",
                               args=None,
                               kwargs={"vvc_type": "AXISTREAM_VVC",
                                       "vvc_id": 1,
                                       "num_bytes": 4,
                                       "exact_length": False},
                               one_way=False)
    print(f"response = {response}")

    print("")
    print("Transmit with AXISTREAM VVC")
    print("----------------------")
    response = rpc_client.call(method="TransmitBytes", args=None,
                               kwargs={"vvc_type": "AXISTREAM_VVC",
                                       "vvc_id": 0,
                                       "data": [9, 8, 7, 4, 1, 3, 0]},
                               one_way=False)
    print(f"response = {response}")

    time.sleep(10.0)

    print("")
    print("Receive with UART VVC")
    print("----------------------")
    response = rpc_client.call(method="ReceiveBytes",
                               args=None,
                               kwargs={"vvc_type": "UART_VVC",
                                       "vvc_id": 0,
                                       "num_bytes": 7,
                                       "exact_length": False},
                               one_way=False)
    print(f"response = {response}")



    NUM_ITERATIONS = 10
    NUM_BYTES = 200
    TIMEOUT_SECONDS = 120

    t0 = time.time()

    for it in range(NUM_ITERATIONS):
        data_xmit_uart2axis = list(random.randbytes(NUM_BYTES))
        data_xmit_axis2uart = list(random.randbytes(NUM_BYTES))

        response = rpc_client.call(method="TransmitBytes", args=None,
                               kwargs={"vvc_type": "UART_VVC",
                                       "vvc_id": 0,
                                       "data": data_xmit_uart2axis},
                               one_way=False)

        assert response["success"] == True, f"Iteration {it}: Failed to send (NUM_BYTES) on UART"

        response = rpc_client.call(method="TransmitBytes", args=None,
                               kwargs={"vvc_type": "AXISTREAM_VVC",
                                       "vvc_id": 0,
                                       "data": data_xmit_axis2uart},
                               one_way=False)

        assert response["success"] == True, f"Iteration {it}: Failed to send (NUM_BYTES) on AXI-STREAM"


        done = False
        timeout_counter = 0

        data_recv_uart2axis = None
        data_recv_axis2uart = None

        while not done and timeout_counter < TIMEOUT_SECONDS:
            time.sleep(1)
            timeout_counter = timeout_counter + 1

            response = rpc_client.call(method="ReceiveBytes",
                                       args=None,
                                       kwargs={"vvc_type": "AXISTREAM_VVC",
                                               "vvc_id": 1,
                                               "num_bytes": NUM_BYTES,
                                               "exact_length": True},
                                       one_way=False)

            if response["success"] and len(response["result"]["data"]) > 0:
                data_recv_uart2axis = response["result"]["data"]


            response = rpc_client.call(method="ReceiveBytes",
                                       args=None,
                                       kwargs={"vvc_type": "UART_VVC",
                                               "vvc_id": 0,
                                               "num_bytes": NUM_BYTES,
                                               "exact_length": True},
                                       one_way=False)

            if response["success"] and len(response["result"]["data"]) > 0:
                data_recv_axis2uart = response["result"]["data"]

            if data_recv_axis2uart is not None and data_recv_uart2axis is not None:
                done = True

        if not done:
            print(f"Failed to receive data after {TIMEOUT_SECONDS}")
            exit(0)

        assert len(data_recv_axis2uart) == len(data_xmit_axis2uart)
        assert len(data_recv_uart2axis) == len(data_xmit_uart2axis)

        mismatch = False

        for x in range(len(data_recv_axis2uart)):
            if data_recv_axis2uart[x] != data_xmit_axis2uart[x]:
                print(f"Iteration {it}: Mismatch: axis2uart byte {x} got {data_recv_axis2uart[x]}, expect {data_xmit_uart2axis[x]}")
                mismatch = True

        for x in range(len(data_recv_uart2axis)):
            if data_recv_uart2axis[x] != data_xmit_uart2axis[x]:
                print(f"Iteration {it}: Mismatch: uart2axis byte {x} got {data_recv_uart2axis[x]}, expect {data_xmit_uart2axis[x]}")
                mismatch = True

        if not mismatch:
            print(f"Iteration {it}: No mismatch")

    t1 = time.time()

    print(f"Elapsed time: {t1 - t0}")

    print("All done - terminating simulation")
    rpc_client.call(method="TerminateSim", args=None, kwargs=None)

if __name__ == '__main__':
    main()
