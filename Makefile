ESP_IDF_IMAGE ?= espressif/idf:v5.5.3
RECOVERY_FIRMWARE_DIR := firmware/ac-sensor-node-esp32s3
TEMPERATURE_FIRMWARE_DIR := firmware/ac-temperature-bench-esp32s3
VERSION ?= v0.1.0-dev.1

.PHONY: verify verify-build build build-recovery build-temperature release

verify:
	python3 tools/verify_repository.py

verify-build:
	python3 tools/verify_repository.py \
		--recovery-build-dir $(RECOVERY_FIRMWARE_DIR)/build \
		--temperature-build-dir $(TEMPERATURE_FIRMWARE_DIR)/build

build: build-recovery build-temperature

build-recovery:
	docker run --rm -v "$(CURDIR):/project" -w /project/$(RECOVERY_FIRMWARE_DIR) $(ESP_IDF_IMAGE) bash -lc 'idf.py set-target esp32s3 && idf.py build'

build-temperature:
	docker run --rm -v "$(CURDIR):/project" -w /project/$(TEMPERATURE_FIRMWARE_DIR) $(ESP_IDF_IMAGE) bash -lc 'idf.py set-target esp32s3 && idf.py build'

release:
	docker run --rm -v "$(CURDIR):/project" -w /project $(ESP_IDF_IMAGE) bash -lc '$(RECOVERY_FIRMWARE_DIR)/tools/build_release.sh $(VERSION)'
