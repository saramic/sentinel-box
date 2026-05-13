PROJECT = in_bluetooth_lpsdk
TARGET  = MAX3263X
BOARD   = EvKit_V1

MAXIM_PATH = ${HOME}/Maxim

COMPILER = GCC

TOOL_DIR = ${MAXIM_PATH}/Toolchain/bin

SRCS  = main.c
VPATH = .
IPATH = .

# TI CC256XB service pack — not committed (proprietary firmware, TI TSPA licence).
# Download CC256XB-BT-SP from https://www.ti.com/tool/CC256XB-BT-SP and place the
# extracted archive at ../../reference/CC256XB_BT_SP so this path resolves.
CC256XB_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../reference/CC256XB_BT_SP/v1.8/Bluetopia)
ifeq ($(wildcard $(CC256XB_DIR)/CC256XB.h),)
  $(error CC256XB.h not found at $(CC256XB_DIR)/CC256XB.h -- download TI CC256XB-BT-SP)
endif
IPATH += $(CC256XB_DIR)

PROJ_CFLAGS += -DMXC_ASSERT_ENABLE

TARGET_UC := $(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LC := $(shell echo $(TARGET) | tr A-Z a-z)

LIBS_DIR    = $(MAXIM_PATH)/Firmware/$(TARGET_UC)/Libraries
CMSIS_ROOT  = $(LIBS_DIR)/CMSIS

BOARD_DIR = $(LIBS_DIR)/Boards/$(BOARD)
include $(BOARD_DIR)/board.mk

PERIPH_DRIVER_DIR = $(LIBS_DIR)/$(TARGET_UC)PeriphDriver
include $(PERIPH_DRIVER_DIR)/periphdriver.mk

include $(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/$(COMPILER)/$(TARGET_LC).mk
