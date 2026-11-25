AS8051 ?= sdas8051
LD8051 ?= sdld
PYTHON ?= python3

IMAGE := build/J5v05l19
REFERENCE ?= ../bins/J5v05l19.bin
SOURCES := src/j5v05l19.asm src/vectors.asm src/interpolation.asm src/opaque.asm

.PHONY: all verify clean
.DELETE_ON_ERROR:

all: $(IMAGE).bin

$(IMAGE).rel: $(SOURCES) Makefile
	mkdir -p build
	$(AS8051) -losv "$@" src/j5v05l19.asm

$(IMAGE).ihx: $(IMAGE).rel
	$(LD8051) -n -i -m -w "$(IMAGE)" "$<"

$(IMAGE).map: $(IMAGE).ihx
	@test -s "$@" || $(LD8051) -n -i -m -w "$(IMAGE)" "$(IMAGE).rel"

$(IMAGE).bin: $(IMAGE).ihx $(IMAGE).map image.py
	$(PYTHON) image.py convert "$<" "$@"

verify: $(IMAGE).bin
	$(PYTHON) image.py verify "$(IMAGE).bin" "$(IMAGE).map" "$(REFERENCE)"

clean:
	rm -rf build
