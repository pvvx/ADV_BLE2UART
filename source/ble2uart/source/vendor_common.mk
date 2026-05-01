OUT_DIR += /vendor/common

OBJS += \
	$(OUT_PATH)/vendor/common/ble_flash.o \
        $(OUT_PATH)/vendor/common/tlkapi_debug.o \
        $(OUT_PATH)/vendor/common/app_common.o \
        $(OUT_PATH)/vendor/common/flash_prot.o \
        $(OUT_PATH)/vendor/common/user_config.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/vendor/common/%.o: $(TEL_PATH)/vendor/common/%.c
	@echo 'Building vendor/common file: $<'
	@$(TC32_PATH)tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
