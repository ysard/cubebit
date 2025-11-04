#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

clean:
	pio run --target clean

d_menuconfig:
	pio run -e debug -t menuconfig

debug:
	pio run -e debug

flash:
	pio run -e debug -t upload

f:
	@echo "Flash firmware only"
	esptool --chip esp32c6 --port "/dev/ttyUSB0" --baud 230400 --before no_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x10000 /tmp/pio_build_cache/debug/firmware.bin

qemu: qemu_efuse.bin
	pio run -e qemu --target upload

all: debug


check:
	@echo "Perform static analysis..."
	pio check -e qemu -v

lint:
	@echo "Perform linter analysis..."
	-cpplint --linelength=95 --extensions=c src/*.c
