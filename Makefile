#*******************************************************************************
#   Ledger App IOST
#   (c) 2020 Stanislav Shihalev <sshihalev@sfxdx.ru>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************


ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif


include $(BOLOS_SDK)/Makefile.defines

#########
#  App  #
#########
APP_LOAD_PARAMS= --curve ed25519 --path "44'/291'" --appFlags 0x240 $(COMMON_LOAD_PARAMS)

APPVERSION_M = 1
APPVERSION_N = 0
APPVERSION_P = 0
APPVERSION = $(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)
APPNAME = IOST


ifeq ($(TARGET_NAME),TARGET_BLUE)
ICONNAME = images/blue_app_$(APPNAME).gif
else
ifeq ($(TARGET_NAME),TARGET_NANOX)
ICONNAME = images/nanox_app_$(APPNAME).gif
else
ICONNAME = images/nanos_app_$(APPNAME).gif
endif
endif


################
# Default rule #
################
all: cleanpb proto default

############
# Platform #
############
DEFINES += $(DEFINES_LIB)

DEFINES   += OS_IO_SEPROXYHAL
DEFINES   += HAVE_BAGL HAVE_SPRINTF
DEFINES   += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES   += APPVERSION_M=$(APPVERSION_M) APPVERSION_N=$(APPVERSION_N) APPVERSION_P=$(APPVERSION_P)

# protobuf
DFEFINES  += PB_FIELD_32BIT=1

# printf
# ifneq ($(TARGET_NAME),TARGET_NANOX)
DEFINES   += PRINTF_DISABLE_SUPPORT_FLOAT PRINTF_DISABLE_SUPPORT_EXPONENTIAL PRINTF_DISABLE_SUPPORT_PTRDIFF_T
DEFINES   += PRINTF_NTOA_BUFFER_SIZE=9U PRINTF_FTOA_BUFFER_SIZE=0
# endif

# U2F
DEFINES   += HAVE_U2F HAVE_IO_U2F
DEFINES   += U2F_PROXY_MAGIC=\"$(APPNAME)abrcadabr\"
DEFINES   += USB_SEGMENT_SIZE=64
DEFINES   += BLE_SEGMENT_SIZE=32 #max MTU, min 20

WEBUSB_URL     = www.ledger.com
DEFINES       += HAVE_WEBUSB WEBUSB_URL_SIZE_B=$(shell echo -n $(WEBUSB_URL) | wc -c) WEBUSB_URL=$(shell echo -n $(WEBUSB_URL) | sed -e "s/./\\\'\0\\\',/g")

DEFINES   += UNUSED\(x\)=\(void\)x
DEFINES   += APPVERSION=\"$(APPVERSION)\"


ifeq ($(TARGET_NAME),TARGET_NANOX)
# Instead of vendor printf
DEFINES 	  += HAVE_SPRINTF

DEFINES       += IO_SEPROXYHAL_BUFFER_SIZE_B=300
DEFINES       += HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000
DEFINES       += HAVE_BLE_APDU # basic ledger apdu transport over BLE

DEFINES       += HAVE_GLO096
DEFINES       += BAGL_WIDTH=128 BAGL_HEIGHT=64
DEFINES       += HAVE_BAGL_ELLIPSIS # long label truncation feature
DEFINES       += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
DEFINES       += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
DEFINES       += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
DEFINES		  += HAVE_UX_FLOW
else
DEFINES   	  += IO_SEPROXYHAL_BUFFER_SIZE_B=128
endif

# Enabling debug PRINTF
ifeq ($(DEBUG),)
DEBUG = 0
endif

ifeq ($(DEBUG),0)
DEFINES += PRINTF\(...\)=
DEFINES += PLINE\(...\)=
else
ifeq ($(TARGET_NAME),TARGET_NANOX)
DEFINES   += HAVE_PRINTF PRINTF=mcu_usb_printf
else
DEFINES   += HAVE_PRINTF PRINTF=screen_printf
endif
DEFINES += PLINE="PRINTF(\"FILE:%s..LINE:%d\n\",__FILE__,__LINE__)"
endif

##############
#  Compiler  #
##############
ifneq ($(BOLOS_ENV),)
CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
endif

CC       := $(CLANGPATH)clang
AS       := $(GCCPATH)arm-none-eabi-gcc
LD       := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS  += -O3 -Os
LDLIBS   += -lm -lgcc -lc


# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### variables processed by the common makefile.rules of the SDK to grab source files and include dirs
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl lib_u2f

ifeq ($(TARGET_NAME),TARGET_NANOX)
SDK_SOURCE_PATH  += lib_blewbxx lib_blewbxx_impl
SDK_SOURCE_PATH  += lib_ux
endif

load: all
	python3 -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

load-offline: all
	python3 -m ledgerblue.loadApp $(APP_LOAD_PARAMS) --offline

delete:
	python3 -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile

listvariants:
	@echo VARIANTS COIN IOST

#check:
#	@ clang-tidy \
#		$(foreach path, $(APP_SOURCE_PATH), $(shell find $(path) -name "*.c" -and -not -name "pb*" -and -not -name "glyphs*")) -- \
#		$(CFLAGS) \
#		$(addprefix -D, $(DEFINES)) \
#		$(addprefix -I, $(INCLUDES_PATH))

sdk/ledger-nanopb/generator/proto/nanopb_pb2.py:
	@ make -C sdk/ledger-nanopb/generator/proto

# TODO: Figure out a way to do this without copying .c files
.PHONY: proto
proto: sdk/ledger-nanopb/generator/proto/nanopb_pb2.py
	@ cp -fr sdk/ledger-nanopb/pb_*.c sdk/ledger-nanopb/pb*.h src/
	@ echo 'syntax = "proto3";' | tee src/$(APPNAME)_api.proto
	@ echo 'import "nanopb.proto";' | tee -a src/$(APPNAME)_api.proto
	@ grep -A 10 'message Action' sdk/go-iost/rpc/pb/rpc.proto | grep -B 10 -e '^}' | tee -a src/$(APPNAME)_api.proto
	@ grep -A 10 'message AmountLimit' sdk/go-iost/rpc/pb/rpc.proto | grep -B 10 -e '^}' | tee -a src/$(APPNAME)_api.proto
	@ grep -A 50 'message TxReceipt' sdk/go-iost/rpc/pb/rpc.proto | grep -B 50 -e '^}' | tee -a src/$(APPNAME)_api.proto
	@ grep -A 50 'message Transaction' sdk/go-iost/rpc/pb/rpc.proto | grep -B 50 -e '^}' | tee -a src/$(APPNAME)_api.proto
	@ protoc \
		--plugin=protoc-gen-nanopb=sdk/ledger-nanopb/generator/protoc-gen-nanopb \
		--nanopb_out=src/ \
		-I=sdk/ledger-nanopb/generator/proto \
		-I=src \
		src/$(APPNAME)_api.proto

.PHONY: cleanpb
cleanpb:
	@ rm -fr src/pb_*.c src/pb*.h src/*.pb.h src/*.pb.c src/*_api.proto
