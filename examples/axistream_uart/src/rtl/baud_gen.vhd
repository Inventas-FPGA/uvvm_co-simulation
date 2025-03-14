library ieee;
use ieee.std_logic_1164.all;

entity baud_gen is
  generic (
    GC_BAUDRATE : natural;
    GC_CLK_FREQ : natural;
    GC_BAUD_RX  : boolean  -- If baud pulse is for receive (midpoint) or transmit
    );
  port (
    clk        : in  std_logic;
    rst        : in  std_logic;
    enable     : in  std_logic;
    baud_pulse : out std_logic);
    
end entity baud_gen;

architecture rtl of baud_gen is
  constant C_COUNT_VAL : natural := GC_CLK_FREQ / GC_BAUDRATE;

  signal enable_d1 : std_logic;
  signal counter   : natural range 0 to C_COUNT_VAL-1;
  
begin

  p_baud_gen: process (clk) is
  begin
    if rising_edge(clk) then
      baud_pulse <= '0';
      enable_d1  <= enable;

      if enable = '1' and enable_d1 = '0' then
        -- Initialize on rising edge of enable
        if GC_BAUD_RX then
          counter <= C_COUNT_VAL/2;
        else
          counter <= 0;
        end if;

      elsif enable = '1' then
        -- Generate pulses while enabled

        if counter = C_COUNT_VAL-1 then
          counter    <= 0;
          baud_pulse <= '1';
        else
          counter <= counter + 1;
        end if;
      end if;

      if rst = '1' then
        enable_d1  <= '0';
        baud_pulse <= '0';
      end if;
    end if;

  end process p_baud_gen;

end architecture rtl;
