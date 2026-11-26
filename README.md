# pl

# INSTALLATION

$ make
$ make img

For rpib3+

$ dtc -I dts -O dtb -o build/rpi3.dtb tools/dts/rpi3.dts
$ qemu-system-aarch64 -M raspi3b -kernel build/64/kernel8.img -dtb build/rpi3.dtb
