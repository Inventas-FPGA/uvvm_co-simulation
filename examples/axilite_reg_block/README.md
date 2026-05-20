# axilite_reg_block

A small AXI-Lite register-block example exercising the AXI-Lite VVC, both
stand-alone and via the cosim layer.

The DUT (`src/rtl/axilite_reg_block.vhd`) is an AXI-Lite slave with 9
read/write 32-bit scratch registers (offsets 0x00..0x20) plus one read-only
ID register at 0x24 returning `0xCAFEC0DE`.

## Stand-alone testbench

From `examples/axilite_reg_block/run/`:

```
python3 ../scripts/run.py -tc TC_AXILITE_WRITE_READ
python3 ../scripts/run.py -tc TC_AXILITE_RANDOM
```

## Cosim

Terminal A:

```
python3 ../scripts/run.py -tc TC_COSIM
```

Terminal B:

```
python3 ../scripts/axilite_cosim_test.py
```
