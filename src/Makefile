#*************************************************************************
#Title:    
#Authors:  Michael Petersen
#          
#File:     
#
#*************************************************************************

export GIT_DISCOVERY_ACROSS_FILESYSTEM := 1

BASE_NAME = ckt-defect

ARD_ESP32_VER = 3.3.12

GITREV := $(shell GIT_DISCOVERY_ACROSS_FILESYSTEM=1 git rev-parse --short=6 HEAD 2>/dev/null || echo "unknown")
REV    := $(shell GIT_DISCOVERY_ACROSS_FILESYSTEM=1 ./git-revision.sh 2>/dev/null || echo "v0.0.0")

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
	@bash -c 'sleep 0.2'
	@echo "Waiting for port $(PORT) to reappear..."
	@bash -c 'for i in {1..10}; do [ -e $(PORT) ] && exit 0; sleep 0.3; done; echo "Port $(PORT) did not reappear!"; exit 1'
	@bash -c 'sleep 0.5; echo "Done!"'
endef

help:
	@echo "make compile........ Build $(BASE_NAME) (runs generate-codec first)"
	@echo "make generate-codec. Run config generator & NVM coverage verifier"
	@echo "make export-schema.. Export version-tagged schema JSON manifest"
	@echo "make upload......... Flash the firmware"
	@echo "make terminal....... Open uart terminal"

# https://arduino.github.io/arduino-cli/0.32/commands/arduino-cli_compile/

generate-codec:
	@python3 tools/gen_codec.py

compile: generate-codec
	$(COMPILE)

compile-verbose: generate-codec
	$(COMPILE) -v

export-schema:
	@mkdir -p $(BUILD_DIR)
	@python3 -c 'import json, sys, subprocess; v = subprocess.check_output(["grep", "CONFIG_SCHEMA_VERSION", "ckt-defect/configuration.h"]).decode().split()[2]; print(json.dumps({"schema_version": int(v), "git_rev": "$(GITREV)", "version": "$(REV)"}, indent=2))' > $(BUILD_DIR)/config_schema_$(REV).json
	@echo "Exported $(BUILD_DIR)/config_schema_$(REV).json"

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
		0xe000 $(BUILD_DIR)/boot_app0.bin \
		0x10000 $(BUILD_DIR)/ckt-defect.ino.bin

firmware:
	$(ESP_TOUCH_RESET)
	esptool.py --chip esp32s2 --port $(PORT) --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 $(BIN)/ckt-defect.ino.bootloader.bin 0x8000 $(BIN)/ckt-defect.ino.partitions.bin 0xe000 $(BIN)/boot_app0.bin 0x10000 $(BIN)/ckt-defect.ino.bin

terminal:
	miniterm.py $(PORT) 115200

clean:
	rm -r $(BUILD_DIR)
