AS8051 ?= sdas8051
LD8051 ?= sdld
SDCC ?= sdcc
PYTHON ?= python3

IMAGE := build/J5v05l19
REFERENCE ?= ../bins/J5v05l19.bin
SOURCES := src/j5v05l19.asm src/vectors.asm src/interpolation.asm src/opaque.asm
C_REGISTER_HELPERS := a33a a381 a3f9
C_FUNCTIONS := 2bcd 3396 $(C_REGISTER_HELPERS) a429 cd7c d2f1 f090 f275
C_VECTORS := 000b 0013 002b 0043 004b 0053 005b 0063 0083 0093 009b 00a3 00ab
C_DATA := 60c7 613c 6ac8 6f56 710b 75ef 99b9
C_FLAGS := -mmcs51 --model-small
C_OBJECTS := $(addprefix build/,$(addsuffix .rel,$(C_FUNCTIONS) $(C_VECTORS) $(C_DATA)))
C_PLACEMENT := $(foreach address,$(C_FUNCTIONS) $(C_VECTORS),-b code_$(address)=0x$(address)) \
	$(foreach address,$(C_DATA),-b const_$(address)=0x$(address))

.PHONY: all verify clean
.DELETE_ON_ERROR:

all: $(IMAGE).bin

$(IMAGE).rel: $(SOURCES) Makefile
	mkdir -p build
	$(AS8051) -losv "$@" src/j5v05l19.asm

$(addprefix build/,$(addsuffix .rel,$(C_VECTORS))): build/%.rel: src/c/unused_vector.c Makefile
	mkdir -p build
	$(SDCC) $(C_FLAGS) --codeseg code_$* -DC509_UNUSED_ISR=irq_unused_$* -c "$<" -o "$@"

build/%.rel: src/c/%.c Makefile
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_OPTIONS) --codeseg code_$* --constseg const_$* -c "$<" -o "$@"

build/a429.rel: C_OPTIONS += --peep-file src/c/accumulator.peep
build/a429.rel: src/c/accumulator.peep

build/2bcd.rel: C_OPTIONS += --no-peep --peep-file src/c/rpm_axis.peep
build/2bcd.rel: src/c/rpm_axis.peep

build/3396.rel: C_OPTIONS += --no-peep --peep-file src/c/coolant_axis.peep
build/3396.rel: src/c/coolant_axis.peep

$(addprefix build/,$(addsuffix .rel,$(C_REGISTER_HELPERS))): C_OPTIONS += \
	--std-sdcc23 --no-peep --peep-file src/c/stock_registers.peep
$(addprefix build/,$(addsuffix .rel,$(C_REGISTER_HELPERS))): \
	src/c/stock_registers.h src/c/stock_registers.peep

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
