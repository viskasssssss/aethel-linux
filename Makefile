# Copyright (C) 2026 viskasssssss
# 
# This file is part of Aethel.
# 
# Aethel is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
# 
# Aethel is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Aethel. If not, see <https://www.gnu.org/licenses/>.

CC = gcc
LD = ld
QEMU = qemu-system-x86_64

SRC_DIR = src
BUILD_DIR = build
ROOTFS_DIR = rootfs
KERNEL_DIR = kernel

BIN_SOURCES := $(wildcard src/bin/*.c)
BIN_PROGRAMS := $(patsubst src/bin/%.c,$(ROOTFS_DIR)/bin/%,$(BIN_SOURCES))

CFLAGS = -nostdlib -ffreestanding -I$(SRC_DIR) -I$(SRC_DIR)/util/

.PHONY: all init bash log initramfs run clean

all: init bash $(BIN_PROGRAMS) initramfs

init: $(ROOTFS_DIR)/init

bash: $(ROOTFS_DIR)/bash

log: $(BUILD_DIR)/log.o

$(ROOTFS_DIR)/bin/%: src/bin/%.c \
	$(BUILD_DIR)/start.o \
	$(BUILD_DIR)/syscall.o \
	$(BUILD_DIR)/log.o
	$(CC) -c $(CFLAGS) $< -o $(BUILD_DIR)/$*.o
	$(LD) \
		$(BUILD_DIR)/start.o \
		$(BUILD_DIR)/syscall.o \
		$(BUILD_DIR)/log.o \
		$(BUILD_DIR)/$*.o \
		-o $@

$(BUILD_DIR)/log.o: $(SRC_DIR)/util/log.c $(SRC_DIR)/util/log.h $(SRC_DIR)/util/syscall.h
	$(CC) -c $(CFLAGS) $< -o $@

$(ROOTFS_DIR)/init: $(BUILD_DIR)/start.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/log.o $(BUILD_DIR)/init.o
	$(LD) $^ -o $@

$(ROOTFS_DIR)/bash: $(BUILD_DIR)/start.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/log.o $(BUILD_DIR)/bash.o
	$(LD) $^ -o $@

$(BUILD_DIR)/start.o: $(SRC_DIR)/asm/start.S
	$(CC) -c -nostdlib $< -o $@

$(BUILD_DIR)/syscall.o: $(SRC_DIR)/util/syscall.c $(SRC_DIR)/util/syscall.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: $(SRC_DIR)/init/init.c $(SRC_DIR)/util/syscall.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/bash.o: $(SRC_DIR)/bash/bash.c $(SRC_DIR)/bash/bash.h $(SRC_DIR)/util/syscall.h
	$(CC) -c $(CFLAGS) $< -o $@

initramfs: init bash $(BIN_PROGRAMS)
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
	rm -f $(ROOTFS_DIR)/bash