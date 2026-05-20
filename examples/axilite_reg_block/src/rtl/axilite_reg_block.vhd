-- File: axilite_reg_block.vhd
-- Description: Simple AXI-Lite slave register block.
--   9 RW scratch registers (offsets 0x00..0x20)
--   1 RO ID register at 0x24, value 0xCAFEC0DE
-- All registers are 32 bit wide; access is word-aligned. Out-of-range
-- accesses return OKAY with zeros (read) or are silently dropped (write).

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity axilite_reg_block is
  generic (
    GC_ADDR_WIDTH : natural := 8;
    GC_DATA_WIDTH : natural := 32);
  port (
    clk : in std_logic;
    rst : in std_logic;

    -- Write address channel
    s_axi_awaddr  : in  std_logic_vector(GC_ADDR_WIDTH-1 downto 0);
    s_axi_awprot  : in  std_logic_vector(2 downto 0);
    s_axi_awvalid : in  std_logic;
    s_axi_awready : out std_logic;

    -- Write data channel
    s_axi_wdata  : in  std_logic_vector(GC_DATA_WIDTH-1 downto 0);
    s_axi_wstrb  : in  std_logic_vector(GC_DATA_WIDTH/8-1 downto 0);
    s_axi_wvalid : in  std_logic;
    s_axi_wready : out std_logic;

    -- Write response channel
    s_axi_bresp  : out std_logic_vector(1 downto 0);
    s_axi_bvalid : out std_logic;
    s_axi_bready : in  std_logic;

    -- Read address channel
    s_axi_araddr  : in  std_logic_vector(GC_ADDR_WIDTH-1 downto 0);
    s_axi_arprot  : in  std_logic_vector(2 downto 0);
    s_axi_arvalid : in  std_logic;
    s_axi_arready : out std_logic;

    -- Read data channel
    s_axi_rdata  : out std_logic_vector(GC_DATA_WIDTH-1 downto 0);
    s_axi_rresp  : out std_logic_vector(1 downto 0);
    s_axi_rvalid : out std_logic;
    s_axi_rready : in  std_logic);
end entity axilite_reg_block;

architecture rtl of axilite_reg_block is

  constant C_NUM_SCRATCH : natural := 9;
  constant C_ID_INDEX    : natural := 9;             -- 0x24 / 4
  constant C_ID_VALUE    : std_logic_vector(31 downto 0) := x"CAFEC0DE";
  constant C_RESP_OKAY   : std_logic_vector(1 downto 0) := "00";

  -- Word index = address(byte) / 4 = addr(GC_ADDR_WIDTH-1 downto 2)
  constant C_REG_IDX_WIDTH : natural := GC_ADDR_WIDTH - 2;

  type t_reg_array is array (0 to C_NUM_SCRATCH-1) of std_logic_vector(31 downto 0);
  signal scratch : t_reg_array;

  -- Write FSM state
  type t_wr_state is (WR_IDLE, WR_RESP);
  signal wr_state : t_wr_state;

  signal awaddr_q : std_logic_vector(GC_ADDR_WIDTH-1 downto 0);
  signal wdata_q  : std_logic_vector(GC_DATA_WIDTH-1 downto 0);

  -- Read FSM state
  type t_rd_state is (RD_IDLE, RD_DATA);
  signal rd_state : t_rd_state;

  signal rdata_q : std_logic_vector(GC_DATA_WIDTH-1 downto 0);

begin

  ------------------------------------------------------------------------
  -- Write channel: accept awaddr+wdata together, then drive bvalid until
  -- accepted. We assert awready/wready only in WR_IDLE so both channels
  -- must arrive in the same cycle. This keeps the FSM trivial and is fine
  -- for the AXI-Lite VVC which presents both signals together.
  ------------------------------------------------------------------------
  p_wr : process(clk)
    variable v_wr_idx : natural;
  begin
    if rising_edge(clk) then
      if rst = '1' then
        wr_state <= WR_IDLE;
        s_axi_awready <= '0';
        s_axi_wready  <= '0';
        s_axi_bvalid  <= '0';
        s_axi_bresp   <= C_RESP_OKAY;
        for i in 0 to C_NUM_SCRATCH-1 loop
          scratch(i) <= (others => '0');
        end loop;
      else
        case wr_state is
          when WR_IDLE =>
            s_axi_awready <= '1';
            s_axi_wready  <= '1';
            s_axi_bvalid  <= '0';
            if s_axi_awvalid = '1' and s_axi_wvalid = '1' then
              awaddr_q <= s_axi_awaddr;
              wdata_q  <= s_axi_wdata;
              -- Commit write
              v_wr_idx := to_integer(unsigned(s_axi_awaddr(GC_ADDR_WIDTH-1 downto 2)));
              if v_wr_idx < C_NUM_SCRATCH then
                scratch(v_wr_idx) <= s_axi_wdata;
              end if;
              s_axi_awready <= '0';
              s_axi_wready  <= '0';
              s_axi_bvalid  <= '1';
              s_axi_bresp   <= C_RESP_OKAY;
              wr_state <= WR_RESP;
            end if;

          when WR_RESP =>
            if s_axi_bready = '1' then
              s_axi_bvalid  <= '0';
              s_axi_awready <= '1';
              s_axi_wready  <= '1';
              wr_state <= WR_IDLE;
            end if;
        end case;
      end if;
    end if;
  end process p_wr;

  ------------------------------------------------------------------------
  -- Read channel: capture araddr, drive rdata/rvalid until accepted.
  ------------------------------------------------------------------------
  p_rd : process(clk)
    variable v_rd_idx : natural;
  begin
    if rising_edge(clk) then
      if rst = '1' then
        rd_state <= RD_IDLE;
        s_axi_arready <= '0';
        s_axi_rvalid  <= '0';
        s_axi_rresp   <= C_RESP_OKAY;
        rdata_q       <= (others => '0');
      else
        case rd_state is
          when RD_IDLE =>
            s_axi_arready <= '1';
            s_axi_rvalid  <= '0';
            if s_axi_arvalid = '1' then
              v_rd_idx := to_integer(unsigned(s_axi_araddr(GC_ADDR_WIDTH-1 downto 2)));
              if v_rd_idx < C_NUM_SCRATCH then
                rdata_q <= scratch(v_rd_idx);
              elsif v_rd_idx = C_ID_INDEX then
                rdata_q <= C_ID_VALUE;
              else
                rdata_q <= (others => '0');
              end if;
              s_axi_arready <= '0';
              s_axi_rvalid  <= '1';
              s_axi_rresp   <= C_RESP_OKAY;
              rd_state <= RD_DATA;
            end if;

          when RD_DATA =>
            if s_axi_rready = '1' then
              s_axi_rvalid  <= '0';
              s_axi_arready <= '1';
              rd_state <= RD_IDLE;
            end if;
        end case;
      end if;
    end if;
  end process p_rd;

  s_axi_rdata <= rdata_q;

end architecture rtl;
