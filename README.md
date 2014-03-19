induino-usb
===========

The induino board is a version of Arduino Uno created by Simple Labs. 

I am trying to develop a linux USB driver which would allow us to control the various peripherals of Induino from a
userspace program in linux. This is a self learning project for me solving two purposes: 
Learning how to write device drivers in linux and learning how to use the Arduino/Induino Boards.

Typically the device drivers are written to conform to the protocols of the device. But these boards contain 
an ATMega processor which has to be programmed by us, and so we are responsible for designing the protocol too. 
Hence the project will be in two portions:

1. Creating an arduino library which implements the device protocol.

2. Creating a linux usb device driver as per this protocol.
