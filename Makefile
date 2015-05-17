CC=gcc
AS=as
CFLAGS=-O1 -std=c99 -D__thread= -Wall -Werror -nostdinc -Iinclude -msoft-float -mno-sse -mno-red-zone -fno-builtin -fPIC -march=amdfam10 -g3
LD=ld
LDLAGS=-nostdlib
AR=ar

ROOTFS=rootfs
ROOTBIN=$(ROOTFS)/bin
ROOTLIB=$(ROOTFS)/lib
ROOTBOOT=$(ROOTFS)/boot

SHELL=/bin/sh

KERN_SRCS:=$(shell find sys/ -name \*.c -o -name \*.s)
LIBC_SRCS:=$(shell find libc/ -name \*.c -o -name \*.s)
CRT_SRCS:=$(shell find crt/ -name \*.c -o -name \*.s)
BIN_SRCS:=$(shell find bin/\* -name \*.c)
INCLUDES:=$(shell find include/ -type f -name \*.h)
BINS:=$(addprefix $(ROOTFS)/,$(wildcard bin/*))

.PHONY: all binary

all: $(USER).iso $(USER).img

$(USER).iso: kernel
	cp kernel $(ROOTBOOT)/kernel/kernel
	mkisofs -r -no-emul-boot -input-charset utf-8 -b boot/cdboot -o $@ $(ROOTFS)/

$(USER).img: newfs.506
	qemu-img create -f raw $@ 16M
	./newfs.506 $@

newfs.506: $(wildcard newfs/*.c)
	$(CC) -o $@ $^

kernel: $(patsubst %.s,obj/%.asm.o,$(KERN_SRCS:%.c=obj/%.o)) obj/tarfs.o
	$(LD) $(LDLAGS) -o $@ -T linker.script $^

obj/tarfs.o: $(BINS)
	tar --format=ustar -cvf tarfs --no-recursion -C $(ROOTFS) $(shell find $(ROOTFS)/ -name boot -prune -o ! -name .empty -printf "%P\n")
	objcopy --input binary --binary-architecture i386 --output elf64-x86-64 tarfs $@
	@rm tarfs

$(ROOTLIB)/libc.a: $(patsubst %.s,obj/%.asm.o,$(LIBC_SRCS:%.c=obj/%.o))
	$(AR) rcs $@ $^

$(BINS): $(patsubst %.s,obj/%.asm.o,$(CRT_SRCS:%.c=obj/%.o)) $(ROOTLIB)/libc.a $(shell find bin/ -type f -name \*.c) $(INCLUDES)
	@$(MAKE) --no-print-directory BIN=$@ binary

binary: $(patsubst %.c,obj/%.o,$(wildcard $(BIN:rootfs/%=%)/*.c))
	$(LD) $(LDLAGS) -o $(BIN) $(patsubst %.s,obj/%.asm.o,$(CRT_SRCS:%.c=obj/%.o)) $^ $(ROOTLIB)/libc.a

obj/%.o: %.c $(INCLUDES)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

obj/%.asm.o: %.s
	@mkdir -p $(dir $@)
	$(AS) -o $@ $<

.PHONY: submit clean

SUBMITTO:=~mferdman/cse506-submit/

submit: clean
	tar -czvf $(USER).tgz --exclude=.empty --exclude=.*.sw? --exclude=*~ LICENSE README Makefile linker.script sys bin crt libc newfs include $(ROOTFS) $(USER).img
	@gpg --quiet --import cse506-pubkey.txt
	gpg --yes --encrypt --recipient 'CSE506' $(USER).tgz
	rm -fv $(SUBMITTO)$(USER)=*.tgz.gpg
	cp -v $(USER).tgz.gpg $(SUBMITTO)$(USER)=`date +%F=%T`.tgz.gpg

clean:
	find $(ROOTLIB) $(ROOTBIN) -type f ! -name .empty -print -delete
	rm -rfv obj kernel newfs.506 $(ROOTBOOT)/kernel/kernel $(USER).iso
