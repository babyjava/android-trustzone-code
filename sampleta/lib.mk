include fp_ta_toolchain.mk

SRC_DIR                 := ta_src_static
SRCS					:= $(wildcard $(SRC_DIR)/*.c)
ifeq ($(FP_TEE), TEE_QSEE)
SRCS					+=$(wildcard $(SRC_DIR)/qualcomm/*.c)
endif
ifeq ($(FP_TEE), TEE_MSEE)
SRCS					+=$(wildcard $(SRC_DIR)/microtrust/*.c)
endif
ifeq ($(FP_TEE), TEE_TSEE)
SRCS					+=$(wildcard $(SRC_DIR)/trustonic/*.c)
endif

INC						:= -I$(SRC_DIR)/qualcomm/inc -Ita_src/inc -Ihal_src -Ihal_src_static -Ita_src/include
MY_CFLAGS				:= -D$(FP_TEE)
OBJS					:= $(SRCS:.c=.o)
D_OBJ					:= obj
TARGET					:= libqsee_ta.a
OUT_DIR					:= ta_src/lib

$(TARGET) : $(OBJS)
	$(AR) -rc $@ $(addprefix $(D_OBJ)/,$^)
	mv $(TARGET) $(OUT_DIR)/$(FP_ARCH)

%.o : %.c
	$(CC) -c $^ -o $(D_OBJ)/$(patsubst %.c,%.o,$^) -Wall $(INC) $(MY_CFLAGS)

.PHONY: clean
clean:
	find . -name *.o | xargs rm