set_property IOSTANDARD LVCMOS33 [get_ports {led_1[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led_1[0]}]
set_property PACKAGE_PIN E3 [get_ports {led_1[1]}]
set_property PACKAGE_PIN F3 [get_ports {led_1[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports sys_clk]
set_property IOSTANDARD LVCMOS33 [get_ports sys_rst_n]
set_property PACKAGE_PIN W19 [get_ports sys_clk]
set_property PACKAGE_PIN Y6 [get_ports sys_rst_n]

set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]