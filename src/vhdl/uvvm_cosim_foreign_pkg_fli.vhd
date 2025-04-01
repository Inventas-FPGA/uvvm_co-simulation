-- Declarations of foreign functions/procedures/callbacks used for
-- cosim with attributes for Modelsim/Questasim FLI

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
  impure function uvvm_cosim_foreign_transmit_queue_empty(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer;

  -- Returns integer with a data byte in bits 7:0 and end_of_packet
  -- flag in bit 8.
  impure function uvvm_cosim_foreign_transmit_queue_get(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer;

  procedure uvvm_cosim_foreign_receive_queue_put(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer;
    constant byte            : in integer;
    constant end_of_packet   : in integer);

  attribute foreign of uvvm_cosim_foreign_start_sim            : procedure is "uvvm_cosim_foreign_start_sim libuvvm_cosim_fli.so";
  attribute foreign of uvvm_cosim_foreign_terminate_sim        : function  is "uvvm_cosim_foreign_terminate_sim libuvvm_cosim_fli.so";
  attribute foreign of uvvm_cosim_foreign_report_vvc_info      : procedure is "uvvm_cosim_foreign_report_vvc_info libuvvm_cosim_fli.so";
  attribute foreign of uvvm_cosim_foreign_vvc_listen_enable    : function  is "uvvm_cosim_foreign_vvc_listen_enable libuvvm_cosim_fli.so";
  attribute foreign of uvvm_cosim_foreign_transmit_queue_empty : function  is "uvvm_cosim_foreign_transmit_queue_empty libuvvm_cosim_fli.so";
  attribute foreign of uvvm_cosim_foreign_transmit_queue_get   : function  is "uvvm_cosim_foreign_transmit_queue_get libuvvm_cosim_fli.so";
  attribute foreign of uvvm_cosim_foreign_receive_queue_put    : procedure is "uvvm_cosim_foreign_receive_queue_put libuvvm_cosim_fli.so";

end package uvvm_cosim_foreign_pkg;
