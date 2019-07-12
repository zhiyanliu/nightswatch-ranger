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
AGENT_NAME = dmpagent
AGENT_ENTRY_NAME = agent
NIGHTWATCH_NAME = nightwatch

AGENT_DIR = .
NIGHTWATCH_DIR = .

#IoT SDK directory
IOT_SDK_DIR = $(AGENT_DIR)/aws-iot-device-sdk-embedded-C

PLATFORM_TLS_DIR = $(IOT_SDK_DIR)/platform/linux/mbedtls
PLATFORM_COMMON_DIR = $(IOT_SDK_DIR)/platform/linux/common
PLATFORM_PTHREAD_DIR = $(IOT_SDK_DIR)/platform/linux/pthread

IOT_INCLUDE_DIRS += -I .
IOT_INCLUDE_DIRS += -I $(IOT_SDK_DIR)/include
IOT_INCLUDE_DIRS += -I $(IOT_SDK_DIR)/sdk_config
IOT_INCLUDE_DIRS += -I $(IOT_SDK_DIR)/external_libs/jsmn
IOT_INCLUDE_DIRS += -I $(PLATFORM_TLS_DIR)
IOT_INCLUDE_DIRS += -I $(PLATFORM_COMMON_DIR)
IOT_INCLUDE_DIRS += -I $(PLATFORM_PTHREAD_DIR)

IOT_SRC_FILES += $(shell find $(IOT_SDK_DIR)/src/ -name '*.c')
IOT_SRC_FILES += $(shell find $(IOT_SDK_DIR)/external_libs/jsmn -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_TLS_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_COMMON_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_PTHREAD_DIR)/ -name '*.c')

#TLS - mbedtls
MBEDTLS_DIR = $(IOT_SDK_DIR)/external_libs/mbedTLS
TLS_LIB_DIR = $(MBEDTLS_DIR)/library
TLS_INCLUDE_DIR = -I $(MBEDTLS_DIR)/include

#MD5 - openssl, for mac/brew
OPENSSL_LIB_DIR = /usr/local/opt/openssl/lib
OPENSSL_INCLUDE_DIR = -I /usr/local/opt/openssl/include

#AGENT - dmpagent
AGENT_INCLUDE_DIRS += -I $(AGENT_DIR)
AGENT_INCLUDE_DIRS += -I $(AGENT_DIR)/include

#NIGHTWATCH - nightwatch
NIGHTWATCH_INCLUDE_DIRS += -I $(NIGHTWATCH_DIR)
NIGHTWATCH_INCLUDE_DIRS += -I $(NIGHTWATCH_DIR)/include

#Aggregate all include and src directories for iot sdk
IOT_INCLUDE_ALL_DIRS += $(IOT_INCLUDE_DIRS)
IOT_INCLUDE_ALL_DIRS += $(TLS_INCLUDE_DIR)
IOT_INCLUDE_ALL_DIRS += $(OPENSSL_INCLUDE_DIR)

#Aggregate all include and src directories for agent - dmpagent
AGENT_INCLUDE_ALL_DIRS += $(AGENT_INCLUDE_DIRS)
AGENT_INCLUDE_ALL_DIRS += $(IOT_INCLUDE_ALL_DIRS)

#Aggregate all include and src directories for nightwatch - nightwatch
NIGHTWATCH_INCLUDE_ALL_DIRS += $(NIGHTWATCH_INCLUDE_DIRS)
NIGHTWATCH_INCLUDE_ALL_DIRS += $(IOT_INCLUDE_ALL_DIRS)

# Logging level control
LOG_FLAGS += -DENABLE_IOT_DEBUG
LOG_FLAGS += -DENABLE_IOT_INFO
LOG_FLAGS += -DENABLE_IOT_WARN
LOG_FLAGS += -DENABLE_IOT_ERROR

COMPILER_FLAGS += $(LOG_FLAGS) -D_ENABLE_THREAD_SUPPORT_
#If the processor is big endian uncomment the compiler flag
#COMPILER_FLAGS += -DREVERSED


MBED_TLS_MAKE_CMD = $(MAKE) -C $(MBEDTLS_DIR)

PRE_MAKE_SDK_CMD = $(MBED_TLS_MAKE_CMD)
MAKE_SDK_CMD = $(CC) $(IOT_SRC_FILES) $(COMPILER_FLAGS) -c $(IOT_INCLUDE_ALL_DIRS)
POST_MAKE_SDK_CMD = $(AR) cr lib$(SDK_NAME).a *.o

AGENT_SRC_FILES += $(shell find $(APP_DIR) -name "*.c" -not -path "./aws-iot-device-sdk-embedded-C/*" -not -path "./nightwatch_main.c")
NIGHTWATCH_SRC_FILES += $(shell find $(APP_DIR) -name "*.c" -not -path "./aws-iot-device-sdk-embedded-C/*" -not -path "./agent_main.c")

AWSIOT_SDK_LIB += -L .

AGENT_EXTERNAL_LIBS += -L $(TLS_LIB_DIR) -L $(OPENSSL_LIB_DIR)
AGENT_LD_FLAG += -Wl,-rpath,$(TLS_LIB_DIR)
AGENT_LD_FLAG += -ldl -lpthread -l$(SDK_NAME) -lmbedtls -lmbedx509 -lmbedcrypto -lcurl -lcrypto

NIGHTWATCH_EXTERNAL_LIBS += -L $(TLS_LIB_DIR) -L $(OPENSSL_LIB_DIR)
NIGHTWATCH_LD_FLAG += -Wl,-rpath,$(TLS_LIB_DIR)
NIGHTWATCH_LD_FLAG += -ldl -lpthread -l$(SDK_NAME) -lmbedtls -lmbedx509 -lmbedcrypto -lcurl -lcrypto

MAKE_AGENT_CMD = $(CC) $(AGENT_SRC_FILES) $(COMPILER_FLAGS) -o $(AGENT_NAME) $(AGENT_LD_FLAG) $(AGENT_EXTERNAL_LIBS) $(AWSIOT_SDK_LIB) $(AGENT_INCLUDE_ALL_DIRS)

MAKE_NIGHTWATCH_CMD = $(CC) $(NIGHTWATCH_SRC_FILES) $(COMPILER_FLAGS) -o $(NIGHTWATCH_NAME) $(NIGHTWATCH_LD_FLAG) $(NIGHTWATCH_EXTERNAL_LIBS) $(AWSIOT_SDK_LIB) $(NIGHTWATCH_INCLUDE_ALL_DIRS)

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

$(AGENT_NAME): lib$(SDK_NAME).a $(AGENT_SRC_FILES)
	$(PRE_MAKE_AGENT_CMD)
	$(DEBUG)$(MAKE_AGENT_CMD)
	$(POST_MAKE_AGENT_CMD)

$(NIGHTWATCH_NAME): lib$(SDK_NAME).a $(NIGHTWATCH_SRC_FILES)
	$(PRE_MAKE_NIGHTWATCH_CMD)
	$(DEBUG)$(MAKE_NIGHTWATCH_CMD)
	$(POST_MAKE_NIGHTWATCH_CMD)

$(AGENT_ENTRY_NAME): $(AGENT_NAME) $(NIGHTWATCH_NAME)
	mkdir -p bin/p1
	mkdir -p bin/p2
	cp -f $(NIGHTWATCH_NAME) bin
	cp -f $(AGENT_NAME) bin/p1/$(AGENT_NAME)
	ln -fs bin/p1/$(AGENT_NAME) $(AGENT_ENTRY_NAME)

install: $(AGENT_ENTRY_NAME)

all: install

clean:
	rm -rf bin
	rm -f *.o
	rm -f lib$(SDK_NAME).a
	rm -f $(AGENT_NAME)
	rm -f $(NIGHTWATCH_NAME)
	rm -f $(AGENT_ENTRY_NAME)
	-$(MBED_TLS_MAKE_CMD) clean
