# addon_config.mk for ofxAVS

meta:
	ADDON_NAME = ofxAVS
	ADDON_DESCRIPTION = Advanced Visualization Studio port for OpenFrameworks - complete self-contained AVS library
	ADDON_AUTHOR = AVS Port Project
	ADDON_TAGS = "audio" "visualization" "effects" "music" "avs"
	ADDON_URL = 

common:
	# Include all AVS library source files
	ADDON_SOURCES_INCLUDE = src/avs_lib/core src/avs_lib/effects

	# No external library dependencies - fully self-contained!
	# ADDON_LIBS = 

	# Compiler settings
	ADDON_CPPFLAGS = -std=c++17
	
	# Include directories for the embedded AVS library
	ADDON_INCLUDES = src/avs_lib src/avs_lib/core