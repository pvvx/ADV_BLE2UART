LIBS := -llt_8258

BOOT_FLAG ?= -DMCU_STARTUP_8258 -DMCU_STARTUP_825X
LS_FLAGS := $(TEL_PATH)/boot/boot.link

OUT_DIR += /source

OBJS += \
$(OUT_PATH)/source/crc.o \
$(OUT_PATH)/source/utils.o \
$(OUT_PATH)/source/app.o \
$(OUT_PATH)/source/app_buffer.o \
$(OUT_PATH)/source/scanning.o \
$(OUT_PATH)/source/blt_common.o \
$(OUT_PATH)/source/ble.o \
$(OUT_PATH)/source/drv_uart.o \
$(OUT_PATH)/source/tinyFlash.o \
$(OUT_PATH)/source/main.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/source/%.o: $(PROJECT_PATH)/%.c
	@echo 'Building application file from C: $<'
	@$(TC32_PATH)tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
