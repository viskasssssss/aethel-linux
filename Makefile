CC = gcc
LD = ld
QEMU = qemu-system-x86_64

BUILD_DIR = build
ROOTFS_DIR = rootfs
KERNEL_DIR = kernel

CFLAGS = -nostdlib -ffreestanding

.PHONY: all init shell initramfs run clean

all: init shell initramfs

init: $(ROOTFS_DIR)/init

shell: $(ROOTFS_DIR)/shell

$(ROOTFS_DIR)/init: $(BUILD_DIR)/start.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/init.o
	$(LD) $^ -o $@

$(ROOTFS_DIR)/shell: $(BUILD_DIR)/start.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/shell.o
	$(LD) $^ -o $@

$(BUILD_DIR)/start.o: src/start.S
	$(CC) -c -nostdlib $< -o $@

$(BUILD_DIR)/syscall.o: src/syscall.c src/syscall.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: src/init.c src/syscall.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/shell.o: src/shell.c src/shell.h src/syscall.h
	$(CC) -c $(CFLAGS) $< -o $@

initramfs: init shell
	cd $(ROOTFS_DIR) && find . -mindepth 1 -print | cpio -o -H newc > ../$(BUILD_DIR)/initramfs.cpio

run: all
	$(QEMU) \
		-kernel $(KERNEL_DIR)/arch/x86/boot/bzImage \
		-initrd $(BUILD_DIR)/initramfs.cpio \
		-append "quiet rdinit=/init"

clean:
	rm -f $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/initramfs.cpio
	rm -f $(ROOTFS_DIR)/init
	rm -f $(ROOTFS_DIR)/shell