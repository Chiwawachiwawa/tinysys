`timescale 1ns / 1ps

module axi4retimer(
	input wire aresetn,
	input wire srcclk,
	axi4if.slave srcbus,
	input wire destclk,
	axi4if.master destbus,
  	output wire destrst);

wire saresetn, maresetn;

assign destrst = maresetn;

axi4interconnect axi4interconnectinst(
  .INTERCONNECT_ACLK(srcclk),
  .INTERCONNECT_ARESETN(aresetn),

  .S00_AXI_ARESET_OUT_N(saresetn),
  .S00_AXI_ACLK(	srcclk),
  .S00_AXI_AWID(    1'b0/*srcbus.awid*/),
  .S00_AXI_AWADDR(  srcbus.awaddr),
  .S00_AXI_AWLEN(   srcbus.awlen),
  .S00_AXI_AWSIZE(  srcbus.awsize),
  .S00_AXI_AWBURST( srcbus.awburst),
  .S00_AXI_AWLOCK(  1'b0/*srcbus.awlock*/),
  .S00_AXI_AWCACHE( 4'b0011/*srcbus.awcache*/),
  .S00_AXI_AWPROT(  3'b000/*srcbus.awprot*/),
  .S00_AXI_AWQOS(   4'h0/*srcbus.awqos*/),
  .S00_AXI_AWVALID( srcbus.awvalid),
  .S00_AXI_AWREADY( srcbus.awready),
  .S00_AXI_WDATA(   srcbus.wdata),
  .S00_AXI_WSTRB(   srcbus.wstrb),
  .S00_AXI_WLAST(   srcbus.wlast),
  .S00_AXI_WVALID(  srcbus.wvalid),
  .S00_AXI_WREADY(  srcbus.wready),
  .S00_AXI_BID(     /*srcbus.bid*/),
  .S00_AXI_BRESP(   srcbus.bresp),
  .S00_AXI_BVALID(  srcbus.bvalid),
  .S00_AXI_BREADY(  srcbus.bready),
  .S00_AXI_ARID(    1'b0/*srcbus.arid*/),
  .S00_AXI_ARADDR(  srcbus.araddr),
  .S00_AXI_ARLEN(   srcbus.arlen),
  .S00_AXI_ARSIZE(  srcbus.arsize),
  .S00_AXI_ARBURST( srcbus.arburst),
  .S00_AXI_ARLOCK(  1'b0/*srcbus.arlock*/),
  .S00_AXI_ARCACHE( 4'b0011/*srcbus.arcache*/),
  .S00_AXI_ARPROT(  3'b000/*srcbus.arprot*/),
  .S00_AXI_ARQOS(   4'h0/*srcbus.arqos*/),
  .S00_AXI_ARVALID( srcbus.arvalid),
  .S00_AXI_ARREADY( srcbus.arready),
  .S00_AXI_RID(     /*srcbus.rid*/),
  .S00_AXI_RDATA(   srcbus.rdata),
  .S00_AXI_RRESP(   srcbus.rresp),
  .S00_AXI_RLAST(   srcbus.rlast),
  .S00_AXI_RVALID(  srcbus.rvalid),
  .S00_AXI_RREADY(  srcbus.rready),

  .M00_AXI_ARESET_OUT_N(maresetn),
  .M00_AXI_ACLK(    destclk),
  .M00_AXI_AWID(    /*destbus.awid*/),
  .M00_AXI_AWADDR(  destbus.awaddr),
  .M00_AXI_AWLEN(   destbus.awlen),
  .M00_AXI_AWSIZE(  destbus.awsize),
  .M00_AXI_AWBURST( destbus.awburst),
  .M00_AXI_AWLOCK(  /*destbus.awlock*/),
  .M00_AXI_AWCACHE( /*destbus.awcache*/),
  .M00_AXI_AWPROT(  /*destbus.awprot*/),
  .M00_AXI_AWQOS(   /*destbus.awqos*/),
  .M00_AXI_AWVALID( destbus.awvalid),
  .M00_AXI_AWREADY( destbus.awready),
  .M00_AXI_WDATA(   destbus.wdata),
  .M00_AXI_WSTRB(   destbus.wstrb),
  .M00_AXI_WLAST(   destbus.wlast),
  .M00_AXI_WVALID(  destbus.wvalid),
  .M00_AXI_WREADY(  destbus.wready),
  .M00_AXI_BID(     4'd0/*destbus.bid*/),
  .M00_AXI_BRESP(   destbus.bresp),
  .M00_AXI_BVALID(  destbus.bvalid),
  .M00_AXI_BREADY(  destbus.bready),
  .M00_AXI_ARID(    /*destbus.arid*/),
  .M00_AXI_ARADDR(  destbus.araddr),
  .M00_AXI_ARLEN(   destbus.arlen),
  .M00_AXI_ARSIZE(  destbus.arsize),
  .M00_AXI_ARBURST( destbus.arburst),
  .M00_AXI_ARLOCK(  /*destbus.arlock*/),
  .M00_AXI_ARCACHE( /*destbus.arcache*/),
  .M00_AXI_ARPROT(  /*destbus.arprot*/),
  .M00_AXI_ARQOS(   /*destbus.arqos*/),
  .M00_AXI_ARVALID( destbus.arvalid),
  .M00_AXI_ARREADY( destbus.arready),
  .M00_AXI_RID(     4'd0/*destbus.rid*/),
  .M00_AXI_RDATA(   destbus.rdata),
  .M00_AXI_RRESP(   destbus.rresp),
  .M00_AXI_RLAST(   destbus.rlast),
  .M00_AXI_RVALID(  destbus.rvalid),
  .M00_AXI_RREADY(  destbus.rready) );

endmodule
