################################################################################
#
# SDK Sample Floating Point and NEON Trusted Application
#
################################################################################


# output binary name without path or extension
OUTPUT_NAME := sampleta


#-------------------------------------------------------------------------------
# MobiConvert parameters, see manual for details
#-------------------------------------------------------------------------------
TA_UUID := 66800000000000000000000000000000
TA_SERVICE_TYPE := SYS
TA_KEYFILE := Locals/Build/pairVendorTltSig.pem
TA_DEBUGGABLE := Y
HEAP_SIZE_INIT := 1048576
HEAP_SIZE_MAX := 1048576
# Enable this to generate code that uses hardware floating points instructions and registers
#HW_FLOATING_POINT := Y

STD_LIBS := -lm
ifeq ($(TOOLCHAIN),CLANG)
    TA_PIE := NON_PIE
    $(warning "WARNING : CLANG does not support PIE and HW_OPTIMIZATION, PIE desactivated")
endif

#-------------------------------------------------------------------------------
# Files and include paths - Add your files here
#-------------------------------------------------------------------------------
CC_OPTS += -DTEE_KINIBI
### Add include path here
INCLUDE_DIRS += ../../inc ../../inc/kinibi

### Add source code files for C++ compiler here
SRC_CPP += \
     Locals/Code/kinibi.c $(wildcard ../../src/*.c)
#
### Add source code files for C compiler here
SRC_C += # nothing

### Add source code files for assembler here
SRC_S += # nothing

#-------------------------------------------------------------------------------
# use generic make file
TRUSTED_APP_DIR ?= Locals/Code
TLSDK_DIR_SRC ?= $(TLSDK_DIR)
include $(TLSDK_DIR)/trusted_application.mk