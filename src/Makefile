#*************************************************************************
#Title:    
#Authors:  Michael Petersen
#          
#File:     
#
#*************************************************************************

BASE_NAME = ckt-defect

ARD_ESP32_VER = 3.3.12

GITREV := $(shell git rev-parse --short=6 HEAD)
REV    := $(shell ./git-revision.sh)

#BOARD     = esp32:esp32:esp32s2:CDCOnBoot=cdc,UploadMode=cdc,PSRAM=enabled,PartitionScheme=huge_app
BOARD     = esp32:esp32:esp32s2:CDCOnBoot=cdc,UploadMode=cdc,PSRAM=enabled
OPTIONS   = --build-property upload.maximum_size=4128768 --build-property compiler.cpp.extra_flags='-DGIT_REV="$(GITREV)" -DVERSION_STRING="$(REV)"' --build-property compiler.optimization_flags=-O2 --libraries ckt-defect/menu/
PORT      = /dev/ttyACM0

ARDUINO   = arduino-cli
BUILD_DIR = ./build

COMPILE   = $(ARDUINO) compile -b $(BOARD) --build-path $(BUILD_DIR) $(OPTIONS) $(BASE_NAME) --warnings all

define ESP_TOUCH_RESET
	@echo "Putting ESP32-S2 into bootloader mode..."
	@python3 -c "import serial; s = serial.Serial('$(PORT)', 1200); s.close()"
	@echo "Waiting for port $(PORT) to reappear..."
	@bash -c 'for i in {1..10}; do [ -e $(PORT) ] && exit 0; sleep 0.3; done; echo "Port $(PORT) did not reappear!"; exit 1'
endef

help:
	@echo "make compile.... Build $(BASE_NAME)"
	@echo "make upload..... Flash the firmware"
	@echo "make terminal... Open uart terminal"

# https://arduino.github.io/arduino-cli/0.32/commands/arduino-cli_compile/

compile:
	$(COMPILE)

compile-verbose:
	$(COMPILE) -v

#upload:
#	$(ARDUINO) upload -v --input-dir $(BUILD_DIR)  -b $(BOARD) -p $(PORT) $(BASE_NAME)

# Routine upload: Only flashes the sketch binary (preserves preferences/NVS)
upload:
	$(ESP_TOUCH_RESET)
	esptool.py --chip esp32s2 --port $(PORT) --baud 921600 --before default_reset --after hard_reset write_flash \
		0x10000 $(BUILD_DIR)/ckt-defect.ino.bin

# Fresh board upload: Flashes bootloader, partition table, boot app, and sketch
upload-fresh:
	$(ESP_TOUCH_RESET)
	esptool.py --chip esp32s2 --port $(PORT) --baud 921600 --before default_reset --after hard_reset write_flash \
		0x1000 $(BUILD_DIR)/ckt-defect.ino.bootloader.bin \
		0x8000 $(BUILD_DIR)/ckt-defect.ino.partitions.bin \
		0xe000 /home/railfan/.arduino15/packages/esp32/hardware/esp32/$(ARD_ESP32_VER)/tools/partitions/boot_app0.bin \
		0x10000 $(BUILD_DIR)/ckt-defect.ino.bin

# For production.  Needs boot_app0.bin in $(BIN) directory.  Eliminates dependency on installed Arduino version
firmware:
	$(ESP_TOUCH_RESET)
	esptool.py --chip esp32s2 --port $(PORT) --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 $(BIN)/ckt-defect.ino.bootloader.bin 0x8000 $(BIN)/ckt-defect.ino.partitions.bin 0xe000 $(BIN)/boot_app0.bin 0x10000 $(BIN)/ckt-defect.ino.bin

terminal:
	miniterm.py $(PORT) 115200

clean:
	rm -r $(BUILD_DIR)
