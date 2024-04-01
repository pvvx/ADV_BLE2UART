OUT_DIR += /drivers

OBJS += $(OUT_PATH)/drivers/cstartup_825x.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/drivers/%.o: $(TEL_PATH)/boot/8258/cstartup_8258.S
	@echo 'Building boot file: $<'
	@$(TC32_PATH)tc32-elf-gcc $(BOOT_FLAG) -c -o"$@" "$<"

