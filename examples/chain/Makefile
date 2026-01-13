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

# Copy AVS resources to data folder
AVS_RESOURCES = ../../libs/avs_lib/resources
DATA_AVS = bin/data/avs

copy-resources:
	@mkdir -p $(DATA_AVS)
	@cp -r $(AVS_RESOURCES)/* $(DATA_AVS)/ 2>/dev/null || true
	@echo "Copied AVS resources to $(DATA_AVS)"
