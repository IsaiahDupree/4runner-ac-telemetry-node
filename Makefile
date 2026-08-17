ESP_IDF_IMAGE ?= espressif/idf:v5.5.3
FIRMWARE_DIR := firmware/ac-sensor-node-esp32s3
VERSION ?= v0.1.0-dev.1

.PHONY: verify verify-build build release

verify:
	python3 tools/verify_repository.py

verify-build:
	python3 tools/verify_repository.py --build-dir $(FIRMWARE_DIR)/build

build:
	docker run --rm -v "$(CURDIR):/project" -w /project/$(FIRMWARE_DIR) $(ESP_IDF_IMAGE) bash -lc 'idf.py set-target esp32s3 && idf.py build'

release:
	docker run --rm -v "$(CURDIR):/project" -w /project $(ESP_IDF_IMAGE) bash -lc '$(FIRMWARE_DIR)/tools/build_release.sh $(VERSION)'
