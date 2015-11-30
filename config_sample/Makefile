#===============================================================================
# export variable
#===============================================================================

SDK_DIR := $(CURDIR)/../..
include $(SDK_DIR)/base.mak

include $(SAMPLE_DIR)/base.mak

#===============================================================================
# local variable
#===============================================================================
CFLAGS += $(CFG_HI_SAMPLE_CFLAGS)
CFLAGS += -I${SDK_INC_DIR} -I$(SAMPLE_DIR)/common -I$(SAMPLE_DIR)/tr069_sample/libtr069/config 

LIBPATH = -L$(SDK_STATIC_LIB_DIR) -L$(SDK_SHARE_LIB_DIR)
LIB = -Wl,--start-group $(SYS_LIBS) $(HI_LIBS) -Wl,--end-group

APP = sample_config
OBJS = main.o

#===============================================================================
# rules
#===============================================================================
.PHONY: all clean install uninstall

all:$(APP)

%.o: %.c
	$(AT)$(CC) $(CFLAGS) -c -o $@ $^

$(APP): ${OBJS}
	$(AT)$(CC) -o $@ $^ ${LIBPATH} ${LIB}

clean:
	-$(AT)rm -f ${OBJS}
	-$(AT)rm -f $(APP)

install:

uninstall:
