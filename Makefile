#
# Created by Zhi Yan Liu on 2019-05-17.
#

#This target is to ensure accidental execution of Makefile as a bash script will not execute commands like rm in unexpected directories and exit gracefully.
.prevent_execution:
	exit 0

CC ?= gcc
AR ?= ar

#remove @ for no make command prints
DEBUG = @

SDK_NAME = awsiot
APP_NAME = dmpagent

APP_DIR = .

#IoT SDK directory
IOT_SDK_DIR = $(APP_DIR)/aws-iot-device-sdk-embedded-C

PLATFORM_DIR = $(IOT_SDK_DIR)/platform/linux/mbedtls
PLATFORM_COMMON_DIR = $(IOT_SDK_DIR)/platform/linux/common

IOT_INCLUDE_DIRS += -I $(IOT_SDK_DIR)/include
IOT_INCLUDE_DIRS += -I $(IOT_SDK_DIR)/sdk_config
IOT_INCLUDE_DIRS += -I $(IOT_SDK_DIR)/external_libs/jsmn
IOT_INCLUDE_DIRS += -I $(PLATFORM_COMMON_DIR)
IOT_INCLUDE_DIRS += -I $(PLATFORM_DIR)

IOT_SRC_FILES += $(shell find $(IOT_SDK_DIR)/src/ -name '*.c')
IOT_SRC_FILES += $(shell find $(IOT_SDK_DIR)/external_libs/jsmn -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_COMMON_DIR)/ -name '*.c')

#TLS - mbedtls
MBEDTLS_DIR = $(IOT_SDK_DIR)/external_libs/mbedTLS
TLS_LIB_DIR = $(MBEDTLS_DIR)/library
TLS_INCLUDE_DIR = -I $(MBEDTLS_DIR)/include

#MD5 - openssl, for mac/brew
OPENSSL_LIB_DIR = /usr/local/opt/openssl/lib
OPENSSL_INCLUDE_DIR = -I /usr/local/opt/openssl/include

#APP - dmpagent
APP_INCLUDE_DIRS += -I $(APP_DIR)
APP_INCLUDE_DIRS += -I $(APP_DIR)/include

#Aggregate all include and src directories
INCLUDE_ALL_DIRS += $(APP_INCLUDE_DIRS)
INCLUDE_ALL_DIRS += $(IOT_INCLUDE_DIRS)
INCLUDE_ALL_DIRS += $(TLS_INCLUDE_DIR)
INCLUDE_ALL_DIRS += $(OPENSSL_INCLUDE_DIR)

# Logging level control
LOG_FLAGS += -DENABLE_IOT_DEBUG
LOG_FLAGS += -DENABLE_IOT_INFO
LOG_FLAGS += -DENABLE_IOT_WARN
LOG_FLAGS += -DENABLE_IOT_ERROR

COMPILER_FLAGS += $(LOG_FLAGS)
#If the processor is big endian uncomment the compiler flag
#COMPILER_FLAGS += -DREVERSED

MBED_TLS_MAKE_CMD = $(MAKE) -C $(MBEDTLS_DIR)

PRE_MAKE_SDK_CMD = $(MBED_TLS_MAKE_CMD)
MAKE_SDK_CMD = $(CC) $(IOT_SRC_FILES) $(COMPILER_FLAGS) -c $(INCLUDE_ALL_DIRS)
POST_MAKE_SDK_CMD = $(AR) cr lib$(SDK_NAME).a *.o

APP_SRC_FILES += $(shell find $(APP_DIR) -name "*.c" -not -path "./aws-iot-device-sdk-embedded-C/*")

EXTERNAL_LIBS += -L $(TLS_LIB_DIR) -L $(OPENSSL_LIB_DIR)
AWSIOT_SDK += -L $(APP_DIR)
LD_FLAG += -Wl,-rpath,$(TLS_LIB_DIR)
LD_FLAG += -ldl -lpthread -l$(SDK_NAME) -lmbedtls -lmbedx509 -lmbedcrypto -lcurl -lcrypto

MAKE_APP_CMD = $(CC) $(APP_SRC_FILES) $(COMPILER_FLAGS) -o $(APP_NAME) $(LD_FLAG) $(EXTERNAL_LIBS) $(AWSIOT_SDK) $(INCLUDE_ALL_DIRS)

default: all

aws-iot-device-sdk-embedded-C:
	@git clone -b v3.0.1 https://github.com/aws/aws-iot-device-sdk-embedded-C
	rm -rf $(APP_DIR)/aws-iot-device-sdk-embedded-C/external_libs/CppUTest
	@git clone -b v3.8 https://github.com/cpputest/cpputest $(APP_DIR)/aws-iot-device-sdk-embedded-C/external_libs/CppUTest
	rm -rf $(APP_DIR)/aws-iot-device-sdk-embedded-C/external_libs/mbedTLS
	@git clone -b mbedtls-2.16.1 https://github.com/ARMmbed/mbedtls $(APP_DIR)/aws-iot-device-sdk-embedded-C/external_libs/mbedTLS

vendor_clean:
	rm -rf $(APP_DIR)/aws-iot-device-sdk-embedded-C

lib$(SDK_NAME).a: aws-iot-device-sdk-embedded-C
	$(PRE_MAKE_SDK_CMD)
	$(DEBUG)$(MAKE_SDK_CMD)
	$(POST_MAKE_SDK_CMD)

$(APP_NAME): lib$(SDK_NAME).a $(APP_SRC_FILES)
	$(PRE_MAKE_APP_CMD)
	$(DEBUG)$(MAKE_APP_CMD)
	$(POST_MAKE_APP_CMD)

all: $(APP_NAME)

clean:
	rm -f *.o
	rm -f lib$(SDK_NAME).a
	rm -f $(APP_NAME)
	-$(MBED_TLS_MAKE_CMD) clean
