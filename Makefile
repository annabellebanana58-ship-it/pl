CC      := clang
OBJCOPY := llvm-objcopy
BUILD   := build/$(ARCH)

ARCH	 ?= 64

STUB_DIR := src/kernel/stub
STUB_ELF := $(BUILD)/stub.elf
KERNEL   ?= $(STUB_ELF)



ifeq ($(ARCH),64)
  TARGET   := aarch64-none-elf
  CPUFLAGS := -mcpu=cortex-a53 -mgeneral-regs-only -mstrict-align
  IMG      := kernel8.img
else ifeq ($(ARCH),32)
  TARGET   := arm-none-eabi
  CPUFLAGS := -mcpu=cortex-a53 -marm -mfloat-abi=soft -mno-unaligned-access
  IMG      := kernel7.img
else
  $(error ARCH must be 32 or 64)
endif

LOADER_DIR    := src/kernel/loader/arch/arm
ARCH_DIR      := $(LOADER_DIR)/$(ARCH)
LIB_DIR       := src/kernel/include/lib
LINKER_SCRIPT := $(ARCH_DIR)/loader.ld

SRCS := $(ARCH_DIR)/boot.S \
        $(ARCH_DIR)/loader_arch.c \
        $(LOADER_DIR)/loader_nonarch.c \
        $(LOADER_DIR)/embed.S \
        $(LIB_DIR)/string.c
OBJS := $(patsubst %,$(BUILD)/%.o,$(SRCS))

CFLAGS  := --target=$(TARGET) $(CPUFLAGS) \
           -std=c99 -pedantic \
           -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
           -Wall -Wextra -Wmissing-prototypes -g -O2 \
           -I$(ARCH_DIR) -I$(LIB_DIR) -MMD -MP

LDFLAGS := --target=$(TARGET) -fuse-ld=lld -nostdlib -static \
           -Wl,-T,$(LINKER_SCRIPT) -Wl,-z,max-page-size=4096

.PHONY: all img clean
all: $(BUILD)/loader.elf
stub: $(STUB_ELF)
img: $(BUILD)/$(IMG)

$(BUILD)/loader.elf: $(OBJS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/$(IMG): $(BUILD)/loader.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(LOADER_DIR)/embed.S.o: $(KERNEL)
$(BUILD)/$(LOADER_DIR)/embed.S.o: CFLAGS += -DKERNEL_ELF=\"$(KERNEL)\"

$(STUB_ELF): $(STUB_DIR)/stub.S $(STUB_DIR)/stub.ld
	@mkdir -p $(dir $@)
	$(CC) --target=$(TARGET) $(CPUFLAGS) -ffreestanding -nostdlib -static \
	      -fuse-ld=lld -Wl,-T,$(STUB_DIR)/stub.ld \
	      -Wl,-z,max-page-size=4096 -Wl,-z,noseparate-code $< -o $@

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
