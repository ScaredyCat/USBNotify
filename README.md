
Simple LED that can be plugged in and triggered passing values. 

I have a homelab that consists of similar looking HP Elitedesk machines and wanted a simple way to identify which machine was which. Super cheap really basic method to identiy a machine from the shell. Also useful if you need to give instructions to another person remotely - you can identify which machine they need to work on.


You might need to remove  os version of  arm gcc. 

sudo apt remove gcc-arm-none-eabi binutils-arm-none-eabi

arm version

cd ~
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz
tar xf arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz


Build: 

cd ~/pico-dev/usbnotify
rm -rf build
mkdir build && cd build
cmake .. -DPICO_BOARD=waveshare_rp2040_one
make -j$(nproc)


Insert the device while holding the boot button, it will present as a usb drive. Copy the uf2 file to it and it will automatically disconnect and restart as a usb hid device. 


Mac users need hidapitester

Example script for sontrol in support also linux udev rule.


