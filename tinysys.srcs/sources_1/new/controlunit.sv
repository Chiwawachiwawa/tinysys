`timescale 1ns / 1ps

import axi4pkg::*;

`include "shared.vh"

module controlunit #(
	parameter int CID = 32'h00000000 // Corresponds to HARTID
) (
	input wire aclk,
	input wire aresetn,
	output wire branchresolved,
	output wire [31:0] branchtarget,
	// Instruction FIFO control
	input wire ififoempty,
	input wire ififovalid,
	input wire [131:0] ififodout,
	output wire ififord_en,
	// CPU cycle / retired instruction counts
	output wire [63:0] cpuclocktime,
	output wire [63:0] retired,
	// Internal bus to data unit
	ibusif.master m_ibus);

logic ififore = 1'b0;
assign ififord_en = ififore;

logic [63:0] cyclecount = 64'd0;
logic [63:0] retiredcount = 64'd0;
assign cpuclocktime = cyclecount;
assign retired = retiredcount;

logic [31:0] PC;
logic [17:0] instrOneHotOut;
logic [3:0] aluop;
logic [2:0] bluop;
logic [2:0] func3;
logic [11:0] func12;
logic [4:0] rs1;
logic [4:0] rs2;
logic [4:0] rd;
logic [11:0] csroffset;
logic [31:0] immed;
logic selectimmedasrval2;
logic [31:0] csrprevval = 32'd0;

logic btready = 1'b0;
logic [31:0] btarget = 32'd0;

assign branchresolved = btready;
assign branchtarget = btarget;

// Operands for exec
logic [31:0] A; // rval1
logic [31:0] B; // rval2
logic [31:0] C; // rval2 : immed
logic [31:0] D; // immed

// Writeback data
logic [31:0] wbdin;
logic [4:0] wbdest;
logic wback;

// --------------------------------------------------
// Register file
// --------------------------------------------------

wire [31:0] rval1;
wire [31:0] rval2;
logic [31:0] rdin;
logic rwen = 1'b0;
integerregisterfile registerfileinst(
	.clock(aclk),
	.rs1(rs1),
	.rs2(rs2),
	.rd(wbdest),
	.wren(rwen),
	.din(rdin),
	.rval1(rval1),
	.rval2(rval2) );

// --------------------------------------------------
// Branch logic
// --------------------------------------------------

wire branchout;
branchlogic branchlogicinst(
	.branchout(branchout),
	.val1(A),
	.val2(B),
	.bluop(bluop) );

// --------------------------------------------------
// Arithmetic
// --------------------------------------------------

wire [31:0] aluout;
arithmeticlogic arithmeticlogicinst(
	.aluout(aluout),
	.val1(A),
	.val2(C),
	.aluop(aluop) );

// --------------------------------------------------
// IMUL/IDIV
// --------------------------------------------------

logic mulstrobe = 1'b0;
logic divstrobe = 1'b0;

wire mulready;
wire [31:0] product;
integermultiplier IMULSU(
    .aclk(aclk),
    .aresetn(aresetn),
    .start(mulstrobe),
    .ready(mulready),
    .func3(func3),
    .multiplicand(A),
    .multiplier(B),
    .product(product) );

wire divuready;
wire [31:0] quotientu, remainderu;
integerdividerunsigned IDIVU (
	.aclk(aclk),
	.aresetn(aresetn),
	.start(divstrobe),
	.ready(divuready),
	.dividend(A),
	.divisor(B),
	.quotient(quotientu),
	.remainder(remainderu) );

wire divready;
wire [31:0] quotient, remainder;
integerdividersigned IDIVS (
	.aclk(aclk),
	.aresetn(aresetn),
	.start(divstrobe),
	.ready(divready),
	.dividend(A),
	.divisor(B),
	.quotient(quotient),
	.remainder(remainder) );

// --------------------------------------------------
// Core logic
// --------------------------------------------------

typedef enum logic [4:0] {
	READINSTR, READREG,
	DISPATCH,
	MATHWAIT, LWAIT, SWAIT,
	SYSOP, SYSWBACK, SYSWAIT,
	CSROPS, WCSROP,
	SYSIFENCE, SYSCDISCARD, SYSCFLUSH, WCACHE,
	SYSMRET, SYSWFI,
	WBACK } controlunitmode;
controlunitmode ctlmode = READINSTR;

logic [31:0] rwaddress = 32'd0;
logic [31:0] offsetPC = 32'd0;
logic [31:0] adjacentPC = 32'd0;

always @(posedge aclk) begin
	if (~aresetn) begin
		// cyclecount <= 64'd0; ?
	end else begin
		// TODO: Stop this if CPU's halted for debug
		cyclecount <= cyclecount + 64'd1;
	end
end

/*m_ibus.waddr <= {20'h80004, `CSR_MIP};
m_ibus.wstrobe <= 4'b1111;
//m_ibus.wop <= `WOP_NOTAND;
// Clear interrupt pending bit based on what we just handled
case (mcause)
32'h8000000b:	csrext <= 32'h00000800; // IRQ
32'h80000007:	csrext <= 32'h00000080; // TRQ
default:		csrext <= 32'h00000008; // SWI
endcase*/

// Entering ISR:
// 1- Fetch unit detects interrupts, passes IRQ bits as part of instruction output, halts for branch resolution
// 2- Control unit ORs in incoming IRQ bits and updates MIP (wstrobe, wop=OR)
// 3- Control unit saves adjacentPC into MEPC (wstrobe, wop=REPLACE)
// 4- Control unit saves interrupt cause value into MCAUSE (wstrobe, wop=REPLACE)
// 5- Control unit saves relevant data into MTVAL (wstrobe, wop=REPLACE)
// 6- Control unit sets branch target to MTVEC and flags branch resolved (wstrobe, wop=REPLACE)
// Returning from ISR:
// 1- Fetch unit detects MRET, halts for branch resolution
// 2- Control unit clears bits for current interrupt and updates MIP (wstrobe, wop=AND)
// 3- Control unit reads MEPC (rstrobe)
// 4- Control unit sets branch target to MEPC and flags branch resolved

always @(posedge aclk) begin
	if (~aresetn) begin

		m_ibus.raddr <= 32'd0;
		m_ibus.waddr <= 32'd0;
		m_ibus.rstrobe <= 1'b0;
		m_ibus.wstrobe <= 4'h0;
		m_ibus.cstrobe <= 1'b0;
		m_ibus.dcacheop <= 2'b0;
		
		ififore <= 1'b0;
		rwen <= 1'b0;
		wback <= 1'b0;

		retiredcount <= 64'd0;

	end else begin

		btready <= 1'b0;
		ififore <= 1'b0;
		rwen <= 1'b0;
		wback <= 1'b0;

		m_ibus.rstrobe <= 1'b0;
		m_ibus.wstrobe <= 4'h0;
		m_ibus.cstrobe <= 1'b0;

		mulstrobe <= 1'b0;
		divstrobe <= 1'b0;

		unique case(ctlmode)
			READINSTR: begin
				// TODO: Fetch unit can halt / resume / debug the CPU via special commands
				{ PC, csroffset, instrOneHotOut,
					aluop, bluop, func3, func12,
					rs1, rs2, rd,
					selectimmedasrval2, immed} <= ififodout;
				ififore <= (ififovalid && ~ififoempty);
				ctlmode <= (ififovalid && ~ififoempty) ? READREG : READINSTR;
			end

			READREG: begin
				A <= rval1;
				B <= rval2;
				C <= selectimmedasrval2 ? immed : rval2;
				D <= immed;
				wbdest <= rd;
				rwaddress <= rval1 + immed;
				offsetPC <= PC + immed;
				adjacentPC <= PC + 32'd4;
				mulstrobe <= (aluop==`ALU_MUL);
				divstrobe <= (aluop==`ALU_DIV || aluop==`ALU_REM);
				ctlmode <= DISPATCH;
			end

			DISPATCH: begin
				retiredcount <= retiredcount + 64'd1;

				unique case(1'b1)
					instrOneHotOut[`O_H_OP],
					instrOneHotOut[`O_H_OP_IMM]: begin
						wbdin <= aluout;
						wback <= 1'b1;
					end
					instrOneHotOut[`O_H_AUIPC],
					instrOneHotOut[`O_H_LUI]: begin
						wbdin <= instrOneHotOut[`O_H_AUIPC] ? offsetPC : D;
						wback <= 1'b1;
					end
					instrOneHotOut[`O_H_STORE]: begin
						m_ibus.waddr <= rwaddress;
						unique case(func3)
							3'b000:  m_ibus.wdata <= {B[7:0], B[7:0], B[7:0], B[7:0]};
							3'b001:  m_ibus.wdata <= {B[15:0], B[15:0]};
							default: m_ibus.wdata <= B;
						endcase
						unique case(func3)
							3'b000:  m_ibus.wstrobe <= {rwaddress[1]&rwaddress[0], rwaddress[1]&(~rwaddress[0]), (~rwaddress[1])&rwaddress[0], (~rwaddress[1])&(~rwaddress[0])};
							3'b001:  m_ibus.wstrobe <= {rwaddress[1], rwaddress[1], ~rwaddress[1], ~rwaddress[1]};
							default: m_ibus.wstrobe <= 4'b1111;
						endcase
					end
					instrOneHotOut[`O_H_LOAD]: begin
						m_ibus.raddr <= rwaddress;
						m_ibus.rstrobe <= 1'b1;
					end
					instrOneHotOut[`O_H_JAL],
					instrOneHotOut[`O_H_JALR]: begin
						wbdin <= adjacentPC;
						wback <= 1'b1;
						btarget <= instrOneHotOut[`O_H_JAL] ? offsetPC : rwaddress;
						btready <= 1'b1;
					end
					instrOneHotOut[`O_H_BRANCH]: begin
						btarget <= branchout ? offsetPC : adjacentPC;
						btready <= 1'b1;
					end
				endcase

				ctlmode <=	(mulstrobe || divstrobe) ? MATHWAIT : (instrOneHotOut[`O_H_FENCE] ? SYSIFENCE : (instrOneHotOut[`O_H_SYSTEM] ? SYSOP : (instrOneHotOut[`O_H_STORE] ? SWAIT : ( instrOneHotOut[`O_H_LOAD] ? LWAIT : WBACK))));
			end

			MATHWAIT: begin
				if (mulready || divready) begin
					unique case (aluop)
						`ALU_MUL: begin
							wbdin <= product;
						end
						`ALU_DIV: begin
							wbdin <= (func3 == `F3_DIV) ? quotient : quotientu;
						end
						`ALU_REM: begin
							wbdin <= (func3 == `F3_REM) ? remainder : remainderu;
						end
					endcase
					wback <= 1'b1;
					ctlmode <= WBACK;
				end else begin
					ctlmode <= MATHWAIT;
				end
			end

			LWAIT: begin
				unique case(func3)
					3'b000: begin // BYTE with sign extension
						unique case(rwaddress[1:0])
							2'b11: begin wbdin <= {{24{m_ibus.rdata[31]}}, m_ibus.rdata[31:24]}; end
							2'b10: begin wbdin <= {{24{m_ibus.rdata[23]}}, m_ibus.rdata[23:16]}; end
							2'b01: begin wbdin <= {{24{m_ibus.rdata[15]}}, m_ibus.rdata[15:8]}; end
							2'b00: begin wbdin <= {{24{m_ibus.rdata[7]}}, m_ibus.rdata[7:0]}; end
						endcase
					end
					3'b001: begin // HALF with sign extension
						unique case(rwaddress[1])
							1'b1: begin wbdin <= {{16{m_ibus.rdata[31]}}, m_ibus.rdata[31:16]}; end
							1'b0: begin wbdin <= {{16{m_ibus.rdata[15]}}, m_ibus.rdata[15:0]}; end
						endcase
					end
					3'b100: begin // BYTE with zero extension
						unique case(rwaddress[1:0])
							2'b11: begin wbdin <= {24'd0, m_ibus.rdata[31:24]}; end
							2'b10: begin wbdin <= {24'd0, m_ibus.rdata[23:16]}; end
							2'b01: begin wbdin <= {24'd0, m_ibus.rdata[15:8]}; end
							2'b00: begin wbdin <= {24'd0, m_ibus.rdata[7:0]}; end 
						endcase
					end
					3'b101: begin // HALF with zero extension
						unique case(rwaddress[1])
							1'b1: begin wbdin <= {16'd0, m_ibus.rdata[31:16]}; end
							1'b0: begin wbdin <= {16'd0, m_ibus.rdata[15:0]}; end
						endcase
					end
					3'b010: begin // WORD
						wbdin <= m_ibus.rdata;
					end
				endcase
				wback <= m_ibus.rdone;
				ctlmode <= m_ibus.rdone ? WBACK : LWAIT;
			end

			SWAIT: begin
				// This one does not visit WBACK mode
				ctlmode <= m_ibus.wdone ? READINSTR : SWAIT;
			end

			SYSOP: begin
				case ({func3, func12})
					{3'b000, `F12_CDISCARD}:	ctlmode <= SYSCDISCARD;
					{3'b000, `F12_CFLUSH}:		ctlmode <= SYSCFLUSH;
					{3'b000, `F12_MRET}:		ctlmode <= SYSMRET;
					{3'b000, `F12_WFI}:			ctlmode <= SYSWFI;
					{3'b000, `F12_EBREAK}:		ctlmode <= READINSTR;
					{3'b000, `F12_ECALL}:		ctlmode <= READINSTR;
					default:					ctlmode <= CSROPS;
				endcase
			end

			SYSIFENCE: begin
				// TODO:
				ctlmode <= READINSTR;
			end

			SYSCDISCARD: begin
				m_ibus.dcacheop <= 2'b01; // {nowb,iscachecmd}
				m_ibus.cstrobe <= 1'b1;
				ctlmode <= WCACHE;
			end

			SYSCFLUSH: begin
				m_ibus.dcacheop <= 2'b11; // {wb,iscachecmd}
				m_ibus.cstrobe <= 1'b1;
				ctlmode <= WCACHE;
			end

			WCACHE: begin
				// Wait for pending cache operation to complete and unblock fetch unit
				btarget <= adjacentPC;
				btready <= m_ibus.cdone;
				ctlmode <= m_ibus.cdone ? READINSTR : WCACHE;
			end

			SYSMRET: begin

				// 1- read MEPC
				// 2- set branch target

				// Jump to saved PC
				// NOTE: Return address must be set up to be +2/+4 of the interrupt site's PC beforehand
				//btarget <= mepc;
				//btready <= 1'b1;

				ctlmode <= READINSTR;
			end

			SYSWFI: begin
				// TODO: Wait for any incoming IRQ bit set and signal branch ready on next address
				//btarget <= adjacentPC;
				//btready <= 1'b1;
				ctlmode <= READINSTR;
			end

			CSROPS: begin
				m_ibus.raddr <= {20'h80004, csroffset};
				m_ibus.rstrobe <= 1'b1;
				ctlmode <= WCSROP;
			end

			WCSROP: begin
				if (m_ibus.rdone) begin
					csrprevval <= m_ibus.rdata;
				end
				ctlmode <= m_ibus.rdone ? SYSWBACK : WCSROP;
			end

			SYSWBACK: begin
				// Update CSR register with read value
				m_ibus.waddr <= {20'h80004, csroffset};
				m_ibus.wstrobe <= 4'b1111;
				unique case (func3)
					3'b001: begin
						m_ibus.wdata <= A;
					end
					3'b101: begin
						m_ibus.wdata <= D;
					end
					3'b010: begin
						m_ibus.wdata <= csrprevval | A;
					end
					3'b110: begin
						m_ibus.wdata <= csrprevval | D;
					end
					3'b011: begin
						m_ibus.wdata <= csrprevval & (~A);
					end
					3'b111: begin
						m_ibus.wdata <= csrprevval & (~D);
					end
					default: begin // Unknown
						m_ibus.wdata <= csrprevval;
					end
				endcase

				// Wait for CSR writeback
				ctlmode <= SYSWAIT;
			end

			SYSWAIT: begin
				ctlmode <= m_ibus.wdone ? WBACK : SYSWAIT;

				// Store old CSR value in wbdest
				wbdin <= csrprevval;
				wback <= m_ibus.wdone;
			end

			WBACK: begin
				rdin <= wbdin;
				rwen <= wback;
				ctlmode <= READINSTR;
			end
		endcase
	end
end

endmodule
