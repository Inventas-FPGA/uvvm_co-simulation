import pathlib
import sys

uart_path = pathlib.Path(__file__).resolve().parent.parent
repo_path = pathlib.Path(__file__).resolve().parent.parent.parent.parent

print(f"uart_path: {uart_path}")
print(f"repo_path: {repo_path}")

# Path to HDLRegression:
sys.path.append(str(repo_path / "thirdparty" / "hdlregression"))
sys.path.append('../../../thirdparty/hdlregression') # Just to make LSP server understand path

from hdlregression import HDLRegression

def init(hr):
    # The command below can be used to compile all of UVVM, it is not necessary
    # to compile individual libraries as we have done below (it is a bit faster
    # to online compile the libraries that are actually used though).

    # hr.compile_uvvm("../../../thirdparty/uvvm/")

    # UVVM
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "uvvm_util" / "src" / "*.vhd",          "uvvm_util")
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "uvvm_vvc_framework" / "src" / "*.vhd", "uvvm_vvc_framework")

    hr.add_files(repo_path / "thirdparty" / "uvvm" / "bitvis_vip_scoreboard" / "src" / "*.vhd", "bitvis_vip_scoreboard")

    # AXI-Stream VIP
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "bitvis_vip_axistream" / "src" / "*.vhd",                "bitvis_vip_axistream")
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "uvvm_vvc_framework" / "src_target_dependent" / "*.vhd", "bitvis_vip_axistream")

    # UART VIP
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "bitvis_vip_uart" / "src" / "*.vhd",                "bitvis_vip_uart")
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "uvvm_vvc_framework" / "src_target_dependent" / "*.vhd", "bitvis_vip_uart")

    # Clock Generator VVC
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "bitvis_vip_clock_generator" / "src" / "*.vhd",          "bitvis_vip_clock_generator")
    hr.add_files(repo_path / "thirdparty" / "uvvm" / "uvvm_vvc_framework" / "src_target_dependent" / "*.vhd", "bitvis_vip_clock_generator")

    # UVVM cosim
    hr.add_files(repo_path / "src" / "vhdl" / "*.vhd", "work")

    # Add RTL code for AXI-Stream UART
    hr.add_files(uart_path / "src" / "rtl" / "*.vhd", "work")

    # Add testbench code for AXI-Stream UART
    hr.add_files(uart_path / "src" / "tb" / "axistream_uart_uvvm_tb.vhd", "work")
    hr.add_files(uart_path / "src" / "tb" / "axistream_uart_uvvm_th.vhd", "work")

def main():
    hr = HDLRegression()
    init(hr)

    if hr.settings.get_simulator_name() == "NVC":
        print("Starting NVC sim")

        # Override heap space parameters to NVC
        # These are by default set to -H64m and -M64m in HDLregression,
        # which is too small.
        global_opts = [opt for opt in hr.settings.get_global_options() if "-H" not in opt and "-M" not in opt]
        global_opts.append("-H1g")
        global_opts.append("-M1g")
        hr.settings.set_global_options(global_opts)

        return hr.start(sim_options=[f"--load={repo_path / '..' / 'uvvm_co-simulation' / 'build' / 'libuvvm_cosim_vhpi.so'}"])
    else:
        # TODO: Add parameters to load cosim lib for other simulators
        return hr.start()

if __name__ == '__main__':
    sys.exit(main())
