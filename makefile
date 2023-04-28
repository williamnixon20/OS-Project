# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc
GENISO        = genisoimage
# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
ISO_NAME      = os2023
DISK_NAME     = storage

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -ffreestanding -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


run:
	@echo if you havent already, please run make disk first
	@qemu-system-i386 -s -drive file=${OUTPUT_FOLDER}/storage.bin,format=raw,if=ide,index=0,media=disk  -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso	
all: clean disk inserter insert-shell build run
build: iso
clean:
	rm -rf $(OUTPUT_FOLDER)/*.o $(OUTPUT_FOLDER)/*.iso $(OUTPUT_FOLDER)/kernel $(OUTPUT_FOLDER)/storage.bin $(OUTPUT_FOLDER)/shell_elf $(OUTPUT_FOLDER)/shell $(OUTPUT_FOLDER)/inserter
disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M


kernel:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel_loader.s -o $(OUTPUT_FOLDER)/kernel_loader.o
	@${CC} ${CFLAGS} src/kernel.c -o bin/kernel.o -c
	@${CC} ${CFLAGS} src/framebuffer.c -o bin/framebuffer.o -c
	@${CC} ${CFLAGS} src/portio.c -o bin/portio.o -c
	@${CC} ${CFLAGS} src/stdmem.c -o bin/stdmem.o -c
	@${CC} ${CFLAGS} src/gdt.c -o bin/gdt.o -c
	@${CC} ${CFLAGS} src/interrupt/idt.c -o bin/idt.o -c
	@${CC} ${CFLAGS} src/keyboard/keyboard.c -o bin/keyboard.o -c
	@${ASM} ${AFLAGS} src/interrupt/intsetup.s -o bin/intsetup.o 
	@${CC} ${CFLAGS} src/filesystem/disk.c -o bin/disk.o -c
	@${CC} ${CFLAGS} src/filesystem/fat32.c -o bin/fat32.o -c
	@${CC} ${CFLAGS} src/filesystem/cmostime.c -o bin/cmostime.o -c
	@${CC} ${CFLAGS} src/paging/paging.c -o bin/paging.o -c
	@${CC} ${CFLAGS} src/interrupt/interrupt.c -o bin/interrupt.o -c
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	@cd bin
	@cd bin; echo $(shell pwd); genisoimage -R                   \
	-b boot/grub/grub1         \
	-no-emul-boot              \
	-boot-load-size 4          \
	-A os                      \
	-input-charset utf8        \
	-quiet                     \
	-boot-info-table           \
	-o os2023.iso              \
	iso
	@cd ..
	@rm -r $(OUTPUT_FOLDER)/iso/

inserter:
	@$(CC) -Wno-builtin-declaration-mismatch -g \
		$(SOURCE_FOLDER)/stdmem.c $(SOURCE_FOLDER)/filesystem/fat32.c $(SOURCE_FOLDER)/filesystem/cmostime.c $(SOURCE_FOLDER)/portio.c\
		$(SOURCE_FOLDER)/inserter/external-inserter.c \
		-o $(OUTPUT_FOLDER)/inserter

user-shell:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/user-entry.s -o user-entry.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/user-shell.c -o user-shell.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/stdmem.c -o stdmem.o
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 \
		user-entry.o user-shell.o stdmem.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=elf32-i386\
		user-entry.o user-shell.o stdmem.o -o $(OUTPUT_FOLDER)/shell_elf
	@echo Linking object shell object files and generate ELF32 for debugging...
	@size --target=binary bin/shell
	@rm -f *.o

insert-shell: inserter user-shell
	@echo Inserting shell into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter shell 2 $(DISK_NAME).bin
