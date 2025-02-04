import pathlib
import sys

project_path = pathlib.Path(__file__).parent.resolve()

print(f"project_path: {project_path}")

# Path to HDLRegression:
sys.path.append(str(project_path / "thirdparty" / "hdlregression"))
sys.path.append('thirdparty/hdlregression') # Just to make LSP server understand path

from hdlregression import HDLRegression


def init(hr):
    # Alternative: Compile ALL of UVVM
    # hr.compile_uvvm("thirdparty/uvvm/")

    # UVVM
    hr.add_files("thirdparty/uvvm/uvvm_util/src/*.vhd",          "uvvm_util")
    hr.add_files("thirdparty/uvvm/uvvm_vvc_framework/src/*.vhd", "uvvm_vvc_framework")

    hr.add_files("thirdparty/uvvm/bitvis_vip_scoreboard/src/*.vhd", "bitvis_vip_scoreboard")

    # AXI-Stream VIP
    hr.add_files("thirdparty/uvvm/bitvis_vip_axistream/src/*.vhd",                "bitvis_vip_axistream")
    hr.add_files("thirdparty/uvvm/uvvm_vvc_framework/src_target_dependent/*.vhd", "bitvis_vip_axistream")

    # UART VIP
    hr.add_files("thirdparty/uvvm/bitvis_vip_uart/src/*.vhd",                "bitvis_vip_uart")
    hr.add_files("thirdparty/uvvm/uvvm_vvc_framework/src_target_dependent/*.vhd", "bitvis_vip_uart")

    # Clock Generator VVC
    hr.add_files("thirdparty/uvvm/bitvis_vip_clock_generator/src/*.vhd",          "bitvis_vip_clock_generator")
    hr.add_files("thirdparty/uvvm/uvvm_vvc_framework/src_target_dependent/*.vhd", "bitvis_vip_clock_generator")

    # Add RTL code for AXI-Stream UART
    hr.add_files("src/vhdl/*.vhd", "work")

    # Add testbench code for AXI-Stream UART
    hr.add_files("test/vhpi_testbench.vhd", "work")

    hr.gen_report(report_file="sim_report.xml")


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

        # Todo: Add build directoy arg to run.py instead of hardcoding it to "build"
        return hr.start(sim_options=[f"--load={project_path / 'build' / 'libuvvm_cosim_vhpi.so'}"])
    else:
        return hr.start()


if __name__ == '__main__':
    sys.exit(main())
