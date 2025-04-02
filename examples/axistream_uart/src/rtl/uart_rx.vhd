library ieee;
use ieee.std_logic_1164.all;

entity uart_rx is
  generic (
    GC_BAUDRATE : natural;
    GC_CLK_FREQ : natural);
  port (
    clk                   : in  std_logic;
    rst                   : in  std_logic;
    rxd                   : in  std_logic;
    m_axis_receive_tdata  : out std_logic_vector(7 downto 0);
    m_axis_receive_tvalid : out std_logic;
    m_axis_receive_tready : in  std_logic);
end entity uart_rx;

architecture rtl of uart_rx is

  signal rxd_d1          : std_logic;
  signal baud_gen_enable : std_logic;
  signal baud_pulse      : std_logic;
  signal receive_data    : std_logic_vector(7 downto 0);
  signal bit_counter     : natural range 0 to 7;

  type t_uart_rx_fsm is (ST_IDLE, ST_RECEIVE_START_BIT, ST_RECEIVE_DATA, ST_RECEIVE_STOP_BIT, ST_ERROR);
  signal fsm_state : t_uart_rx_fsm;

begin

  inst_baud_gen: entity work.baud_gen
    generic map (
      GC_BAUDRATE => GC_BAUDRATE,
      GC_CLK_FREQ => GC_CLK_FREQ,
      GC_BAUD_RX  => true)
    port map (
      clk        => clk,
      rst        => rst,
      enable     => baud_gen_enable,
      baud_pulse => baud_pulse);


  p_uart_rx_fsm : process (clk) is
  begin
    if rising_edge(clk) then

      if m_axis_receive_tvalid = '1' and m_axis_receive_tready = '1' then
        m_axis_receive_tvalid <= '0';
      end if;

      rxd_d1 <= rxd;

      case fsm_state is
        when ST_IDLE =>
          baud_gen_enable <= '0';
          bit_counter     <= 0;

          -- Detect falling edge for start bit
          if rxd_d1 = '1' and rxd = '0' then
            baud_gen_enable <= '1';
            fsm_state       <= ST_RECEIVE_START_BIT;
          end if;

        when ST_RECEIVE_START_BIT =>

          if baud_pulse = '1' then
            if rxd = '0' then
              -- Start bit correct
              fsm_state <= ST_RECEIVE_DATA;
            else
              -- Start bit wrong polarity
              fsm_state <= ST_ERROR;
            end if;
          end if;

        when ST_RECEIVE_DATA =>
          if baud_pulse = '1' then
            receive_data(bit_counter) <= rxd;

            if bit_counter = 7 then
              fsm_state <= ST_RECEIVE_STOP_BIT;
            else
              bit_counter <= bit_counter + 1;
            end if;
          end if;

        when ST_RECEIVE_STOP_BIT =>
          if baud_pulse = '1' then
            if rxd = '1' then
              -- Stop bit correct
              m_axis_receive_tdata  <= receive_data;
              m_axis_receive_tvalid <= '1';
              fsm_state             <= ST_IDLE;
            else
              -- Start bit wrong polarity
              fsm_state <= ST_ERROR;
            end if;
          end if;

        when ST_ERROR =>
          -- Todo: Indicate error? Wait?
          fsm_state <= ST_IDLE;

      end case;

      if rst = '1' then
        fsm_state             <= ST_IDLE;
        rxd_d1                <= '1';
        m_axis_receive_tvalid <= '0';
        baud_gen_enable       <= '0';
      end if;
    end if;
  end process p_uart_rx_fsm;

end architecture rtl;
