# Attempt to load a config.make file.
# If none is found, project defaults in config.project.make will be used.
ifneq ($(wildcard config.make),)
	include config.make
endif

# make sure the the OF_ROOT location is defined
ifndef OF_ROOT
	OF_ROOT=$(realpath ../../../..)
endif

# call the project makefile!
include $(OF_ROOT)/libs/openFrameworksCompiled/project/makefileCommon/compile.project.mk

# Show available targets
help:
	@echo "Available targets:"
	@echo "  make                 - Build debug version"
	@echo "  make Release         - Build release version"
	@echo "  make clean           - Clean build artifacts"
	@echo "  make help            - Show this help"
	@echo ""
	@echo "Resources:"
	@echo "  make copy-resources  - Copy AVS resources to bin/data"
	@echo ""
	@echo "Packaging:"
	@echo "  make package         - Build release + package + create DMG"
	@echo "  make package-only    - Package existing build (no rebuild)"
	@echo "  make run-release     - Open the packaged app"
	@echo "  make clean-release   - Remove release artifacts"
	@echo "  Output: bin/release/chain.app, bin/release/chain.dmg"
	@echo ""
	@echo "Info:"
	@echo "  make app-info        - Show architectures and min OS version"

# Copy AVS resources to data folder
AVS_RESOURCES = ../../libs/avs_lib/resources
DATA_AVS = bin/data/avs

copy-resources:
	@mkdir -p $(DATA_AVS)
	@cp -r $(AVS_RESOURCES)/* $(DATA_AVS)/ 2>/dev/null || true
	@echo "Copied AVS resources to $(DATA_AVS)"

# App packaging targets
SCRIPTS_DIR = ../../scripts
APP_NAME = $(shell basename $(CURDIR))

# Build release and package as distributable .app
package: Release copy-resources
	@$(SCRIPTS_DIR)/package-app.sh $(APP_NAME) bin

# Package without rebuilding (use existing build)
package-only:
	@$(SCRIPTS_DIR)/package-app.sh $(APP_NAME) bin

# Open the packaged app
run-release:
	@open bin/release/$(APP_NAME).app

# Clean release artifacts
clean-release:
	@rm -rf bin/release
	@echo "Cleaned release directory"

# Show app binary info (architectures and minimum OS version)
app-info:
	@BINARY="bin/$(APP_NAME).app/Contents/MacOS/$(APP_NAME)"; \
	if [ ! -f "$$BINARY" ]; then \
		echo "App not built yet. Run 'make Release' first."; \
		exit 1; \
	fi; \
	echo "=== Binary Info: $(APP_NAME) ==="; \
	echo ""; \
	echo "Architectures:"; \
	lipo -archs "$$BINARY" | tr ' ' '\n' | sed 's/^/  /'; \
	echo ""; \
	echo "File type:"; \
	file "$$BINARY" | sed 's/.*: /  /'; \
	echo ""; \
	echo "Minimum macOS version:"; \
	otool -l "$$BINARY" 2>/dev/null | grep -A 4 "LC_BUILD_VERSION" | grep -E "minos|platform" | sed 's/^/  /' || \
	otool -l "$$BINARY" 2>/dev/null | grep -A 2 "LC_VERSION_MIN_MACOSX" | grep "version" | sed 's/^/  /' || \
	echo "  (not found)"

.PHONY: help copy-resources package package-only run-release clean-release app-info
