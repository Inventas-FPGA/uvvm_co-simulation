-- Utility funcitons used by uvvm_cosim
library std;
use std.textio.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library bitvis_vip_uart;
context bitvis_vip_uart.vvc_context;

library bitvis_vip_axistream;
context bitvis_vip_axistream.vvc_context;

library bitvis_vip_axilite;
context bitvis_vip_axilite.vvc_context;

package uvvm_cosim_utils_pkg is

  function to_string(constant ch : t_channel) return string;
  function to_string(constant status : t_transaction_status) return string;

  function strcmp (
    constant str1 : string;
    constant str2 : string)
    return boolean;

  -- bfm_cfg_to_string functions create a comma-separated string
  -- of key=val pairs for whatever config options from BFM config
  -- types that we are interested in reporting to cosim.

  function bfm_cfg_to_string(
    constant cfg : t_axistream_bfm_config)
    return line;

  function bfm_cfg_to_string(
    constant cfg : t_uart_bfm_config)
    return line;

  function bfm_cfg_to_string(
    constant cfg : t_axilite_bfm_config)
    return line;

  -- For unsupported VVCs/BFMs
  function bfm_cfg_to_string(
    constant void : t_void)
    return line;

end package uvvm_cosim_utils_pkg;


package body uvvm_cosim_utils_pkg is

  function to_string(
    constant ch : t_channel
    ) return string is
  begin
    case ch is
      when NA           => return "NA";
      when ALL_CHANNELS => return "ALL_CHANNELS";
      when RX           => return "RX";
      when TX           => return "TX";
      when others =>
        report "Unknown channel type" severity failure;
        return "";
    end case;
  end function to_string;

  function to_string(
    constant status : t_transaction_status
    ) return string is
  begin
    case status is
      when INACTIVE    => return "INACTIVE";
      when IN_PROGRESS => return "IN_PROGRESS";
      when FAILED      => return "FAILED";
      when SUCCEEDED   => return "SUCCEEDED";
      when COMPLETED   => return "COMPLETED";
      when others =>
        report "Unknown transaction status type" severity failure;
        return "";
    end case;
  end function to_string;

  function strcmp (
    constant str1 : string;
    constant str2 : string)
    return boolean is
  begin
    assert str1'length <= str2'length report "str1 can't be longer than str2" severity failure;

    if str1(1 to str1'length) = str2(1 to str1'length) then
      return true;
    else
      return false;
    end if;
  end function strcmp;

  function bfm_cfg_to_string(
    constant cfg : t_axistream_bfm_config)
    return line
  is
    variable v_line : line := new string'("");
  begin
    write(v_line, string'("cosim_support=1,"));

    if cfg.check_packet_length then
      write(v_line, string'("packet_based=1,"));
    else
      write(v_line, string'("packet_based=0,"));
    end if;
    return v_line;
  end function bfm_cfg_to_string;

  function bfm_cfg_to_string(
    constant cfg : t_uart_bfm_config)
    return line
  is
    variable v_line : line := new string'("");
  begin
    write(v_line, string'("cosim_support=1,"));
    return v_line;
  end function bfm_cfg_to_string;

  function bfm_cfg_to_string(
    constant cfg : t_axilite_bfm_config)
    return line
  is
    variable v_line : line := new string'("");
  begin
    write(v_line, string'("cosim_support=1,"));
    return v_line;
  end function bfm_cfg_to_string;

  -- For unsupported VVCs/BFMs
  function bfm_cfg_to_string(
    constant void : t_void)
    return line
  is
    variable v_line : line := new string'("");
  begin
    write(v_line, string'("cosim_support=0,"));
    return v_line;
  end function bfm_cfg_to_string;

end package body uvvm_cosim_utils_pkg;
