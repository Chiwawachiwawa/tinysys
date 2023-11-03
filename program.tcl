open_hw_manager
connect_hw_server -allow_non_jtag

open_hw_target
set_property PROGRAM.FILE {./tinysys.runs/impl_1/tophat.bit} [get_hw_devices xc7a200t_0]
current_hw_device [get_hw_devices xc7a200t_0]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices xc7a200t_0] 0]
create_hw_cfgmem -hw_device [get_hw_devices xc7a200t_0] -mem_dev [lindex [get_cfgmem_parts {is25lp128f-spi-x1_x2_x4}] 0]

set_property PROBES.FILE {} [get_hw_devices xc7a200t_0]
set_property FULL_PROBES.FILE {} [get_hw_devices xc7a200t_0]
set_property PROGRAM.FILE {./tinysys.runs/impl_1/tophat.bit} [get_hw_devices xc7a200t_0]
program_hw_devices [get_hw_devices xc7a200t_0]
