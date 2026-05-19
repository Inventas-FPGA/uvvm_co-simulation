from tinyrpc.protocols.jsonrpc import JSONRPCProtocol
from tinyrpc.transports.http import HttpPostClientTransport
from tinyrpc import RPCClient
import random
import time

VVC_TYPE = "AXILITE_VVC"
VVC_ID   = 0

NUM_SCRATCH = 9
ID_OFFSET   = 0x24
ID_VALUE    = 0xCAFEC0DE

TIMEOUT_SECONDS = 5


def write_reg(rpc, addr, value):
    """Write a 32-bit value to a register (little-endian bytes)."""
    data = [(value >> (8*i)) & 0xFF for i in range(4)]

    response = rpc.call(method="SendWriteRequest", args=None,
                        kwargs={"vvc_type": VVC_TYPE,
                                "vvc_id": VVC_ID,
                                "addr": addr,
                                "data": data,
                                "addr_increment": False},
                        one_way=False)
    assert response["success"], f"SendWriteRequest failed: {response}"

    # Poll until the write completes
    deadline = time.time() + TIMEOUT_SECONDS
    while time.time() < deadline:
        response = rpc.call(method="GetWriteResponse", args=None,
                            kwargs={"vvc_type": VVC_TYPE, "vvc_id": VVC_ID},
                            one_way=False)
        if response["success"] and response["result"].get("available"):
            return
        time.sleep(0.01)
    raise TimeoutError(f"Write to 0x{addr:08X} did not complete in {TIMEOUT_SECONDS}s")


def read_reg(rpc, addr):
    """Read a 32-bit value from a register. Returns the value as int."""
    response = rpc.call(method="SendReadRequest", args=None,
                        kwargs={"vvc_type": VVC_TYPE,
                                "vvc_id": VVC_ID,
                                "addr": addr,
                                "num_bytes": 4},
                        one_way=False)
    assert response["success"], f"SendReadRequest failed: {response}"

    deadline = time.time() + TIMEOUT_SECONDS
    while time.time() < deadline:
        response = rpc.call(method="GetReadResponse", args=None,
                            kwargs={"vvc_type": VVC_TYPE, "vvc_id": VVC_ID},
                            one_way=False)
        if response["success"]:
            data = response["result"].get("data", [])
            if len(data) == 4:
                return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)
        time.sleep(0.01)
    raise TimeoutError(f"Read from 0x{addr:08X} did not complete in {TIMEOUT_SECONDS}s")


def main():
    rpc = RPCClient(
        JSONRPCProtocol(),
        HttpPostClientTransport('http://localhost:8484/jsonrpc'))

    rpc.call(method="StartSim", args=None, kwargs=None)
    time.sleep(0.5)

    response = rpc.call(method="GetVvcList", args=None, kwargs=None)
    print(f"VVC list response: {response}")

    failures = 0

    # 1) Check the ID register.
    id_val = read_reg(rpc, ID_OFFSET)
    print(f"ID register = 0x{id_val:08X} (expect 0x{ID_VALUE:08X})")
    if id_val != ID_VALUE:
        failures += 1
        print("  MISMATCH!")

    # 2) Walk the 9 scratch registers: write a known pattern, read back.
    for idx in range(NUM_SCRATCH):
        addr = idx * 4
        value = 0xDEAD0000 | idx
        write_reg(rpc, addr, value)
        readback = read_reg(rpc, addr)
        if readback != value:
            failures += 1
            print(f"  Scratch reg {idx} mismatch: wrote 0x{value:08X}, read 0x{readback:08X}")
        else:
            print(f"  Scratch reg {idx}: wrote 0x{value:08X}, read 0x{readback:08X} OK")

    # 3) Random write/read iterations against a host-side shadow.
    NUM_ITERATIONS = 100
    shadow = [0] * NUM_SCRATCH
    # Seed the shadow from what we just wrote
    for idx in range(NUM_SCRATCH):
        shadow[idx] = 0xDEAD0000 | idx

    for it in range(NUM_ITERATIONS):
        wr_idx = random.randint(0, NUM_SCRATCH - 1)
        wr_val = random.randint(0, 0xFFFFFFFF)
        write_reg(rpc, wr_idx * 4, wr_val)
        shadow[wr_idx] = wr_val

        rd_idx = random.randint(0, NUM_SCRATCH - 1)
        rd_val = read_reg(rpc, rd_idx * 4)
        if rd_val != shadow[rd_idx]:
            failures += 1
            print(f"Iter {it}: reg {rd_idx} mismatch: expect 0x{shadow[rd_idx]:08X}, "
                  f"got 0x{rd_val:08X}")

    # 4) Verify the ID register survived all the noise.
    id_val = read_reg(rpc, ID_OFFSET)
    if id_val != ID_VALUE:
        failures += 1
        print(f"ID register changed after random traffic: 0x{id_val:08X}")

    if failures == 0:
        print(f"\nAll {1 + NUM_SCRATCH + NUM_ITERATIONS + 1} checks passed.")
    else:
        print(f"\n{failures} failures.")

    print("Terminating simulation")
    rpc.call(method="TerminateSim", args=None, kwargs=None)


if __name__ == '__main__':
    main()
