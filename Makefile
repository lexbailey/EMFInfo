SHELL=bash

ZX0=./ZX0/src/zx0

PROG_START=0x5e88

main_sources=main.c bitstream_parse.c file_io.c image_render.c intmath.c map_headers target_defs.h target_vars.c text_render.c

default: all

bas2tap/bas2tap: bas2tap/bas2tap.c
	cd bas2tap && $(MAKE) bas2tap

bin2tap/bin2tap: bin2tap/bin2tap.hs
	cd bin2tap && $(MAKE) bin2tap

bin2tap/bin2block: bin2tap/bin2block.hs
	cd bin2tap && $(MAKE) bin2block

$(ZX0): ZX0/src/zx0.c
	cd ZX0/src && $(MAKE) zx0

schedule.json:
	curl -Lo $@ https://www.emfcamp.org/schedule/2024.json

# TODO fix the mess around building the events files
evlist_default.bin strings_default.bin c_lut_default.bin: evbuild_intermediate_pc_linux ;
c_lut.bin evlist.bin strngs.bin: evbuild_intermediate_zxspec48 ;
evMSD.bin sMSD.bin clMSD.bin: evbuild_intermediate_zxspec48 ;

.INTERMEDIATE: evbuild_intermediate_pc_linux
.INTERMEDIATE: evbuild_intermediate_zxspec48
.INTERMEDIATE: evbuild_intermediate_pc_msdos

evbuild_intermediate_zxspec48: build_events schedule.json
	./build_events ZXSPEC48

evbuild_intermediate_pc_linux: build_events schedule.json
	./build_events PC_LINUX

evbuild_intermediate_pc_msdos: build_events schedule.json
	./build_events PC_MSDOS

main.zxspec48.bin: crt0.s dzx0.s mapdata.h $(main_sources)
	sdasz80 -l -o -s -g -j -y -a crt0.s 
	sdasz80 -l -o -s -g -j -y -a dzx0.s 
	sdcc -c --no-std-crt0 --std-c23 -D TARGET_ZXSPEC48 -mz80 --reserve-regs-iy main.c
	./gen_zxspec_link_script
	sdldz80 -f zxspec48.lnk
	makebin -s 65535 -o $(PROG_START) -p main.ihx $@

%.tap: %.bas bas2tap/bas2tap
	bas2tap/bas2tap -spreload -a1 $< $@

%.bin.tap: %.bin bin2tap/bin2tap
	bin2tap/bin2tap 0x0 $< $<

%.rawbin.tap: %.bin bin2tap/bin2block
	bin2tap/bin2block $<
	mv $<.tap $@

main.zxspec48.bin.tap: main.zxspec48.bin bin2tap/bin2tap
	bin2tap/bin2tap $(PROG_START) "EMFI_BIN" $<

%.zx0: % $(ZX0)
	-rm $@
	$(ZX0) ./$<

mapzx.bin: map/map_full.scr.zx0 map/map_north.scr.zx0 map/map_south.scr.zx0
	cat $^ > $@

mapdata.h mapsix.h: map_headers
.INTERMEDIATE: map_headers
map_headers: mapzx.bin gen_map_header
	./gen_map_header

descriptions.tap: evbuild_intermediate_zxspec48
	rm -f descriptions.tap
	$(MAKE) $(patsubst %.bin,%.bin.tap,$(shell echo desc*.bin | tr ' ' '\n' | grep 'desc[0-9]' | sort -V | tr '\n' ' ' ))
	cat $(shell echo desc*.bin.tap | tr ' ' '\n' | grep 'desc[0-9]' | sort -V | tr '\n' ' ' ) > $@

emfinfo_zxspec48.tap: preload.tap main.zxspec48.bin.tap mapzx.bin.tap evlist.bin.tap c_lut.bin.tap strngs.bin.tap descriptions.tap
	cat $^ > $@

%.wav: %.tap
	tape2wav $< $@

mapdata_six: map/map_full.six map/map_zoom_north.six map/map_zoom_south.six
	cat $^ > $@

emfinfo_linux: $(main_sources) evbuild_intermediate_pc_linux mapdata_six
	cc -gdwarf -DTARGET_PC_LINUX -c main.c -o main.o
	cc -gdwarf -o $@ main.o

WATCOM=$(PWD)/watcom
export WATCOM
export PATH:=$(WATCOM)/binl64:$(PATH)

emfinfo_msdos.exe: $(main_sources) evbuild_intermediate_pc_msdos
	$(WATCOM)/binl64/wcl -zastd=c99 -i=$(WATCOM)/h -bc -bt=dos -dTARGET_PC_MSDOS -fe=$@ main.c

emfinfo_msdos_textonly.exe: $(main_sources) evbuild_intermediate_pc_msdos
	$(WATCOM)/binl64/wcl -zastd=c99 -i=$(WATCOM)/h -bc -bt=dos -dTARGET_PC_MSDOS_TEXT -fe=$@ main.c

all: emfinfo_zxspec48.tap emfinfo_linux

clean:
	-rm -f *.wav *.tap *.lst *.bin *.lst *.map *.ihx *.rst *.rel *.sym *.mem *.noi *.lk
