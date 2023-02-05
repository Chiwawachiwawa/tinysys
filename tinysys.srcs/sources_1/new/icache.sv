`timescale 1ns / 1ps

`include "shared.vh"

module instructioncache(
	input wire aclk,
	input wire aresetn,
	input wire icacheflush,
	// custom bus to cpu
	input wire [31:0] addr,
	output wire [31:0] dout,
	input wire ren,
	output wire rready,
	axi4if.master m_axi);

wire [7:0] line = addr[13:6];	// Cache line
wire [16:0] tag = addr[30:14];	// Cache tag
wire [3:0] offset = addr[5:2];	// Cache word offset

logic readdone = 1'b0;
assign rready = readdone;

logic [31:0] dataout = 32'd0;
assign dout = dataout;

logic [31:0] cacheaddress;
data_t cachedin[0:3];
logic memwritestrobe = 1'b0;
logic memreadstrobe = 1'b0;

logic [16:0] ctag;					// current cache tag (17 bits)
logic [3:0] coffset;				// current word offset 0..15
logic [7:0] cline;					// current cache line 0..256
logic chit = 1'b0;

logic cachelinevalid[0:255];		// cache line state (invalid / valid)
logic [16:0] cachelinetags[0:255];	// cache line tags (17 bits)

logic [63:0] cachewe = 64'd0;		// byte select for 64 byte cache line
logic [511:0] cdin;					// input data to write to cache
wire [511:0] cdout;					// output data read from cache

logic flushing = 1'b0;				// high during cache flush operation
logic [7:0] dccount = 8'h00;		// line counter for cache flush/invalidate ops
logic [16:0] flushline = 18'd0;		// contents of line being flushed

logic [7:0] cacheaccess;
always_comb begin
	if (flushing)
		cacheaccess = dccount;
	else
		cacheaccess = line;
end

wire rsta_busy;
cachemem CacheMemory512(
	.addra(cacheaccess),		// current cache line
	.clka(aclk),				// cache clock
	.dina(cdin),				// updated cache data to write
	.wea(cachewe),				// write strobe for current cache line
	.douta(cdout),				// output of currently selected cache line
	.rsta(~aresetn),			// Reset
	.rsta_busy(rsta_busy) );	// Reset busy

initial begin
	integer i;
	for (int i=0; i<256; i=i+1) begin	// 256 lines total
		cachelinevalid[i] = 'd0;		// contents invalid
		cachelinetags[i] = 'd0;			// start of memory
	end
end

// ----------------------------------------------------------------------------
// cached/uncached memory controllers
// ----------------------------------------------------------------------------

wire rdone;
cachedmemorycontroller cachedmemorycontrollerinst(
	.aclk(aclk),
	.aresetn(aresetn),
	// From cache
	.addr(cacheaddress),
	.din(),
	.dout(cachedin),
	.start_read(memreadstrobe),
	.start_write(1'b0),
	.wdone(),
	.rdone(rdone),
	// To memory
	.m_axi(m_axi) );

typedef enum logic [4:0] {
	IDLE,
	CREAD,
	CPOPULATE, CPOPULATEWAIT, CUPDATE, CUPDATEDELAY,
	INVALIDATEBEGIN, INVALIDATESTEP} cachestatetype;
cachestatetype cachestate = IDLE;

always_ff @(posedge aclk) begin
	if (~aresetn) begin
		cachestate <= IDLE;
		memreadstrobe <= 1'b0;
	end else begin
		memreadstrobe <= 1'b0;
		readdone <= 1'b0;
		cachewe <= 64'd0;

		unique case(cachestate)
			IDLE : begin
				coffset <= offset;							// Cache offset 0..15
				cline <= line;								// Cache line
				ctag <= tag;								// Cache tag 00000..1ffff
				dccount <= 8'h00;
				chit <= (tag == cachelinetags[line]) && cachelinevalid[line];
				cachestate <= icacheflush ? INVALIDATEBEGIN : (ren ? CREAD : IDLE);
			end
			
			INVALIDATEBEGIN: begin
				// Invalidate
				cachelinetags[dccount] <= 'd0;
				cachelinevalid[dccount] <= 1'b0;
				cachestate <= INVALIDATESTEP;
			end

			INVALIDATESTEP: begin
				dccount <= dccount + 8'd1;
				readdone <= dccount == 8'hFF;
				cachestate <= dccount == 8'hFF ? IDLE : INVALIDATEBEGIN;
			end

			CREAD: begin
				if (chit) begin // Hit
					unique case(coffset)
						4'b0000:  dataout <= cdout[31:0];
						4'b0001:  dataout <= cdout[63:32];
						4'b0010:  dataout <= cdout[95:64];
						4'b0011:  dataout <= cdout[127:96];
						4'b0100:  dataout <= cdout[159:128];
						4'b0101:  dataout <= cdout[191:160];
						4'b0110:  dataout <= cdout[223:192];
						4'b0111:  dataout <= cdout[255:224];
						4'b1000:  dataout <= cdout[287:256];
						4'b1001:  dataout <= cdout[319:288];
						4'b1010:  dataout <= cdout[351:320];
						4'b1011:  dataout <= cdout[383:352];
						4'b1100:  dataout <= cdout[415:384];
						4'b1101:  dataout <= cdout[447:416];
						4'b1110:  dataout <= cdout[479:448];
						4'b1111:  dataout <= cdout[511:480];
					endcase
					readdone <= 1'b1;
					cachestate <= IDLE;
				end else begin // Miss
					cachestate <= CPOPULATE;
				end
			end

			CPOPULATE : begin
				// Same as current memory address with device selector, aligned to cache boundary, top bit ignored (cached address)
				cacheaddress <= {1'b0, ctag, cline, 6'd0};
				memreadstrobe <= 1'b1;
				cachestate <= CPOPULATEWAIT;
			end

			CPOPULATEWAIT: begin
				cachestate <= rdone ? CUPDATE : CPOPULATEWAIT;
			end

			CUPDATE: begin
				chit <= 1'b1;
				cachewe <= 64'hFFFFFFFFFFFFFFFF; // All entries
				cdin <= {cachedin[3], cachedin[2], cachedin[1], cachedin[0]}; // Data from memory
				cachestate <= CUPDATEDELAY;
			end

			CUPDATEDELAY: begin
				cachelinetags[cline] <= ctag;
				cachelinevalid[cline] <= 1'b1;
				cachestate <= CREAD;
			end
		endcase
	end
end

endmodule
