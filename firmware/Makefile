AS8051 ?= sdas8051
LD8051 ?= sdld
SDCC ?= sdcc
PYTHON ?= python3

IMAGE := build/J5v05l19
REFERENCE ?= ../bins/J5v05l19.bin
SOURCES := src/j5v05l19.asm src/vectors.asm src/interpolation.asm src/opaque.asm
C_FUNCTIONS := cd7c d2f1 f275
C_OBJECTS := $(addprefix build/,$(addsuffix .rel,$(C_FUNCTIONS)))
C_PLACEMENT := $(foreach address,$(C_FUNCTIONS),-b code_$(address)=0x$(address))

.PHONY: all verify clean
.DELETE_ON_ERROR:

all: $(IMAGE).bin

$(IMAGE).rel: $(SOURCES) Makefile
	mkdir -p build
	$(AS8051) -losv "$@" src/j5v05l19.asm

build/%.rel: src/c/%.c Makefile
	mkdir -p build
	$(SDCC) -mmcs51 --model-small --codeseg code_$* -c "$<" -o "$@"

$(IMAGE).ihx: $(IMAGE).rel $(C_OBJECTS)
	$(LD8051) -n -i -m -w $(C_PLACEMENT) "$(IMAGE)" $(IMAGE).rel $(C_OBJECTS)

$(IMAGE).map: $(IMAGE).ihx
	@test -s "$@" || $(LD8051) -n -i -m -w $(C_PLACEMENT) "$(IMAGE)" $(IMAGE).rel $(C_OBJECTS)

$(IMAGE).bin: $(IMAGE).ihx $(IMAGE).map image.py
	$(PYTHON) image.py convert "$<" "$@"

verify: $(IMAGE).bin
	$(PYTHON) image.py verify "$(IMAGE).bin" "$(IMAGE).map" "$(REFERENCE)"

clean:
	rm -rf build
