PROJECT = TFC
OBJECTS = ./main.o ./BufferedSerial/Buffer.o ./BufferedSerial/BufferedSerial.o ./FRDM-TFC/TFC.o ./Telemetry/Telemetry.o ./Telemetry/c_api/crc16.o ./Telemetry/c_api/framing.o ./Telemetry/c_api/telemetry.o
SYS_OBJECTS = ./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM/board.o ./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM/cmsis_nvic.o ./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM/mbed_overrides.o ./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM/retarget.o ./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM/startup_MKL25Z4.o ./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM/system_MKL25Z4.o
INCLUDE_PATHS = -I. -I./BufferedSerial -I./FRDM-TFC -I./Telemetry -I./mbed -I./mbed/TARGET_KL25Z -I./mbed/TARGET_KL25Z/TARGET_Freescale -I./mbed/TARGET_KL25Z/TARGET_Freescale/TARGET_KLXX -I./mbed/TARGET_KL25Z/TARGET_Freescale/TARGET_KLXX/TARGET_KL25Z -I./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM
LIBRARY_PATHS = -L./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM
LIBRARIES = -lmbed
LINKER_SCRIPT = ./mbed/TARGET_KL25Z/TOOLCHAIN_GCC_ARM/MKL25Z4.ld
DEVPATH = $(shell df 2>/dev/null | grep "FRDM-KL25Z" | awk '{print $$6}')

CC      = $(GCC_BIN)arm-none-eabi-gcc
CPP     = $(GCC_BIN)arm-none-eabi-g++
LD      = $(GCC_BIN)arm-none-eabi-gcc
OBJCOPY = $(GCC_BIN)arm-none-eabi-objcopy
OBJDUMP = $(GCC_BIN)arm-none-eabi-objdump
SIZE    = $(GCC_BIN)arm-none-eabi-size 

CPU = -mcpu=cortex-m0plus -mthumb
CC_FLAGS = $(CPU) -c -g -fno-common -fmessage-length=0 -Wall -Wextra -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -MMD -MP
CC_SYMBOLS = -DTARGET_FF_ARDUINO -DTOOLCHAIN_GCC_ARM -DTARGET_KLXX -DTARGET_KL25Z -DTARGET_CORTEX_M -DTARGET_M0P -DTARGET_Freescale -D__CORTEX_M0PLUS -DMBED_BUILD_TIMESTAMP=1454887513.61 -DTOOLCHAIN_GCC -D__MBED__=1 -DARM_MATH_CM0PLUS

LD_FLAGS = $(CPU) -Wl,--gc-sections --specs=nano.specs -Wl,--wrap,main -Wl,-Map=$(PROJECT).map,--cref
LD_SYS_LIBS = -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys

ifeq ($(DEBUG), 1)
  CC_FLAGS += -DDEBUG -O0
else
  CC_FLAGS += -DNDEBUG -Os
endif

# -----------------------------------------------------------------------------

.PHONY: all clean lst size

all: $(PROJECT).bin

clean:
	rm -f $(PROJECT).bin $(PROJECT).elf $(PROJECT).hex $(PROJECT).map $(PROJECT).lst $(OBJECTS) $(DEPS)

cp: $(PROJECT).bin
	cp $(PROJECT).bin $(DEVPATH)

.c.o:
	$(CC)  $(CC_FLAGS) $(CC_SYMBOLS) -std=gnu99   $(INCLUDE_PATHS) -o $@ $<

.cpp.o:
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) -std=gnu++98 -fno-rtti $(INCLUDE_PATHS) -o $@ $<

$(PROJECT).elf: $(OBJECTS) $(SYS_OBJECTS)
	$(LD) $(LD_FLAGS) -T$(LINKER_SCRIPT) $(LIBRARY_PATHS) -o $@ $^ $(LIBRARIES) $(LD_SYS_LIBS) $(LIBRARIES) $(LD_SYS_LIBS)

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECT).hex: $(PROJECT).elf
	@$(OBJCOPY) -O ihex $< $@

$(PROJECT).lst: $(PROJECT).elf
	@$(OBJDUMP) -Sdh $< > $@

lst: $(PROJECT).lst

size: $(PROJECT).elf
	$(SIZE) $(PROJECT).elf

DEPS = $(OBJECTS:.o=.d) $(SYS_OBJECTS:.o=.d)
-include $(DEPS)