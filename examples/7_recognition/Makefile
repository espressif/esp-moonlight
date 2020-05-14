#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := moonlight


MODULE_PATH := $(abspath $(shell pwd))

EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/lib
EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/wake_word_engine
EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/speech_command_recognition
EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/acoustic_algorithm

include $(IDF_PATH)/make/project.mk

