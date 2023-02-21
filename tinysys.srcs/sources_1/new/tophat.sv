`timescale 1ns / 1ps

module tophat(
	// Board clock and reset
    input sys_clk
    ,input sys_rst_n
    // Debug LEDs
    ,output wire [1:0] leds
    // UART - USB-c module
    ,output wire uart_rxd_out
	,input wire uart_txd_in
	// DDR3 SDRAM
	,output ddr3_reset_n
	,output wire [0:0] ddr3_cke
	,output wire [0:0] ddr3_ck_p 
	,output wire [0:0] ddr3_ck_n
	,output wire ddr3_ras_n 
	,output wire ddr3_cas_n 
	,output wire ddr3_we_n
	,output wire [2:0] ddr3_ba
	,output wire [13:0] ddr3_addr
	,output wire [0:0] ddr3_odt
	,output wire [1:0] ddr3_dm
	,inout wire [1:0] ddr3_dqs_p
	,inout wire [1:0] ddr3_dqs_n
	,inout wire [15:0] ddr3_dq
	// DVI -> HDMI
	,output wire [2:0] hdmi_tx_p
	,output wire [2:0] hdmi_tx_n
	,output wire hdmi_tx_clk_p
	,output wire hdmi_tx_clk_n
	// Micro SD Card
	,input wire sd_miso
	,output wire sd_cs_n
	,output wire sd_clk
	,output wire sd_mosi
	// Analog inputs
	,input wire VA[4:0] );

// --------------------------------------------------
// Clock and reset generator
// --------------------------------------------------

wire aresetn, preresetn;
wire init_calib_complete;
wire aclk, clk10, clk25, clk50, clk166, clk200, clk250;

// Clock and reset generator
clockandreset clockandresetinst(
	.sys_clock_i(sys_clk),
	.sys_rst_n(sys_rst_n),
	.init_calib_complete(init_calib_complete),
	.aclk(aclk),
	.clk10(clk10),
	.clk25(clk25),
	.clk50(clk50),
	.clk166(clk166),
	.clk200(clk200),
	.clk250(clk250),
	.aresetn(aresetn),
	.preresetn(preresetn));

// --------------------------------------------------
// DDR3 SDRAM wires
// --------------------------------------------------

ddr3sdramwires ddr3wires(
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
	.init_calib_complete(init_calib_complete) );

// --------------------------------------------------
// Video wires
// --------------------------------------------------

gpuwires gpuvideoout(
	.tmdsp(hdmi_tx_p),
	.tmdsn(hdmi_tx_n),
	.tmdsclkp(hdmi_tx_clk_p ),
	.tmdsclkn(hdmi_tx_clk_n) );

// --------------------------------------------------
// SPI wires
// --------------------------------------------------

sdwires sdconn(
	.sd_miso(sd_miso),
	.sd_cs_n(sd_cs_n),
	.sd_clk(sd_clk),
	.sd_mosi(sd_mosi) );

// --------------------------------------------------
// SoC device
// --------------------------------------------------

tinysoc socinstance(
	.aclk(aclk),
	.clk10(clk10),
	.clk25(clk25),
	.clk50(clk50),
	.clk166(clk166),
	.clk200(clk200),
	.clk250(clk250),
	.aresetn(aresetn),
	.preresetn(preresetn),
	.uart_rxd_out(uart_rxd_out),
	.uart_txd_in(uart_txd_in),
	.leds(leds),
	.ddr3wires(ddr3wires),
	.gpuvideoout(gpuvideoout),
	.sdconn(sdconn));

endmodule
