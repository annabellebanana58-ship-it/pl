CC      := clang
OBJCOPY := llvm-objcopy
TARGET  := aarch64-none-elf
BUILD   := build

LOADER_DIR    := src/kernel/loader/arch/arm
LIB_DIR       := src/kernel/include/lib
LINKER_SCRIPT := $(LOADER_DIR)/64/loader.ld

SRCS := $(LOADER_DIR)/64/boot.S \
        $(LOADER_DIR)/elfloader.c \
        $(LIB_DIR)/string.c
OBJS := $(patsubst %,$(BUILD)/%.o,$(SRCS))

CFLAGS  := --target=$(TARGET) -mcpu=cortex-a53 \
           -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
           -mgeneral-regs-only -mstrict-align \
           -Wall -Wextra -g -O2 -I$(LIB_DIR) -MMD -MP

LDFLAGS := --target=$(TARGET) -fuse-ld=lld -nostdlib -static \
           -Wl,-T,$(LINKER_SCRIPT) -Wl,-z,max-page-size=4096

.PHONY: all img clean
all: $(BUILD)/loader.elf
img: $(BUILD)/kernel8.img

$(BUILD)/loader.elf: $(OBJS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/kernel8.img: $(BUILD)/loader.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
