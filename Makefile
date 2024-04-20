SHELL=bash

default: all

bas2tap/bas2tap: bas2tap/bas2tap.c
	cd bas2tap && $(MAKE) bas2tap

bin2tap/bin2tap: bin2tap/bin2tap.hs
	cd bin2tap && $(MAKE) bin2tap

bin2tap/bin2block: bin2tap/bin2block.hs
	cd bin2tap && $(MAKE) bin2block

schedule.json:
	curl -Lo $@ https://www.emfcamp.org/schedule/2022.json

evlist.bin strngs.bin: evbuild_intermediate ;

.INTERMEDIATE: evbuild_intermediate
evbuild_intermediate: build_events schedule.json
	./build_events

main.zxspec48.bin: main.c crt0.s
	sdasz80 -l -o -s -g -j -y -a crt0.s 
	sdcc -c --no-std-crt0 --std-c23 -D TARGET_ZXSPEC48 -mz80 --reserve-regs-iy $<
	./gen_zxspec_link_script
	sdldz80 -f zxspec48.lnk
	makebin -s 65535 -o 0x8000 -p main.ihx $@

%.tap: %.bas bas2tap/bas2tap
	bas2tap/bas2tap -spreload -a1 $< $@

%.bin.tap: %.bin bin2tap/bin2tap
	bin2tap/bin2tap 0x0 $< $<

%.rawbin.tap: %.bin bin2tap/bin2block
	bin2tap/bin2block $<
	mv $<.tap $@

main.zxspec48.bin.tap: main.zxspec48.bin bin2tap/bin2tap
	bin2tap/bin2tap 0x8000 "EMFI_BIN" $<

emfinfo_zxspec48.tap: preload.tap main.zxspec48.bin.tap evlist.bin.tap strngs.bin.tap
	cat $^ > $@

%.wav: %.tap
	tape2wav $< $@

emfinfo_linux: main.c
	cc -gdwarf -DTARGET_PC_LINUX -c main.c -o main.o
	cc -gdwarf -o $@ main.o

all: emfinfo_zxspec48.tap emfinfo_linux

clean:
	-rm -f *.wav *.tap *.lst *.bin *.lst *.map *.ihx *.rst *.rel *.sym *.mem *.noi *.lk
