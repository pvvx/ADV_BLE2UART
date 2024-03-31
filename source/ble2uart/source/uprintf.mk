
OUT_DIR += /application/print

OBJS += \
$(OUT_PATH)/application/print/putchar.o \
$(OUT_PATH)/application/print/u_printf.o \

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/application/%.o: $(TEL_PATH)/application/%.c 
	@echo 'Building printf file: $<'
	@$(TC32_PATH)tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
