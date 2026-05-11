PROJECT = in_stepper_motors_lpsdk
TARGET  = MAX3263X
BOARD   = EvKit_V1

MAXIM_PATH = ${HOME}/Maxim

COMPILER = GCC

TOOL_DIR = ${MAXIM_PATH}/Toolchain/bin

SRCS  = main.c
VPATH = .
IPATH = .

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
