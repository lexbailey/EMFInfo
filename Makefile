SHELL=bash

default: all

bas2tap/bas2tap: bas2tap/bas2tap.c
	cd bas2tap && $(MAKE) bas2tap

bin2tap/bin2tap: bin2tap/bin2tap.hs
	cd bin2tap && $(MAKE) bin2tap

main.zxspec48.bin: main.c crt0.s
	sdasz80 -l -o -s -g -j -y -a crt0.s
	sdcc -c --no-std-crt0 --std-c23 -D TARGET_ZXSPEC48 -mz80 $<
	sdldz80 -u -m -i main.ihx crt0.rel -b _GS_INIT=s__HEADER+l__HEADER
	makebin -s 65535 -o 0x8000 -p main.ihx $@

%.tap: %.bas bas2tap/bas2tap
	bas2tap/bas2tap -spreload -a1 $< $@

#%.bin.tap: %.bin bin2tap/bin2tap
#	bin2tap/bin2tap 0x8000 $< $<

main.zxspec48.bin.tap: main.zxspec48.bin
	bin2tap/bin2tap 0x8000 "EMFI_BIN" $<

full.tap: preload.tap main.zxspec48.bin.tap
	cat $^ > $@

%.wav: %.tap
	tape2wav $< $@

emfinfo.elf: main.c sched_explore.c
	cc -DTARGET_PC_LINUX -c main.c -o main.o
	cc -DTARGET_PC_LINUX -c sched_explore.c -o sched_explore.o
	cc -o $@ main.o sched_explore.o

all: full.wav emfinfo.elf

clean:
	-rm -f *.wav *.tap *.lst *.bin *.lst *.map *.ihx *.rst *.rel *.sym *.mem *.noi *.lk
