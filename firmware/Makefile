AS8051 ?= sdas8051
LD8051 ?= sdld
SDCC ?= sdcc
PYTHON ?= python3

IMAGE := build/J5v05l19
REFERENCE ?= ../bins/J5v05l19.bin
SOURCES := src/j5v05l19.asm src/vectors.asm src/opaque.asm
C_REGISTER_HELPERS := a33a a381 a3f9
C_IGNITION := 280e 3f74 3f95 3fad 3fbb 3fef 400d 4042 406d 4081 430d
C_INTERPOLATION := a5ff a611
C_SIGNED_BOUNDS := a412 a42f
C_FUNCTIONS := 2bcd 2c3f 3396 $(C_IGNITION) 4b1a a22e a314 $(C_REGISTER_HELPERS) a352 a39e a3d9 $(C_SIGNED_BOUNDS) a429 a575 a59d $(C_INTERPOLATION) a62f cd7c d2f1 f090 f275
C_VECTORS := 000b 0013 002b 0043 004b 0053 005b 0063 0083 0093 009b 00a3 00ab
C_DATA := 60ad 60c7 60ed 613c 62ac 64d3 6613 6ac8 6f56 710b 75ef 7877 7ac7 7e07 8193 83ba 852f 87e4 99b9
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

build/a352.rel: C_OPTIONS += --no-peep --peep-file src/c/multiply.peep
build/a352.rel: src/c/multiply.peep src/c/stock_registers.h

build/a314.rel: C_OPTIONS += --no-peep --peep-file src/c/byte_bounds.peep
build/a314.rel: src/c/byte_bounds.peep src/c/stock_registers.h

build/a22e.rel: C_OPTIONS += --no-peep --peep-file src/c/iac_history.peep
build/a22e.rel: src/c/iac_history.peep

build/4b1a.rel: C_OPTIONS += --no-peep --peep-file src/c/fan.peep
build/4b1a.rel: src/c/fan.peep

build/a39e.rel: C_OPTIONS += --no-peep --peep-file src/c/shift_left.peep
build/a39e.rel: src/c/shift_left.peep src/c/stock_registers.h

build/a3d9.rel: C_OPTIONS += --no-peep --peep-file src/c/shift_right.peep
build/a3d9.rel: src/c/shift_right.peep src/c/stock_registers.h

$(addprefix build/,$(addsuffix .rel,$(C_SIGNED_BOUNDS))): C_OPTIONS += \
	--no-peep --peep-file src/c/signed_bounds.peep
$(addprefix build/,$(addsuffix .rel,$(C_SIGNED_BOUNDS))): src/c/signed_bounds.peep src/c/stock_registers.h

build/a429.rel: C_OPTIONS += --peep-file src/c/accumulator.peep
build/a429.rel: src/c/accumulator.peep

build/a575.rel: C_OPTIONS += --no-peep --peep-file src/c/curve_q4.peep
build/a575.rel: src/c/curve_q4.peep src/c/stock_registers.h

build/a62f.rel: C_OPTIONS += -DBILINEAR_XDATA
build/a59d.rel build/a62f.rel: build/%.rel: src/c/bilinear_interp.c src/c/bilinear.peep src/c/stock_registers.h Makefile
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_OPTIONS) --no-peep --peep-file src/c/bilinear.peep --codeseg code_$* -c "$<" -o "$@"

$(addprefix build/,$(addsuffix .rel,$(C_INTERPOLATION))): C_OPTIONS += \
	--no-peep --peep-file src/c/curve_interp.peep
$(addprefix build/,$(addsuffix .rel,$(C_INTERPOLATION))): src/c/curve_interp.peep src/c/stock_registers.h

build/2bcd.rel: C_OPTIONS += --no-peep --peep-file src/c/rpm_axis.peep
build/2bcd.rel: src/c/rpm_axis.peep

build/2c3f.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_noise_zone.peep
build/2c3f.rel: src/c/knock_noise_zone.peep

build/3396.rel: C_OPTIONS += --no-peep --peep-file src/c/coolant_axis.peep
build/3396.rel: src/c/coolant_axis.peep

$(addprefix build/,$(addsuffix .rel,$(C_IGNITION))): C_OPTIONS += \
	--no-peep --peep-file src/c/ignition.peep
$(addprefix build/,$(addsuffix .rel,$(C_IGNITION))): src/c/ignition.peep
build/3f74.rel build/3f95.rel build/3fbb.rel build/400d.rel build/4081.rel: src/c/stock_registers.h

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
