#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := moonlight


MODULE_PATH := $(abspath $(shell pwd))

EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/speech_recognition/lib
EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/speech_recognition/wake_word_engine
EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/speech_recognition/speech_command_recognition
EXTRA_COMPONENT_DIRS += $(MODULE_PATH)/components/speech_recognition/acoustic_algorithm

include $(IDF_PATH)/make/project.mk

