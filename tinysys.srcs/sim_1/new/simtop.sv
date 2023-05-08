`timescale 1ns / 1ps

module simtop();

logic boardresetn;
logic boardclock;

initial begin
	boardresetn = 1'bz;
	boardclock = 1'bz;
	#80;
	boardclock = 1'b0;
	boardresetn = 1'b1;
end

wire [3:0] ledout;
wire uart_rxd_out;
wire uart_txd_in = 1'b1;

wire sdcard_mosi;
wire sdcard_miso = sdcard_mosi; // nul device
wire usbc_mosi;
wire usbc_miso = usbc_mosi; // nul device

wire adclk = 1'b0;
wire addout = 1'b0;
wire addin;
wire adcs = 1'b0;

// DDR3 simulation model
wire ddr3_reset_n;
wire [0:0]   ddr3_cke;
wire [0:0]   ddr3_ck_p; 
wire [0:0]   ddr3_ck_n;
wire ddr3_ras_n; 
wire ddr3_cas_n;
wire ddr3_we_n;
wire [2:0]   ddr3_ba;
wire [13:0]  ddr3_addr;
wire [0:0]   ddr3_odt;
wire [1:0]   ddr3_dm;
wire [1:0]   ddr3_dqs_p;
wire [1:0]   ddr3_dqs_n;
wire [15:0]  ddr3_dq;

wire usb_d_p;
wire usb_d_n;

ddr3_model ddr3simmod(
    .rst_n(ddr3_reset_n),
    .ck(ddr3_ck_p),
    .ck_n(ddr3_ck_n),
    .cke(ddr3_cke),
    .cs_n(1'b0),
    .ras_n(ddr3_ras_n),
    .cas_n(ddr3_cas_n),
    .we_n(ddr3_we_n),
    .dm_tdqs(ddr3_dm),
    .ba(ddr3_ba),
    .addr(ddr3_addr),
    .dq(ddr3_dq),
    .dqs(ddr3_dqs_p),
    .dqs_n(ddr3_dqs_n),
    .tdqs_n(), // out
    .odt(ddr3_odt) );

tophat main(
    .sys_clk(boardclock),
    .sysresetn(boardresetn),
    // LEDs
    .leds(ledout),
    // UART
    .uart_rxd_out(uart_rxd_out),
    .uart_txd_in(uart_txd_in),
    // USB
	.usb_d_p(usb_d_p),
	.usb_d_n(usb_d_n),
	// DDR3 SDRAM
	.ddr3_reset_n(ddr3_reset_n),
	.ddr3_cke(ddr3_cke),
	.ddr3_ck_p(ddr3_ck_p), 
	.ddr3_ck_n(ddr3_ck_n),
	.ddr3_ras_n(ddr3_ras_n), 
	.ddr3_cas_n(ddr3_cas_n), 
	.ddr3_we_n(ddr3_we_n),
	.ddr3_ba(ddr3_ba),
	.ddr3_addr(ddr3_addr),
	.ddr3_odt(ddr3_odt),
	.ddr3_dm(ddr3_dm),
	.ddr3_dqs_p(ddr3_dqs_p),
	.ddr3_dqs_n(ddr3_dqs_n),
	.ddr3_dq(ddr3_dq),
	// DVI -> HDMI
	.hdmi_tx_p(),
	.hdmi_tx_n(),
	.hdmi_tx_clk_p(),
	.hdmi_tx_clk_n(),
	// Micro SD Card
	.sdcard_miso(sdcard_miso),
	.sdcard_cs_n(),
	.sdcard_clk(),
	.sdcard_mosi(sdcard_mosi),
	.sdcard_swtch(1'b0),
	// USB-C via MAX4320
	.usbc_miso(usbc_miso),
	.usbc_ss_n(),
	.usbc_clk(),
	.usbc_mosi(usbc_mosi),
	.usbc_resn(),
	.usbc_int(1'b0),
	.usbc_gpx(1'b0),
	// ADC
	.adclk(adclk),
	.addout(addout),
	.addin(addin),
	.adcs(adcs));

always begin
	#10
	boardclock = ~boardclock;
end

endmodule
