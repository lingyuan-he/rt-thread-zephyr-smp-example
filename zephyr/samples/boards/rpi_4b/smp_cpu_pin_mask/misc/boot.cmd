fatload mmc 0:1 ${fdt_addr_r} bcm2711-rpi-4-b.dtb
fatload mmc 0:1 ${kernel_addr_r} rtthread.bin
setenv bootargs "earlycon=uart8250,mmio32,0xfe215040 console=ttyAMA0,115200 cma=8M coherent_pool=2M"
booti ${kernel_addr_r} - ${fdt_addr_r}
