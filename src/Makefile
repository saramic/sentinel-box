PROJECT = sentinel_box
TARGET  = MAX3263X
BOARD   = EvKit_V1

MAXIM_PATH = ${HOME}/Maxim

COMPILER = GCC

TOOL_DIR = ${MAXIM_PATH}/Toolchain/bin

# Capture this Makefile's directory before any includes expand MAKEFILE_LIST
SRC_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# -------------------------------------------------------------------------
# BTstack source tree — shared submodule at repo root third_party/btstack
# -------------------------------------------------------------------------
BTSTACK_ROOT := $(SRC_DIR)/../third_party/btstack
BTSTACK_SRC  := $(BTSTACK_ROOT)/src

# cc256x_init_script.c is generated from TI .bts files (proprietary, not committed).
# Run: make generate
ifeq ($(wildcard cc256x_init_script.c),)
  ifneq ($(MAKECMDGOALS),generate)
    $(error cc256x_init_script.c not found -- run: make generate)
  endif
endif

BTSTACK_SRCS := \
    btstack_memory.c \
    btstack_memory_pool.c \
    btstack_linked_list.c \
    btstack_run_loop.c \
    btstack_run_loop_base.c \
    btstack_util.c \
    btstack_crypto.c \
    btstack_slip.c \
    hci.c \
    hci_cmd.c \
    hci_dump.c \
    hci_dump_dispatch.c \
    hci_event.c \
    hci_transport_h4.c \
    ad_parser.c \
    l2cap.c \
    l2cap_signaling.c \
    att_db.c \
    att_dispatch.c \
    att_server.c \
    le_device_db_memory.c \
    sm.c \
    btstack_run_loop_embedded.c \
    btstack_tlv.c \
    btstack_chipset_cc256x.c \
    btstack_uart_block_embedded.c

PORT_SRCS := \
    btstack_port.c

SRCS  = main.c \
        cc256x_init_script.c \
        $(BTSTACK_SRCS) \
        $(PORT_SRCS)

VPATH = .
VPATH += $(BTSTACK_ROOT)/src
VPATH += $(BTSTACK_ROOT)/src/ble
VPATH += $(BTSTACK_ROOT)/platform/embedded
VPATH += $(BTSTACK_ROOT)/chipset/cc256x
VPATH += port

IPATH = .

IPATH += $(BTSTACK_ROOT)/src
IPATH += $(BTSTACK_ROOT)/src/ble
IPATH += $(BTSTACK_ROOT)/src/classic
IPATH += $(BTSTACK_ROOT)/platform/embedded
IPATH += $(BTSTACK_ROOT)/3rd-party/micro-ecc
IPATH += $(BTSTACK_ROOT)/chipset/cc256x
IPATH += port

PROJ_CFLAGS += -DMXC_ASSERT_ENABLE
PROJ_CFLAGS += -Wno-unused-function

TARGET_UC := $(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LC := $(shell echo $(TARGET) | tr A-Z a-z)

LIBS_DIR    = $(MAXIM_PATH)/Firmware/$(TARGET_UC)/Libraries
CMSIS_ROOT  = $(LIBS_DIR)/CMSIS

BOARD_DIR = $(LIBS_DIR)/Boards/$(BOARD)
include $(BOARD_DIR)/board.mk

PERIPH_DRIVER_DIR = $(LIBS_DIR)/$(TARGET_UC)PeriphDriver
include $(PERIPH_DRIVER_DIR)/periphdriver.mk

include $(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/$(COMPILER)/$(TARGET_LC).mk

# -------------------------------------------------------------------------
# Generate cc256x init script from TI .bts files
# -------------------------------------------------------------------------
BTS_DIR := $(SRC_DIR)/../reference/CC256XB_BT_SP/v1.8/initscripts

.PHONY: generate
generate:
	python3 $(BTSTACK_ROOT)/chipset/cc256x/convert_bts_init_scripts.py \
	    $(BTS_DIR)-TIInit_6.7.16_bt_spec_4.1.bts \
	    $(BTS_DIR)-TIInit_6.7.16_ble_add-on.bts \
	    cc256x_init_script.c

# -------------------------------------------------------------------------
# Generate ATT database header from GATT service definition
# -------------------------------------------------------------------------
led_service.h: led_service.gatt $(BTSTACK_ROOT)/tool/compile_gatt.py
	python3 $(BTSTACK_ROOT)/tool/compile_gatt.py led_service.gatt led_service.h

$(BUILD_DIR)/main.o: led_service.h
