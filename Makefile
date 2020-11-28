# Project Name
TARGET = field_dist_test

# Sources
CPP_SOURCES = main.cpp

# Library Locations
LIBDAISY_DIR = /mnt/c/Users/usman/Documents/GitHub/Marguerite/DaisyExamples/libDaisy
DAISYSP_DIR = /mnt/c/Users/usman/Documents/GitHub/Marguerite/DaisyExamples/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

