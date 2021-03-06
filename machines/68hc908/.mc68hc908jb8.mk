#
# include for Makefile
#
CPU_VARIANT = hc908jb8

# true size
NATIVE_RAM_START = 0x40
NATIVE_RAM_SIZE = 256

# with bootloader
RAM_START = 0x48
RAM_SIZE = 248

STACK_LOC = 0x013F
# NATIVE_RAM_SIZE + NATIVE_RAM_START -1

FLASH_START = 0xDC00
NATIVE_FLASH_SIZE = 8192
FLASH_SIZE = 7808

BOOT_LOADER_SIZE = 384

MONI_START = 0xFC00
# MONI_2_START = 0xFE10
MONI_SIZE = 512
# MONI_2_SIZE = 240

MHZ = 3

MEM_MODEL = --model-small --stack-auto --idata-loc 0x48 

