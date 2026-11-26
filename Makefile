# gnu makefile ruined my life
ARCH ?= 64
LLVM_BIN := $(shell brew --prefix llvm 2>/dev/null)/bin
ifneq ($(wildcard $(LLVM_BIN)/clang),)
  CC      := $(LLVM_BIN)/clang
  OBJCOPY := $(LLVM_BIN)/llvm-objcopy
else
  CC      := clang
  OBJCOPY := llvm-objcopy
endif
BUILD := build/$(ARCH)

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

# dirs and linker scripts
INC_DIR       := src/kernel/include
KERNEL_DIR    := src/kernel
LOADER_DIR    := src/kernel/loader/arch/arm
ARCH_DIR      := $(LOADER_DIR)/$(ARCH)
LIB_DIR       := src/kernel/include/lib
DRIVER_DIR    := src/drivers/PL011
K_ARCH_DIR    := src/arch/arm/$(ARCH)
K_LINKER      ?= $(K_ARCH_DIR)/kernel.ld

INC_DIRS      := $(INC_DIR) \
		 $(LIB_DIR) \
		 $(DRIVER_DIR) \
		 $(ARCH_DIR) \
		 $(K_ARCH_DIR)

LINKER_SCRIPT := $(ARCH_DIR)/loader.ld
K_LINKER      ?= src/arch/arm/$(ARCH)/kernel.ld

KERNEL_ELF    := $(BUILD)/kernel.elf
KERNEL        ?= $(KERNEL_ELF)
LOADER_ELF    := $(BUILD)/loader.elf

# sources and object files
LOADER_SRCS := $(ARCH_DIR)/boot.S \
               $(ARCH_DIR)/loader_arch.c \
               $(LOADER_DIR)/loader_nonarch.c \
               $(LOADER_DIR)/embed.S \
               $(LIB_DIR)/string.c \
               $(LIB_DIR)/kprint.c \
               $(DRIVER_DIR)/uart.c

KERNEL_SRCS := $(K_ARCH_DIR)/kentry.S \
	       $(K_ARCH_DIR)/traps.S \
	       $(K_ARCH_DIR)/exceptions.c \
               $(KERNEL_DIR)/kmain.c \
               $(LIB_DIR)/string.c \
               $(LIB_DIR)/kprint.c \
               $(DRIVER_DIR)/uart.c

LOADER_OBJS := $(patsubst %,$(BUILD)/%.o,$(LOADER_SRCS))
KERNEL_OBJS := $(patsubst %,$(BUILD)/%.o,$(KERNEL_SRCS))
ALL_OBJS    := $(sort $(LOADER_OBJS) $(KERNEL_OBJS))

# compiler, assembler, linker Flags
COMMON_FLAGS := --target=$(TARGET) $(CPUFLAGS) -ffreestanding -fno-pic -fno-pie -g
CPPFLAGS     := $(addprefix -I,$(INC_DIRS)) -MMD -MP
CFLAGS       := $(COMMON_FLAGS) -std=c99 -pedantic -fno-builtin -fno-stack-protector \
                -Wall -Wextra -Wmissing-prototypes -O2
ASFLAGS      := $(COMMON_FLAGS)

LDFLAGS_BASE   := --target=$(TARGET) -fuse-ld=lld -nostdlib -static -Wl,-z,max-page-size=4096
LOADER_LDFLAGS := $(LDFLAGS_BASE) -Wl,-T,$(LINKER_SCRIPT)
KERNEL_LDFLAGS := $(LDFLAGS_BASE) -Wl,-T,$(K_LINKER)

.PHONY: all kernel img clean clean-all
all: $(BUILD)/$(IMG)
kernel: $(KERNEL_ELF)
img: $(BUILD)/$(IMG)

$(LOADER_ELF): $(LOADER_OBJS) $(LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(LOADER_LDFLAGS) $(LOADER_OBJS) -o $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(K_LINKER)
	@mkdir -p $(dir $@)
	$(CC) $(KERNEL_LDFLAGS) $(KERNEL_OBJS) -o $@

$(BUILD)/$(IMG): $(LOADER_ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.S.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(LOADER_DIR)/embed.S.o: $(KERNEL)
$(BUILD)/$(LOADER_DIR)/embed.S.o: CPPFLAGS += -DKERNEL_ELF=\"$(KERNEL)\"

ifdef STUB_ELF
$(STUB_ELF): $(STUB_DIR)/stub.S $(STUB_DIR)/stub.ld
	@mkdir -p $(dir $@)
	$(CC) $(COMMON_FLAGS) -nostdlib -static -fuse-ld=lld \
	      -Wl,-T,$(STUB_DIR)/stub.ld -Wl,-z,max-page-size=4096 \
	      -Wl,-z,noseparate-code $< -o $@
endif

clean:
	rm -rf $(BUILD)

clean-all:
	rm -rf build

-include $(ALL_OBJS:.o=.d)
