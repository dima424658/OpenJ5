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
C_FUNCTIONS := 2041 2990 2bcd 2c3f 324f 3396 3aea $(C_IGNITION) 428d 433a 4383 4427 4453 44db 454e 4598 4b1a 9bb9 9dc8 9e62 9ed3 9f32 a020 a0b3 a153 a1de a22e a23a a2db a314 $(C_REGISTER_HELPERS) a352 a39e a3d9 $(C_SIGNED_BOUNDS) a429 a43e a4dc a575 a59d $(C_INTERPOLATION) a62f ac05 cd7c d2f1 f090 f275
C_VECTORS := 000b 0013 002b 0043 004b 0053 005b 0063 0083 0093 009b 00a3 00ab
C_DATA := 5f57 5f62 5f6d 5f77 5f8d 5fa3 5faf 5fe5 5fe7 607b 6081 6086 608a 608b 608d 608f 6091 6093 6098 60aa 60ad 60c7 60c9 60e5 60e9 60ea 60eb 60ed 60ef 60f5 60f8 613c 62ac 64d3 6613 6ac8 6f56 6fc4 6fe4 700b 710b 75ef 7877 7ac7 7e07 8193 83ba 852f 866f 8796 87e4 8ac0 8c00 98f2 99b9
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

build/2041.rel: C_OPTIONS += --no-peep --peep-file src/c/fuel_history.peep
build/2041.rel: src/c/fuel_history.peep

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

build/a43e.rel build/a4dc.rel: C_OPTIONS += --no-peep --peep-file src/c/iac_airflow.peep
build/a43e.rel build/a4dc.rel: src/c/iac_airflow.peep src/c/stock_registers.h

build/a575.rel: C_OPTIONS += --no-peep --peep-file src/c/curve_q4.peep
build/a575.rel: src/c/curve_q4.peep src/c/stock_registers.h

build/a62f.rel: C_OPTIONS += -DBILINEAR_XDATA
build/a59d.rel build/a62f.rel: build/%.rel: src/c/bilinear_interp.c src/c/bilinear.peep src/c/stock_registers.h Makefile
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_OPTIONS) --no-peep --peep-file src/c/bilinear.peep --codeseg code_$* -c "$<" -o "$@"

$(addprefix build/,$(addsuffix .rel,$(C_INTERPOLATION))): C_OPTIONS += \
	--no-peep --peep-file src/c/curve_interp.peep
$(addprefix build/,$(addsuffix .rel,$(C_INTERPOLATION))): src/c/curve_interp.peep src/c/stock_registers.h

build/3aea.rel: C_OPTIONS += --no-peep --peep-file src/c/engine_shutdown.peep
build/3aea.rel: src/c/engine_shutdown.peep

build/324f.rel build/ac05.rel: C_OPTIONS += --no-peep --peep-file src/c/ignition_supply.peep
build/324f.rel build/ac05.rel: src/c/ignition_supply.peep

build/324f.rel: src/c/stock_registers.h

build/9bb9.rel: C_OPTIONS += --no-peep --peep-file src/c/adc.peep
build/9bb9.rel: src/c/adc.peep

build/a23a.rel: C_OPTIONS += --no-peep --peep-file src/c/iac_shutdown.peep
build/a23a.rel: src/c/iac_shutdown.peep

build/2990.rel build/a2db.rel: C_OPTIONS += --no-peep --peep-file src/c/iac_startup.peep
build/2990.rel build/a2db.rel: src/c/iac_startup.peep src/c/stock_registers.h

build/2bcd.rel: C_OPTIONS += --no-peep --peep-file src/c/rpm_axis.peep
build/2bcd.rel: src/c/rpm_axis.peep

build/2c3f.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_noise_zone.peep
build/2c3f.rel: src/c/knock_noise_zone.peep

build/3396.rel: C_OPTIONS += --no-peep --peep-file src/c/coolant_axis.peep
build/3396.rel: src/c/coolant_axis.peep

build/428d.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_speed.peep
build/428d.rel: src/c/idle_speed.peep src/c/stock_registers.h

build/433a.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_airflow.peep
build/433a.rel: src/c/idle_airflow.peep src/c/stock_registers.h

build/4383.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_dispatch.peep
build/4383.rel: src/c/idle_dispatch.peep

build/4427.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_load.peep
build/4427.rel: src/c/idle_load.peep src/c/stock_registers.h

build/4453.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_entry.peep
build/4453.rel: src/c/idle_entry.peep

build/9dc8.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_feedback.peep
build/9dc8.rel: src/c/idle_feedback.peep src/c/stock_registers.h

build/9e62.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_ignition_feedback.peep
build/9e62.rel: src/c/idle_ignition_feedback.peep src/c/stock_registers.h

build/9ed3.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_iac_delta.peep
build/9ed3.rel: src/c/idle_iac_delta.peep src/c/stock_registers.h

build/9f32.rel: C_OPTIONS += --no-peep --peep-file src/c/idle_iac_pi.peep
build/9f32.rel: src/c/idle_iac_pi.peep src/c/stock_registers.h

build/a020.rel: C_OPTIONS += --no-peep --peep-file src/c/iac_adaptation_samples.peep
build/a020.rel: src/c/iac_adaptation_samples.peep src/c/stock_registers.h

build/a0b3.rel: C_OPTIONS += --no-peep --peep-file src/c/iac_adaptation_eval.peep
build/a0b3.rel: src/c/iac_adaptation_eval.peep src/c/stock_registers.h

build/a153.rel build/a1de.rel: C_OPTIONS += --no-peep --peep-file src/c/iac_adaptation_apply.peep
build/a153.rel build/a1de.rel: src/c/iac_adaptation_apply.peep src/c/stock_registers.h

build/44db.rel build/454e.rel build/4598.rel: C_OPTIONS += --no-peep --peep-file src/c/coast_cut.peep
build/44db.rel build/454e.rel build/4598.rel: src/c/coast_cut.peep

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
