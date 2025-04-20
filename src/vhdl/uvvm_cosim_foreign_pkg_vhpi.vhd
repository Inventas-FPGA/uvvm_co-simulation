-- Declarations of foreign functions/procedures/callbacks used for
-- cosim with attributes for VHPI

-- NOTE about foreign functions (not procedures):
-- It is best to obey the same pure/impure rules as normal VHDL functions.
-- Modelsim may aggressively optimize away calls to pure FLI functions if
-- it thinks the arguments have not changed.

package uvvm_cosim_foreign_pkg is

  -- Blocks until simulation should run/resume
  procedure uvvm_cosim_foreign_start_sim;

  -- Returns bool as integer. True=1, False=0.
  impure function uvvm_cosim_foreign_terminate_sim return integer;

  procedure uvvm_cosim_foreign_report_vvc_info(
    constant vvc_type        : in string;
    constant vvc_channel     : in string;
    constant vvc_instance_id : in integer;
    constant bfm_cfg         : in string
    );

  impure function uvvm_cosim_foreign_vvc_listen_enable (
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer)
    return integer;

  -- Returns bool as integer. True=1, False=0.
  impure function uvvm_cosim_foreign_transmit_byte_queue_empty(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer;

  -- Returns integer with a data byte in bits 7:0 and end_of_packet
  -- flag in bit 8.
  impure function uvvm_cosim_foreign_transmit_byte_queue_get(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer;

  procedure uvvm_cosim_foreign_receive_byte_queue_put(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer;
    constant byte            : in integer);

  impure function uvvm_cosim_foreign_transmit_packet_queue_empty(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer;

  impure function uvvm_cosim_foreign_transmit_packet_queue_get(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer;

  procedure uvvm_cosim_foreign_receive_packet_queue_put(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer;
    constant byte            : in integer;
    constant end_of_packet   : in integer);

  attribute foreign of uvvm_cosim_foreign_start_sim                   : procedure is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_start_sim";
  attribute foreign of uvvm_cosim_foreign_terminate_sim               : function is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_terminate_sim";
  attribute foreign of uvvm_cosim_foreign_report_vvc_info             : procedure is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_report_vvc_info";
  attribute foreign of uvvm_cosim_foreign_vvc_listen_enable           : function is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_vvc_listen_enable";
  attribute foreign of uvvm_cosim_foreign_transmit_byte_queue_empty   : function is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_transmit_byte_queue_empty";
  attribute foreign of uvvm_cosim_foreign_transmit_byte_queue_get     : function is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_transmit_byte_queue_get";
  attribute foreign of uvvm_cosim_foreign_receive_byte_queue_put      : procedure is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_receive_byte_queue_put";
  attribute foreign of uvvm_cosim_foreign_transmit_packet_queue_empty : function is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_transmit_packet_queue_empty";
  attribute foreign of uvvm_cosim_foreign_transmit_packet_queue_get   : function is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_transmit_packet_queue_get";
  attribute foreign of uvvm_cosim_foreign_receive_packet_queue_put    : procedure is "VHPI libuvvm_cosim_vhpi.so uvvm_cosim_foreign_receive_packet_queue_put";

end package uvvm_cosim_foreign_pkg;
