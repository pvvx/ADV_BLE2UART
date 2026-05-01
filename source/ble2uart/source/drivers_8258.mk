OUT_DIR += /drivers/B85 /drivers/B85/driver_ext

OBJS += \
$(OUT_PATH)/drivers/B85/adc.o \
$(OUT_PATH)/drivers/B85/aes.o \
$(OUT_PATH)/drivers/B85/analog.o \
$(OUT_PATH)/drivers/B85/audio.o \
$(OUT_PATH)/drivers/B85/bsp.o \
$(OUT_PATH)/drivers/B85/clock.o \
$(OUT_PATH)/drivers/B85/flash.o \
$(OUT_PATH)/drivers/B85/gpio.o \
$(OUT_PATH)/drivers/B85/i2c.o \
$(OUT_PATH)/drivers/B85/lpc.o \
$(OUT_PATH)/drivers/B85/qdec.o \
$(OUT_PATH)/drivers/B85/s7816.o \
$(OUT_PATH)/drivers/B85/spi.o \
$(OUT_PATH)/drivers/B85/timer.o \
$(OUT_PATH)/drivers/B85/uart.o \
$(OUT_PATH)/drivers/B85/watchdog.o \
$(OUT_PATH)/drivers/B85/driver_ext/rf_pa.o \
$(OUT_PATH)/drivers/B85/driver_ext/ext_calibration.o


# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/drivers/B85/%.o: $(TEL_PATH)/drivers/B85/%.c
	@echo 'Building driver rfile: $<'
	@$(TC32_PATH)tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
