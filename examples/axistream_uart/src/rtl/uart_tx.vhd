library ieee;
use ieee.std_logic_1164.all;


entity uart_tx is

  generic (
    GC_BAUDRATE : natural;
    GC_CLK_FREQ : natural);
  port (
    clk                    : in  std_logic;
    rst                    : in  std_logic;
    txd                    : out std_logic;
    s_axis_transmit_tdata  : in  std_logic_vector(7 downto 0);
    s_axis_transmit_tvalid : in  std_logic;
    s_axis_transmit_tready : out std_logic);

end entity uart_tx;

architecture rtl of uart_tx is

  signal baud_gen_enable : std_logic;
  signal baud_pulse      : std_logic;
  signal transmit_data   : std_logic_vector(0 to 9);
  signal bit_counter     : natural range 0 to 10;

  type t_uart_tx_fsm is (ST_IDLE, ST_TRANSMIT);
  signal fsm_state : t_uart_tx_fsm;

begin

  inst_baud_gen : entity work.baud_gen
    generic map (
      GC_BAUDRATE => GC_BAUDRATE,
      GC_CLK_FREQ => GC_CLK_FREQ,
      GC_BAUD_RX  => false)
    port map (
      clk        => clk,
      rst        => rst,
      enable     => baud_gen_enable,
      baud_pulse => baud_pulse);

  p_uart_tx_fsm : process (clk) is
  begin
    if rising_edge(clk) then

      baud_gen_enable <= '0';

      case fsm_state is
        when ST_IDLE =>
          txd         <= '1';
          bit_counter <= 0;

          -- AXI-S handshake done?
          if s_axis_transmit_tready = '1' and s_axis_transmit_tvalid = '1' then
            s_axis_transmit_tready    <= '0';
            transmit_data(0)          <= '0';                    -- Start bit

            -- Data bits
            for bit_num in 0 to 7 loop
              transmit_data(1+bit_num)  <= s_axis_transmit_tdata(7-bit_num);
              transmit_data(1+bit_num)  <= s_axis_transmit_tdata(bit_num);
            end loop;
            
            transmit_data(9)          <= '1';                    -- Stop bit
            baud_gen_enable           <= '1';
            fsm_state                 <= ST_TRANSMIT;
          else
            s_axis_transmit_tready <= '1';
          end if;

        when ST_TRANSMIT =>
          baud_gen_enable <= '1';

          if baud_pulse = '1' then

            if bit_counter = 10 then
              -- Stop bit has been transmitted
              fsm_state <= ST_IDLE;
            else
              txd         <= transmit_data(bit_counter);
              bit_counter <= bit_counter + 1;
            end if;
          end if;

      end case;

      if rst = '1' then
        fsm_state              <= ST_IDLE;
        s_axis_transmit_tready <= '0';
        baud_gen_enable        <= '0';
        txd                    <= '1';
      end if;
    end if;
  end process p_uart_tx_fsm;


end architecture rtl;
