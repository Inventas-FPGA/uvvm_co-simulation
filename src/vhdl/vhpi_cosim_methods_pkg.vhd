-- Declarations of VHPI foreign functions/procedures/callbacks used for cosim

library uvvm_util;
context uvvm_util.uvvm_util_context;

package vhpi_cosim_methods_pkg is

  -- Blocks until simulation should run/resume
  procedure vhpi_cosim_start_sim;

  -- Returns bool as integer. True=1, False=0.
  function vhpi_cosim_terminate_sim return integer;

  procedure vhpi_cosim_report_vvc_info(
    constant vvc_type        : in string;
    constant vvc_channel     : in string;
    constant vvc_instance_id : in integer;
    constant bfm_cfg         : in string
    );

  function vhpi_cosim_vvc_listen_enable (
    constant vvc_type        : string;
    constant vvc_instance_id : integer)
    return boolean;

  -- Returns bool as integer. True=1, False=0.
  function vhpi_cosim_transmit_queue_empty(
    constant vvc_type        : string;
    constant vvc_instance_id : integer) return integer;

  -- Returns integer with a data byte in bits 7:0 and end_of_packet
  -- flag in bit 8.
  function vhpi_cosim_transmit_queue_get(
    constant vvc_type        : string;
    constant vvc_instance_id : integer) return integer;

  procedure vhpi_cosim_receive_queue_put(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer;
    constant byte            : in integer;
    constant end_of_packet   : in integer);

  attribute foreign of vhpi_cosim_start_sim            : procedure is "VHPI uvvm_cosim_lib vhpi_cosim_start_sim";
  attribute foreign of vhpi_cosim_terminate_sim        : function  is "VHPI uvvm_cosim_lib vhpi_cosim_terminate_sim";
  attribute foreign of vhpi_cosim_report_vvc_info      : procedure is "VHPI uvvm_cosim_lib vhpi_cosim_report_vvc_info";
  attribute foreign of vhpi_cosim_vvc_listen_enable    : function  is "VHPI uvvm_cosim_lib vhpi_cosim_listen_enable";
  attribute foreign of vhpi_cosim_transmit_queue_empty : function  is "VHPI uvvm_cosim_lib vhpi_cosim_transmit_queue_empty";
  attribute foreign of vhpi_cosim_transmit_queue_get   : function  is "VHPI uvvm_cosim_lib vhpi_cosim_transmit_queue_get";
  attribute foreign of vhpi_cosim_receive_queue_put    : procedure is "VHPI uvvm_cosim_lib vhpi_cosim_receive_queue_put";

end package vhpi_cosim_methods_pkg;


package body vhpi_cosim_methods_pkg is

  procedure vhpi_cosim_start_sim is
  begin
    report "Error: Should use foreign VHPI implementation" severity failure;
  end procedure;

  function vhpi_cosim_terminate_sim return integer is
  begin
    report "Error: Should use foreign VHPI implementation" severity failure;
  end function;

  procedure vhpi_cosim_report_vvc_info(
    constant vvc_type        : in string;
    constant vvc_channel     : in string;
    constant vvc_instance_id : in integer;
    constant bfm_cfg         : in string
    ) is
  begin
    report "Error: Should use foreign VHPI implementation" severity failure;
  end procedure;

  function vhpi_cosim_vvc_listen_enable (
    constant vvc_type        : string;
    constant vvc_instance_id : integer)
    return boolean is
  begin
    report "Error: Should use foreign VHPI implementation" severity failure;
  end;

  function vhpi_cosim_transmit_queue_empty(
    constant vvc_type        : string;
    constant vvc_instance_id : integer) return integer is
  begin
    report "Error: Should use foreign VHPI implementation" severity failure;
  end function;

  function vhpi_cosim_transmit_queue_get(
    constant vvc_type        : string;
    constant vvc_instance_id : integer) return integer is
  begin
    report "Error: Should use foreign VHPI implementation" severity failure;
  end function;

  procedure vhpi_cosim_receive_queue_put(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer;
    constant byte            : in integer;
    constant end_of_packet   : in integer
    ) is
  begin
    report "Error: Should use foreign VHPI implementation" severity failure;
  end procedure;

end package body vhpi_cosim_methods_pkg;
