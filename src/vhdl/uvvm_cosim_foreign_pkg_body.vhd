-- Declarations for foreign functions/procedures.
-- Although they are replaced with their foreign implementations and
-- not used, the language still requires them.

package body uvvm_cosim_foreign_pkg is

  procedure uvvm_cosim_foreign_start_sim is
  begin
    report "Error: Should use foreign implementation" severity failure;
  end procedure;

  impure function uvvm_cosim_foreign_terminate_sim return integer is
  begin
    report "Error: Should use foreign implementation" severity failure;
    return 0;
  end function;

  procedure uvvm_cosim_foreign_report_vvc_info(
    constant vvc_type        : in string;
    constant vvc_channel     : in string;
    constant vvc_instance_id : in integer;
    constant bfm_cfg         : in string
    ) is
  begin
    report "Error: Should use foreign implementation" severity failure;
  end procedure;

  impure function uvvm_cosim_foreign_vvc_listen_enable (
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer)
    return integer is
  begin
    report "Error: Should use foreign implementation" severity failure;
    return 0;
  end;

  impure function uvvm_cosim_foreign_transmit_byte_queue_empty(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer is
  begin
    report "Error: Should use foreign implementation" severity failure;
    return 0;
  end function;

  impure function uvvm_cosim_foreign_transmit_byte_queue_get(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer) return integer is
  begin
    report "Error: Should use foreign implementation" severity failure;
    return 0;
  end function;

  procedure uvvm_cosim_foreign_receive_byte_queue_put(
    constant vvc_type        : in string;
    constant vvc_instance_id : in integer;
    constant byte            : in integer
    ) is
  begin
    report "Error: Should use foreign implementation" severity failure;
  end procedure;

end package body uvvm_cosim_foreign_pkg;
