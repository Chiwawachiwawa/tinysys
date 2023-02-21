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
	input wire [119:0] ififodout,
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
// Core logic
// --------------------------------------------------

typedef enum logic [3:0] {READINSTR, READREG, DISPATCH, LWAIT, SWAIT, SYSOP, SYSWBACK, SYSWAIT, SYSCACHE, WBACK} controlunitmode;
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

always @(posedge aclk) begin
	if (~aresetn) begin

		m_ibus.raddr <= 32'd0;
		m_ibus.waddr <= 32'd0;
		m_ibus.rstrobe <= 1'b0;
		m_ibus.wstrobe <= 4'h0;
		
		ififore <= 1'b0;
		rwen <= 1'b0;
		wback <= 1'b0;

	end else begin

		btready <= 1'b0;
		ififore <= 1'b0;
		rwen <= 1'b0;
		wback <= 1'b0;

		m_ibus.rstrobe <= 1'b0;
		m_ibus.wstrobe <= 4'h0;

		unique case(ctlmode)
			READINSTR: begin
				// TODO: Fetch unit can halt / resume / debug the CPU via special commands
				{ PC, csroffset, instrOneHotOut,
					aluop, bluop, func3,
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
					instrOneHotOut[`O_H_SYSTEM]: begin
						// Set to CSR address range at 0x80004000 and trigger a read regardless of the op
						m_ibus.raddr <= {20'h80004, csroffset};
						m_ibus.rstrobe <= 1'b1;
					end
				endcase
				ctlmode <= instrOneHotOut[`O_H_SYSTEM] ? SYSOP : (instrOneHotOut[`O_H_STORE] ? SWAIT : ( instrOneHotOut[`O_H_LOAD] ? LWAIT : WBACK));
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
				ctlmode <= m_ibus.wdone ? READINSTR : SWAIT;
			end

			SYSOP: begin
				// Wait for CSR load
				if (m_ibus.rdone) begin
					csrprevval <= m_ibus.rdata;
				end
				// When done, either jump to cache op or csr wback
				// We waste a load here for simplicity during cache ops
				ctlmode <= m_ibus.rdone ? (func3 == 3'b000 ? SYSCACHE : SYSWBACK) : SYSOP;
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

			SYSCACHE: begin
				// TODO: Cache op ignored for now
				ctlmode <= WBACK;
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
