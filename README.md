
## Simple LED that can be plugged in and triggered passing colour values and commands.

I have a homelab that consists of similar looking HP Elitedesk machines and wanted a simple way to identify which machine was which. Super cheap really basic method to identify which machine I want to do something with from the shell. Also useful if you need to give instructions to another person remotely - you can identify which machine they need to work on.

Each device is controlled based on its serial number so you can have multiple devices plugged in and control each one specifically.

### After you've installed the prerequisites

```
sudo apt update
sudo apt install -y git cmake gcc-arm-none-eabi libnewlib-arm-none-eabi \
    build-essential libstdc++-arm-none-eabi-newlib
```


***You might need to remove you os version of arm gcc. I built this on Ubuntu 24.04 LTS and had to use the arm site toolchain.***

```
sudo apt remove gcc-arm-none-eabi binutils-arm-none-eabi
```

and install the arm version

```
cd ~
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz
tar xf arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz
```

### Clone the Pico SDK

```
cd ~
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

```

### Add the location to your profile

```
echo 'export PICO_SDK_PATH=$HOME/pico-sdk' >> ~/.bashrc
source ~/.bashrc
```

### Clone the examples too

```
cd ~
git clone https://github.com/raspberrypi/pico-examples.git
```

### Build the installable file

```
cd ~/pico-dev/USBNotify
rm -rf build
mkdir build && cd build
cmake .. -DPICO_BOARD=waveshare_rp2040_one
make -j$(nproc)
```
Once the code builds successfully insert the device while holding the boot button, it will present as a usb drive. Copy the uf2 file from the build directory to it and it will automatically disconnect and restart as a usb hid device. You'll need to be root or use sudo to control the device unless you do the following:

### Copy udev rule file and reload

```
cp support/99-hid-led.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger

```

Remove and plug in your device again, you should no longer need to sudo to control it.


### Mac users need hidapitester


Example script for control in support  directory also linux udev rule.

There are named colours in the example script ( red, green, blue, yellow, magenta, cyan,white, orange, purple,amber, indigo, off )

List all the devices on a machine:

```
./example.sh list

Scanning for Waveshare RP2040-One devices...
Node         | Serial Number       
/dev/hidraw4 | DE64A876CF551522    

```

Control the LED with

```
./example.sh <DEVICE-SERIAL-NUMBER> <COLOUR> <EFFECT> <BRIGHTNESS>
```
eg

```
./example.sh DE64A876CF551522 red pulse 10

```

Valid effects are solid, pulse, flash, blip, rainbow.

The colour off just sets the RGB values to 0,0,0
