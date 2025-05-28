## Makefile

# Prefix for older riscv gcc is  risv-none-embed
# Prefix for newer riscv gcc is  risv-none-elf
# TOOLCHAIN_PREFIX := riscv-none-embed
TOOLCHAIN_PREFIX := ../../../../CH592/MRS_Toolchain_Linux_x64_V1.92/RISC-V_Embedded_GCC12/bin/riscv-none-elf


APP_C_SRCS = \
  ./APP/app_drv_fifo.c \
  ./APP/app_usb.c \
  ./APP/observer.c \
  ./APP/observer_main.c


SDK_HAL_C_SRCS = \
  ./HAL/MCU.c \
  ./HAL/RTC.c \
  ./HAL/SLEEP.c

SDK_STDPERIPHDRIVER_C_SRCS += \
  ./SRC/StdPeriphDriver/CH58x_adc.c \
	./SRC/StdPeriphDriver/CH58x_clk.c \
	./SRC/StdPeriphDriver/CH58x_flash.c \
	./SRC/StdPeriphDriver/CH58x_gpio.c \
	./SRC/StdPeriphDriver/CH58x_i2c.c \
	./SRC/StdPeriphDriver/CH58x_pwm.c \
	./SRC/StdPeriphDriver/CH58x_pwr.c \
	./SRC/StdPeriphDriver/CH58x_spi0.c \
	./SRC/StdPeriphDriver/CH58x_spi1.c \
	./SRC/StdPeriphDriver/CH58x_sys.c \
	./SRC/StdPeriphDriver/CH58x_timer0.c \
	./SRC/StdPeriphDriver/CH58x_timer1.c \
	./SRC/StdPeriphDriver/CH58x_timer2.c \
	./SRC/StdPeriphDriver/CH58x_timer3.c \
	./SRC/StdPeriphDriver/CH58x_uart0.c \
	./SRC/StdPeriphDriver/CH58x_uart1.c \
	./SRC/StdPeriphDriver/CH58x_uart2.c \
	./SRC/StdPeriphDriver/CH58x_uart3.c \
	./SRC/StdPeriphDriver/CH58x_usb2dev.c \
	./SRC/StdPeriphDriver/CH58x_usb2hostBase.c \
	./SRC/StdPeriphDriver/CH58x_usb2hostClass.c \
	./SRC/StdPeriphDriver/CH58x_usbdev.c \
	./SRC/StdPeriphDriver/CH58x_usbhostBase.c \
	./SRC/StdPeriphDriver/CH58x_usbhostClass.c

SDK_RVMSIS_C_SRCS += \
  ./SRC/RVMSIS/core_riscv.c

SDK_STARTUP_S_UPPER_SRCS += \
  ./SRC/Startup/startup_CH583.S

C_SRCS := \
  $(APP_C_SRCS) \
  $(SDK_HAL_C_SRCS) \
  $(SDK_STDPERIPHDRIVER_C_SRCS) \
  $(SDK_RVMSIS_C_SRCS)

S_UPPER_SRCS := \
  $(SDK_BLE_LIB_S_UPPER_SRCS) \
  $(SDK_STARTUP_S_UPPER_SRCS)

OBJS := \
  $(foreach src,$(C_SRCS),$(subst ./,obj/,$(patsubst %.c,%.o,$(src)))) \
  $(foreach src,$(S_UPPER_SRCS),$(subst ./,obj/,$(patsubst %.S,%.o,$(src))))

MAKEFILE_DEPS := \
  $(foreach obj,$(OBJS),$(patsubst %.o,%.d,$(obj)))


STDPERIPHDRIVER_LIBS := -L"./SRC/StdPeriphDriver" -lISP583
BLE_LIB_LIBS := -L"./LIB" -lCH58xBLE
LIBS := $(STDPERIPHDRIVER_LIBS) $(BLE_LIB_LIBS)

SECONDARY_FLASH := observer.hex
SECONDARY_LIST := observer.lst
SECONDARY_SIZE := observer.siz
SECONDARY_BIN := observer.bin

# ARCH is rv32imac on older gcc, rv32imac_zicsr on newer gcc
# ARCH := rv32imac
ARCH := rv32imac_zicsr

CFLAGS_COMMON := \
  -march=$(ARCH) \
  -mabi=ilp32 \
  -mcmodel=medany \
  -msmall-data-limit=8 \
  -mno-save-restore \
  -Os \
  -fmessage-length=0 \
  -fsigned-char \
  -ffunction-sections \
  -fdata-sections
  #-g

.PHONY: all
all: observer.elf secondary-outputs

.PHONY: clean
clean:
	-rm $(OBJS)
	-rm $(MAKEFILE_DEPS)
	-rm $(SECONDARY_FLASH)
	-rm $(SECONDARY_LIST)
	-rm $(SECONDARY_BIN)
	-rm observer.elf
	-rm observer.map
	-rm -r ./obj

.PHONY: secondary-outputs
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE) $(SECONDARY_BIN)

observer.elf: $(OBJS)
	${TOOLCHAIN_PREFIX}-gcc \
	    $(CFLAGS_COMMON) \
	    -T "SRC/Ld/Link.ld" \
	    -nostartfiles \
	    -Xlinker \
	    --gc-sections \
	    -Xlinker \
	    --print-memory-usage \
	    -Wl,-Map,"observer.map" \
	    -Lobj \
	    --specs=nano.specs \
	    --specs=nosys.specs \
	    -o "observer.elf" \
	    $(OBJS) \
	    $(LIBS)

%.hex: %.elf
	@ ${TOOLCHAIN_PREFIX}-objcopy -O ihex "$<"  "$@"

%.bin: %.elf
	$(TOOLCHAIN_PREFIX)-objcopy -O binary $< "$@"

%.lst: %.elf
	@ ${TOOLCHAIN_PREFIX}-objdump \
	    --source \
	    --all-headers \
	    --demangle \
	    --line-numbers \
	    --wide "$<" > "$@"

%.siz: %.elf
	@ ${TOOLCHAIN_PREFIX}-size --format=berkeley "$<"

obj/%.o: ./%.c
	@ mkdir --parents $(dir $@)
	@ ${TOOLCHAIN_PREFIX}-gcc \
	    $(CFLAGS_COMMON) \
	    -I"./APP/include" \
	    -I"./SRC/StdPeriphDriver/inc" \
	    -I"./SRC/RVMSIS" \
	    -I"./LIB" \
	    -I"./HAL/include" \
	    -std=gnu99 \
	    -MMD \
	    -MP \
	    -MF"$(@:%.o=%.d)" \
	    -MT"$(@)" \
	    -c \
	    -o "$@" "$<"

obj/%.o: ./%.S
	@ mkdir --parents $(dir $@)
	@ ${TOOLCHAIN_PREFIX}-gcc \
	    $(CFLAGS_COMMON) \
	    -x assembler \
	    -MMD \
	    -MP \
	    -MF"$(@:%.o=%.d)" \
	    -MT"$(@)" \
	    -c \
	    -o "$@" "$<"

f: clean all
	chprog observer.bin

flash: 
	chprog observer.bin
