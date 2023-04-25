`timescale 1ns / 1ps

module tinysoc #(
	parameter int RESETVECTOR = 32'd0
) (
	input wire aclk,
	input wire clk10,
	input wire clk12,
	input wire clk25,
	input wire clk100,
	input wire clk125,
	input wire clk166,
	input wire clk200,
	input wire aresetn,
	input wire preresetn,
	output wire [3:0] leds,
	output wire uart_rxd_out,
	input wire uart_txd_in,
	inout wire usb_d_p,
	inout wire usb_d_n,
	gpuwires.def gpuvideoout,
	ddr3sdramwires.def ddr3conn,
	audiowires.def i2sconn,
	spiwires.def sdconn,
	adcwires.def adcconn);

// --------------------------------------------------
// Bus lines
// --------------------------------------------------

axi4if instructionbus();	// Fetch unit bus
axi4if databus();			// Data unit bus
axi4if gpubus();			// Graphics unit bus
axi4if dmabus();			// Direct memory access bus
axi4if romcopybus();		// Bus for boot ROM copy (TODO: switch between ROM types?)
axi4if audiobus();			// Bus for audio device output

axi4if memorybus();			// Main memory

axi4if devicebus();			// Arbitrated, to devices
axi4if uartif();			// Sub bus: UART device
axi4if ledif();				// Sub bus: LED contol device
axi4if gpucmdif();			// Sub bus: GPU command fifo
axi4if spiif();				// Sub bus: SPI control device
axi4if csrif();				// Sub bus: CSR file device
axi4if xadcif();			// Sub bus: ADC controller
axi4if dmaif();				// Sub bus: DMA controller
axi4if usbif();				// Sub bus: USB host i/o
axi4if audioif();			// Sub bus: AUDIO delta-sigma output unit

ibusif ibus();				// Internal bus between units

// --------------------------------------------------
// Fetch unit
// --------------------------------------------------

wire branchresolved;
wire [31:0] branchtarget;
wire ififoempty;
wire ififovalid;
wire [143:0] ififodout;
wire ififord_en;
wire [1:0] irqReq;
wire [31:0] mepc;
wire [31:0] mtvec;
wire sie;
wire romReady;

// Reset vector at last 64K of DDR3 SDRAM
fetchunit #(.RESETVECTOR(RESETVECTOR)) fetchdecodeinst (
	.aclk(aclk),
	.aresetn(aresetn),
	.branchresolved(branchresolved),
	.branchtarget(branchtarget),
	.ififoempty(ififoempty),
	.ififovalid(ififovalid),
	.ififodout(ififodout),
	.ififord_en(ififord_en),
	.irqReq(irqReq),
	.mepc(mepc),
	.mtvec(mtvec),
	.sie(sie),
	.romReady(romReady),
	.m_axi(instructionbus) );

// --------------------------------------------------
// Data unit
// --------------------------------------------------

dataunit dataunitinst (
	.aclk(aclk),
	.aresetn(aresetn),
	.s_ibus(ibus),
	.databus(databus),
	.devicebus(devicebus) );

// --------------------------------------------------
// Wall clock
// --------------------------------------------------

logic [63:0] wallclockcounter = 64'd0;

always @(posedge clk10) begin
	if (~aresetn) begin
		wallclockcounter <= 64'd0;
	end else begin
		wallclockcounter <= wallclockcounter + 64'd1;
	end
end

// CDC for wall clock
(* async_reg = "true" *) logic [63:0] wallclocktimeA = 64'd0;
(* async_reg = "true" *) logic [63:0] wallclocktime = 64'd0;

always @(posedge aclk) begin
	if (~aresetn) begin
		wallclocktimeA <= 64'd0;
		wallclocktime <= 64'd0;
	end else begin
		wallclocktimeA <= wallclockcounter;
		wallclocktime <= wallclocktimeA;
	end
end

// --------------------------------------------------
// Control unit
// --------------------------------------------------

wire [63:0] cpuclocktime;
wire [63:0] retired;

controlunit #(.CID(32'h00000000)) controlunitinst (
	.aclk(aclk),
	.aresetn(aresetn),
	.branchresolved(branchresolved),
	.branchtarget(branchtarget),
	.ififoempty(ififoempty),
	.ififovalid(ififovalid),
	.ififodout(ififodout),
	.ififord_en(ififord_en), 
	.cpuclocktime(cpuclocktime),
	.retired(retired),
	.m_ibus(ibus));

// --------------------------------------------------
// Graphics unit
// --------------------------------------------------

wire gpufifoempty;
wire [31:0] gpufifodout;
wire gpufifore;
wire gpufifovalid;
wire [31:0] vblankcount;
gpucore GPU(
	.aclk(aclk),
	.clk25(clk25),
	.clk125(clk125),
	.aresetn(aresetn),
	.m_axi(gpubus),
	.gpuvideoout(gpuvideoout),
	.gpufifoempty(gpufifoempty),
	.gpufifodout(gpufifodout),
	.gpufifore(gpufifore),
	.gpufifovalid(gpufifovalid),
	.vblankcount(vblankcount));

// --------------------------------------------------
// DMA unit
// --------------------------------------------------

wire dmafifoempty;
wire dmafifore;
wire dmafifovalid;
wire dmabusy;
wire [31:0] dmafifodout;

axi4dma DMA(
	.aclk(aclk),
	.aresetn(aresetn),
	.m_axi(dmabus),
	.dmafifoempty(dmafifoempty),
	.dmafifodout(dmafifodout),
	.dmafifore(dmafifore),
	.dmafifovalid(dmafifovalid),
	.dmabusy(dmabusy) );

// --------------------------------------------------
// Boot ROM copy unit
// --------------------------------------------------

axi4romcopy #(.RESETVECTOR(RESETVECTOR)) ROMCopy(
	.aclk(aclk),
	.aresetn(aresetn),
	.m_axi(romcopybus),
	.romReady(romReady));

// --------------------------------------------------
// Audio processing unit
// --------------------------------------------------

wire audiofifoempty;
wire [31:0] audiofifodout;
wire audiofifore;
wire audiofifovalid;
wire [31:0] audiobufferswapcount;

axi4i2saudio APU(
	.aclk(aclk),				// Bus clock
	.aresetn(aresetn),
    .audioclock(clk12),			// 22.591MHz master clock

	.m_axi(audiobus),			// Memory access

	.abempty(audiofifoempty),	// APU command FIFO
	.abvalid(audiofifovalid),
	.audiore(audiofifore),
    .audiodin(audiofifodout),
    .swapcount(audiobufferswapcount),

    .tx_mclk(i2sconn.mclk),
    .tx_lrck(i2sconn.lrclk),
    .tx_sclk(i2sconn.sclk),
    .tx_sdout(i2sconn.sdin) );

// --------------------------------------------------
// Traffic arbiter between master units and memory
// --------------------------------------------------

arbiter arbiter6x1inst(
	.aclk(aclk),
	.aresetn(aresetn),
	.axi_s({romcopybus, audiobus, dmabus, gpubus, databus, instructionbus}),
	.axi_m(memorybus) );

// --------------------------------------------------
// RAM
// --------------------------------------------------

wire [11:0] device_temp;

// dev   start     end
// DDR3: 00000000  0FFFFFFF

axi4ddr3sdram axi4ddr3sdraminst(
	.aclk(aclk),
	.aresetn(aresetn),
	.preresetn(preresetn),
	.clk_sys_i(clk166),
	.clk_ref_i(clk200),
	.m_axi(memorybus),
	.ddr3conn(ddr3conn),
	.device_temp(device_temp) );

// --------------------------------------------------
// Memory mapped device router
// --------------------------------------------------

// NOTE: CSR offsets are word offsets therefore the
// address space is for 4Kbytes of data yet we can
// access 16KBytes worth of data.

// 12bit (4K) address space reserved for each memory mapped device
// dev   start     end       addrs[30:12]                 size  notes
// UART: 80000000  80000FFF  19'b000_0000_0000_0000_0000  4KB
// LEDS: 80001000  80001FFF  19'b000_0000_0000_0000_0001  4KB
// GPUC: 80002000  80002FFF  19'b000_0000_0000_0000_0010  4KB
// SPIC: 80003000  80003FFF  19'b000_0000_0000_0000_0011  4KB
// CSRF: 80004000  80004FFF  19'b000_0000_0000_0000_0100  4KB	HART#0
// XADC: 80005000  80005FFF  19'b000_0000_0000_0000_0101  4KB
// DMAC: 80006000  80006FFF  19'b000_0000_0000_0000_0110  4KB
// USBH: 80007000  80007FFF  19'b000_0000_0000_0000_0111  4KB
// AUDI: 80008000  80008FFF  19'b000_0000_0000_0000_1000  4KB
// ----: 80009000  80009FFF  19'b000_0000_0000_0000_1001  4KB
// ----: 8000A000  8000AFFF  19'b000_0000_0000_0000_1010  4KB
// ----: 8000B000  8000BFFF  19'b000_0000_0000_0000_1011  4KB
// ----: 8000C000  8000CFFF  19'b000_0000_0000_0000_1100  4KB
// ----: 8000D000  8000DFFF  19'b000_0000_0000_0000_1101  4KB
// ----: 8000E000  8000EFFF  19'b000_0000_0000_0000_1110  4KB
// ----: 8000F000  8000FFFF  19'b000_0000_0000_0000_1111  4KB

devicerouter devicerouterinst(
	.aclk(aclk),
	.aresetn(aresetn),
    .axi_s(devicebus),
    .addressmask({
    	19'b000_0000_0000_0000_1000,	// AUDI
        19'b000_0000_0000_0000_0111,	// USBH
    	19'b000_0000_0000_0000_0110,	// DMAC
    	19'b000_0000_0000_0000_0101,	// XADC
		19'b000_0000_0000_0000_0100,	// CSRF
		19'b000_0000_0000_0000_0011,	// SPIC
		19'b000_0000_0000_0000_0010,	// GPUC
		19'b000_0000_0000_0000_0001,	// LEDS
		19'b000_0000_0000_0000_0000 }),	// UART
    .axi_m({audioif, usbif, dmaif, xadcif, csrif, spiif, gpucmdif, ledif, uartif}));

// --------------------------------------------------
// Memory mapped devices
// --------------------------------------------------

wire uartrcvempty;

axi4uart #(.BAUDRATE(115200), .FREQ(10000000)) uartinst(
	.aclk(aclk),
	.uartclk(clk10),
	.aresetn(aresetn),
	.s_axi(uartif),
	.uart_rxd_out(uart_rxd_out),
	.uart_txd_in(uart_txd_in),
	.uartrcvempty(uartrcvempty));

axi4led leddevice(
	.aclk(aclk),
	.aresetn(aresetn),
	.s_axi(ledif),
	.led(leds) );

// NOTE: This is different than other command devices for now
commandqueue gpucmdinst(
	.aclk(aclk),
	.aresetn(aresetn),
	.s_axi(gpucmdif),
	.fifoempty(gpufifoempty),
	.fifodout(gpufifodout),
	.fifore(gpufifore),
	.fifovalid(gpufifovalid),
	.devicestate(vblankcount));

// TODO: Use sdconn switch state to trigger a card detect IRQ
axi4spi spictlinst(
	.aclk(aclk),
	.aresetn(aresetn),
	.spibaseclock(clk100),
	.sdconn(sdconn),
	.s_axi(spiif));

// CSR file acts as a region of uncached memory
// Access to register indices get mapped to LOAD/STORE
// and addresses get mapped starting at 0x80004000 + csroffset
// CSR module also acts as the interrupt generator
axi4CSRFile csrfileinst(
	.aclk(aclk),
	.aresetn(aresetn),
	.cpuclocktime(cpuclocktime),
	.wallclocktime(wallclocktime),
	.retired(retired),
	// IRQ tracking
	.irqReq(irqReq),
	// External interrupt wires
	.uartrcvempty(uartrcvempty),
	//.usbint(usbint),	// TODO: interrupt from USB host
	// Shadow registers
	.mepc(mepc),
	.mtvec(mtvec),
	.sie(sie),
	// Bus
	.s_axi(csrif) );

// XADC
axi4xadc xadcinst(
	.aclk(aclk),
	.clk10(clk10),
	.aresetn(aresetn),
	.s_axi(xadcif),
	.adcconn(adcconn),
	.device_temp(device_temp) );

// DMA command queue
commandqueue dmacmdinst(
	.aclk(aclk),
	.aresetn(aresetn),
	.s_axi(dmaif),
	// Internal comms channel for DMA
	.fifoempty(dmafifoempty),
	.fifodout(dmafifodout),
	.fifore(dmafifore),
	.fifovalid(dmafifovalid),
    .devicestate({31'd0,dmabusy}));

// Audio command queue
commandqueue audiocmdinst(
	.aclk(aclk),
	.aresetn(aresetn),
	.s_axi(audioif),
	// Internal comms channel for APU
	.fifoempty(audiofifoempty),
	.fifodout(audiofifodout),
	.fifore(audiofifore),
	.fifovalid(audiofifovalid),
    .devicestate(audiobufferswapcount));

// USB Host
// TODO: Wire usb_d_p & usb_d_n pair and usbint here
dummydevice usbhostcontroller(
	.aclk(alck),
	.aresetn(aresetn),
	.s_axi(usbif));

endmodule
