`timescale 1ns / 1ps

// --------------------------------------------------
// Unsigned divider
// --------------------------------------------------

module integerdividerunsigned(
    input wire aclk,
    input wire aresetn,
    input wire start,
    input wire [31:00] dividend,
    input wire [31:00] divisor,
    output logic [31:00] quotient,
    output logic [31:00] remainder,
    output wire ready);

logic [5:0] count;
logic [31:0] reg_q;
logic [31:0] reg_r;
logic [31:0] reg_b;
logic rdy = 1'b0;
logic r_sign;
assign ready = rdy;

typedef enum logic [1:0] {INIT, WCMD, DIVLOOP} divcmdmodetype;
divcmdmodetype cmdmode = INIT;

wire [32:0] sub_addn = ({reg_r,reg_q[31]}+{1'b0,reg_b});
wire [32:0] sub_addm = ({reg_r,reg_q[31]}-{1'b0,reg_b});
wire [31:0] rem = r_sign ? reg_r + reg_b : reg_r;
wire [31:0] quo = reg_q;

always @(posedge aclk) begin

    count <= count+1;
    rdy <= 1'b0;

    unique case (cmdmode)
        INIT: begin
            reg_r <= 32'b0;
            cmdmode <= WCMD;
        end
        WCMD: begin
            r_sign <= 0;
            reg_q <= dividend;
            reg_b <= divisor;
            count <= 0;
            cmdmode <= start ? DIVLOOP : WCMD;
        end
        DIVLOOP: begin
        	unique case (r_sign)
        		1'b1: begin
            		reg_r <= sub_addn[31:0];
    		        r_sign <= sub_addn[32];
	            	reg_q <= {reg_q[30:0], ~sub_addn[32]};
        		end
        		1'b0: begin
            		reg_r <= sub_addm[31:0];
    		        r_sign <= sub_addm[32];
	            	reg_q <= {reg_q[30:0], ~sub_addm[32]};
        		end
        	endcase
            rdy <= count[5];
            cmdmode <= count[5] ? WCMD : DIVLOOP;
            if (count[5]) begin
                remainder <= rem;
                quotient <= quo;
                reg_r <= 32'b0;
            end else begin
                remainder <= 32'd0;
                quotient <= 32'd0;
            end
        end
    endcase

    if (~aresetn) begin
        cmdmode <= INIT;
    end
end

endmodule

// --------------------------------------------------
// Signed divider
// --------------------------------------------------

module integerdividersigned(
    input wire aclk,
    input wire aresetn,
    input wire start,
    input wire [31:0] dividend,
    input wire [31:0] divisor,
    output logic [31:0] quotient,
    output logic [31:0] remainder,
    output wire ready);

logic [5:0] count;
logic [31:0] reg_q;
logic [31:0] reg_r;
logic [31:0] reg_b;
logic rdy = 1'b0;
logic r_sign;
logic negd;
logic negq;
assign ready = rdy;

typedef enum logic [1:0] {INIT, WCMD, DIVLOOP} divcmdmodetype;
divcmdmodetype cmdmode = INIT;

wire [32:0] sub_addn = ({reg_r,reg_q[31]}+{1'b0,reg_b});
wire [32:0] sub_addm = ({reg_r,reg_q[31]}-{1'b0,reg_b});
wire [31:0] rem = r_sign ? reg_r + reg_b : reg_r;
wire [31:0] quo = reg_q;

always @(posedge aclk) begin

    count <= count+1;
    rdy <= 0;

    unique case (cmdmode)
        INIT: begin
            reg_r <= 32'b0;
            cmdmode <= WCMD;
        end
        WCMD: begin
            r_sign <= 0;
            reg_q <= dividend[31] ? (~dividend+1) : dividend;	// ABS(dividend)
            reg_b <= divisor[31] ? (~divisor+1) : divisor;		// ABS(divisor)
            negd <= dividend[31];
            negq <= (divisor[31]^dividend[31]);
            count <= 0;
            cmdmode <= start ? DIVLOOP : WCMD;
        end
        DIVLOOP: begin
        	unique case (r_sign)
        		1'b1: begin
            		reg_r <= sub_addn[31:0];
    		        r_sign <= sub_addn[32];
	            	reg_q <= {reg_q[30:0], ~sub_addn[32]};
        		end
        		1'b0: begin
            		reg_r <= sub_addm[31:0];
    		        r_sign <= sub_addm[32];
	            	reg_q <= {reg_q[30:0], ~sub_addm[32]};
        		end
        	endcase
            rdy <= count[5];
            cmdmode <= count[5] ? WCMD : DIVLOOP;
            if (count[5]) begin
                remainder <= negd ? (~rem+1) : rem;
                quotient <= negq ? (~quo+1) : quo;
                reg_r <= 32'b0;
            end else begin
                remainder <= 32'd0;
                quotient <= 32'd0;
            end
        end
    endcase

    if (~aresetn) begin
        cmdmode <= INIT;
    end
end
endmodule
