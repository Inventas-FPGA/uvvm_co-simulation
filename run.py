import os
import pathlib
import sys

project_path = pathlib.Path(__file__).parent.resolve()

print(f"project_path: {project_path}")

# Path to HDLRegression:
sys.path.append(str(project_path / "thirdparty" / "hdlregression"))
sys.path.append('thirdparty/hdlregression') # Just to make LSP server understand path

from hdlregression import HDLRegression

def add_project_files(hr):
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

    # Add cosim module code
    hr.add_files("src/vhdl/uvvm_cosim_axis_vvc_ctrl.vhd", "uvvm_cosim_lib")
    hr.add_files("src/vhdl/uvvm_cosim_uart_vvc_ctrl.vhd", "uvvm_cosim_lib")
    hr.add_files("src/vhdl/uvvm_cosim_utils_pkg.vhd", "uvvm_cosim_lib")
    hr.add_files("src/vhdl/uvvm_cosim.vhd", "uvvm_cosim_lib")

    if hr.settings.get_simulator_name() == "NVC":
        hr.add_files("src/vhdl/uvvm_cosim_foreign_pkg_vhpi.vhd", "uvvm_cosim_lib")
    elif hr.settings.get_simulator_name() == "MODELSIM":
        hr.add_files("src/vhdl/uvvm_cosim_foreign_pkg_fli.vhd", "uvvm_cosim_lib")

    hr.add_files("src/vhdl/uvvm_cosim_foreign_pkg_body.vhd", "uvvm_cosim_lib")

    # Add testbench code
    hr.add_files("test/cosim_testbench.vhd", "work")

    hr.gen_report(report_file="sim_report.xml")


def main():
    hr = HDLRegression()

    if not hr.settings.get_simulator_name() in ("NVC", "MODELSIM"):
        print(f"Unsupported simulator {hr.settings.get_simulator_name()}. Only NVC and Modelsim/Questasim supported currently.")
        return -1

    add_project_files(hr)

    if hr.settings.get_simulator_name() == "NVC":
        print("Using NVC compiler/simulator")

        # Override heap space parameters to NVC
        # These are by default set to -H64m and -M64m in HDLregression,
        # which is too small.
        global_opts = [opt for opt in hr.settings.get_global_options() if "-H" not in opt and "-M" not in opt]
        global_opts.append("-H1g")
        global_opts.append("-M1g")
        hr.settings.set_global_options(global_opts)

        # Todo: Add build directoy arg to run.py instead of hardcoding it to "build"
        return hr.start(sim_options=[f"--load={project_path / 'build' / 'libuvvm_cosim_vhpi.so'}"])

    elif hr.settings.get_simulator_name() == "MODELSIM":
        print("Using Modelsim/Questasim")

        # Modelsim needs to find the cosim library in the build directory
        # See "Location of Shared Object Files" in the Foreign Language Reference Manual for options.
        # Here we just set $MGC_WD (one of the searched paths) to point to the build directory
        os.environ["MGC_WD"] = f"{project_path / 'build'}"
        return hr.start(sim_options=["-voptargs=+acc=r", "-foreign {uvvm_cosim_fli_init libuvvm_cosim_fli.so}"])

if __name__ == '__main__':
    sys.exit(main())
