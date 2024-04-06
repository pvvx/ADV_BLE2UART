OUT_DIR += /vendor/common

OBJS += \
	$(OUT_PATH)/vendor/common/blt_common.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/vendor/common/%.o: $(TEL_PATH)/vendor/common/%.c
	@echo 'Building vendor/common file: $<'
	@$(TC32_PATH)tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
