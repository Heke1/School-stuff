# objective:

Create a Linux kernel module that can hold a secret message. Encrypting and decrypting the message needs to be done via ioctl-systemcall.

# structure:

ioctl macros are defined in separate header. Program for testing the module is provided, 
it assumes that the modules name is 'salausmoduuli' and tries to access it directly from /dev.


# usage:

Build with 'make'.

load the module with insmod, and start the test program, you need to be a root.


