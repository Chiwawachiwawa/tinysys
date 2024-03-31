`timescale 1ns / 1ps

module tinysoc #(
	parameter int RESETVECTOR = 32'd0
) (
	input wire aclk,
	input wire clk10,
	input wire clkaudio,
	input wire clk25,
	input wire clk50,
	input wire clk100,
	input wire clk166,
	input wire clk200,
	input wire aresetn,
	input wire preresetn,
	// LEDs
	output wire [3:0] leds,
	// ESP32
	inout wire [18:0] esp_io,
	// Video output
	output wire vvsync,
	output wire vhsync,
	output wire vclk,
	output wire vde,
	output wire [11:0] vdat,
	// DDR3
	ddr3sdramwires.def ddr3conn,
	// I2S
	audiowires.def i2sconn,
	// SDCard
	sdcardwires.def sdconn,
	// USB-C (peripheral)
	max3420wires.def usbcconn,
	// USB-A (host)
	max3420wires.def usbaconn );

// --------------------------------------------------
// Bus lines
// --------------------------------------------------

axi4if instructionbusHart0();	// Fetch bus for HART#0
axi4if databusHart0();			// Data bus for HART#0
axi4if devicebusHart0();		// Direct access to devices from data unit of HART#0

axi4if videobus();				// Video output unit bus
axi4if dmabus();				// Direct memory access bus
axi4if romcopybus();			// Bus for boot ROM copy (TODO: switch between ROM types?)
axi4if audiobus();				// Bus for audio device output

axi4if memorybus();				// Arbitrated access to main memory

axi4if gpioif();				// Sub bus: GPIO port
axi4if ledif();					// Sub bus: LED contol device (debug LEDs)
axi4if vpucmdif();				// Sub bus: VPU command fifo (video scan out logic)
axi4if spiif();					// Sub bus: SPI control device (sdcard)
axi4if csrif();					// Sub bus: CSR file device (control registers)
axi4if xadcif();				// Sub bus: ADC controller (temperature sensor)
axi4if dmaif();					// Sub bus: DMA controller (memcopy)
axi4if usbcif();				// Sub bus: USB-C controller (usb peripheral interface)
axi4if audioif();				// Sub bus: APU i2s audio output unit (raw audio)
axi4if usbaif();				// Sub bus: USB-A controller (usb host interface)

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
	wallclocktimeA <= wallclockcounter;
	wallclocktime <= wallclocktimeA;
end

// --------------------------------------------------
// HART#0
// --------------------------------------------------

wire romReady, sieHart0;
wire [63:0] cpuclocktimeHart0;
wire [63:0] retiredHart0;
wire [31:0] mepcHart0;
wire [31:0] mtvecHart0;
wire [1:0] irqReqHart0;

riscv #(.RESETVECTOR(RESETVECTOR)) hart0(
	.aclk(aclk),
	.aresetn(aresetn),
	.romReady(romReady),
	.sie(sieHart0),
	.mepc(mepcHart0),
	.mtvec(mtvecHart0),
	.irqReq(irqReqHart0),
	.instructionbus(instructionbusHart0),
	.databus(databusHart0),
	.devicebus(devicebusHart0),
	.cpuclocktime(cpuclocktimeHart0),
	.retired(retiredHart0));

// --------------------------------------------------
// Video output unit
// --------------------------------------------------

wire vpufifoempty;
wire [31:0] vpufifodout;
wire vpufifore;
wire vpufifovalid;
wire [31:0] vpustate;
videocore VPU(
	.aclk(aclk),
	.clk25(clk25),
	.aresetn(aresetn),
	.m_axi(videobus),
	.vvsync(vvsync),
	.vhsync(vhsync),
	.vclk(vclk),
	.vde(vde),
	.vdat(vdat),
	.vpufifoempty(vpufifoempty),
	.vpufifodout(vpufifodout),
	.vpufifore(vpufifore),
	.vpufifovalid(vpufifovalid),
	.vpustate(vpustate));

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

axi4romcopy #(.RESETVECTOR(RESETVECTOR)) COPYROM(
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
    .audioclock(clkaudio),		// 22.591MHz master clock

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

arbiter arbiter6x1instSDRAM(
	.aclk(aclk),
	.aresetn(aresetn),
	.axi_s({romcopybus, audiobus, dmabus, videobus, databusHart0, instructionbusHart0}),
	.axi_m(memorybus) );

// --------------------------------------------------
// RAM
// --------------------------------------------------

wire [11:0] device_temp;

// dev   start     end      size
// DDR3: 00000000  0FFFFFFF (256 Mbytes, 16 byte r/w only)

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

// 12bit (4K) address space reserved for each memory mapped device (255 devices max)
// dev   start     end       addrs[19:12]  size  notes
// GPIO: 80000000  8xx00FFF  8'b0000_0000  4KB	GPIO pins
// LEDS: 8xx01000  8xx01FFF  8'b0000_0001  4KB	Debug LEDs
// VPUC: 8xx02000  8xx02FFF  8'b0000_0010  4KB	Video Processing Unit
// SDCC: 8xx03000  8xx03FFF  8'b0000_0011  4KB	SDCard SPI Unit
// XADC: 8xx04000  8xx04FFF  8'b0000_0100  4KB	Die Temperature DAC
// DMAC: 8xx05000  8xx05FFF  8'b0000_0101  4KB	Direct Memory Access / Memcopy
// USBC: 8xx06000  8xx06FFF  8'b0000_0110  4KB	USB-C Peripheral Interface Unit
// APUC: 8xx07000  8xx07FFF  8'b0000_0111  4KB	Audio Processing Unit / Mixer
// USBA: 8xx08000  8xx08FFF  8'b0000_1000  4KB	USB-A Host Interface Unit
// CSR0: 8xx09000  8xx09FFF  8'b0000_1001  4KB	HART#0
// ----: 8xx0A000  8xx0AFFF  8'b0000_1010  4KB	Unused
// ----: 8xx0B000  8xx0BFFF  8'b0000_1011  4KB	Unused
// ----: 8xx0C000  8xx0CFFF  8'b0000_1100  4KB	Unused
// ----: 8xx0D000  8xx0DFFF  8'b0000_1101  4KB	Unused
// ----: 8xx0E000  8xx0EFFF  8'b0000_1110  4KB	Unused
// ----: 8xx0F000  8xx0FFFF  8'b0000_1111  4KB	Unused
// ----: 8xx10000  8xx10FFF  8'b0001_0000  4KB	Unused

devicerouter devicerouterinst(
	.aclk(aclk),
	.aresetn(aresetn),
    .axi_s(devicebusHart0),				// TODO: Will need this to come from a bus arbiter
    .addressmask({
		8'b0000_1001,		// CRS0 CSR file for HART#0
    	8'b0000_1000,		// USBA USB-A access via SPI
    	8'b0000_0111,		// APUC	Audio Processing Unit Command Fifo
        8'b0000_0110,		// USBC USB-C access via SPI
    	8'b0000_0101,		// DMAC DMA Command Fifo
    	8'b0000_0100,		// XADC Analog / Digital Converter Interface
		8'b0000_0011,		// SDCC SDCard access via SPI
		8'b0000_0010,		// VPUC Graphics Processing Unit Command Fifo
		8'b0000_0001,		// LEDS Debug / Status LED interface
		8'b0000_0000}),		// GPIO Input/output pins to ESP32 module
    .axi_m({csrif, usbaif, audioif, usbcif, dmaif, xadcif, spiif, vpucmdif, ledif, gpioif}));

// --------------------------------------------------
// Interrupt wires
// --------------------------------------------------

wire keyfifoempty;
wire usbcirq, usbairq;

// --------------------------------------------------
// Memory mapped devices
// --------------------------------------------------

axi4gpio gpiodevice(
	.aclk(aclk),
	.aresetn(aresetn),
	.s_axi(gpioif),
	.gpio(esp_io) );

axi4led leddevice(
	.aclk(aclk),
	.aresetn(aresetn),
	.s_axi(ledif),
	.led(leds) );

commandqueue vpucmdinst(
	.aclk(aclk),
	.aresetn(aresetn),
	.s_axi(vpucmdif),
	.fifoempty(vpufifoempty),
	.fifodout(vpufifodout),
	.fifore(vpufifore),
	.fifovalid(vpufifovalid),
	.devicestate(vpustate));

axi4sdcard sdcardinst(
	.aclk(aclk),
	.clk10(clk10),
	.aresetn(aresetn),
	.spibaseclock(clk50),
	.sdconn(sdconn),
	.keyfifoempty(keyfifoempty),
	.s_axi(spiif));

axi4usbc usbserialport(
	.aclk(aclk),
	.aresetn(aresetn),
	.spibaseclock(clk50),
	.usbcconn(usbcconn),
	.usbirq(usbcirq),
	.s_axi(usbcif));

axi4usbc usbhostport(
	.aclk(aclk),
	.aresetn(aresetn),
	.spibaseclock(clk50),
	.usbcconn(usbaconn),
	.usbirq(usbairq),
	.s_axi(usbaif));

// CSR file acts as a region of uncached memory
// Access to register indices get mapped to LOAD/STORE
// and addresses get mapped starting at 0x8000B000 + csroffset
// CSR module also acts as the interrupt generator
axi4CSRFile csrfileinstHart0(
	.aclk(aclk),
	.aresetn(aresetn),
	.cpuclocktime(cpuclocktimeHart0),
	.wallclocktime(wallclocktime),
	.retired(retiredHart0),
	// IRQ tracking
	.irqReq(irqReqHart0),
	// External interrupt wires
	.keyfifoempty(keyfifoempty),
	.usbirq({usbairq, usbcirq}),
	// Soft reset
	//.sysresetn(sysresetn),
	// Shadow registers
	.mepc(mepcHart0),
	.mtvec(mtvecHart0),
	.sie(sieHart0),
	// Bus
	.s_axi(csrif) );

// XADC
axi4xadc xadcinst(
	.aclk(aclk),
	.clk10(clk10),
	.aresetn(aresetn),
	.s_axi(xadcif),
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

endmodule
