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
C_FUNCTIONS := 2041 2291 2328 240e 2474 252b 26d2 277e 27d3 2818 28b5 28f7 2990 29d0 2a4a 2a91 2b1b 2b42 2bcd 2be6 2c3f 2c6c 2cff 2e0e 2e9e 2f70 2ff0 30c3 3187 324f 328f 32f2 3396 33c2 34a3 3561 35c9 3630 36de 37aa 381d 38bd 3927 39f2 3a85 3aea 3bb1 3c2b $(C_IGNITION) 4077 409a 4119 41aa 421a 428d 433a 4383 4427 4453 44db 454e 4598 47b1 47d4 4866 4912 49cc 4a95 4b1a 518f 5952 596e 9bb9 9dac 9dc8 9e62 9ed3 9f32 a020 a0b3 a153 a1de a22e a23a a2db a314 $(C_REGISTER_HELPERS) a352 a39e a3d9 $(C_SIGNED_BOUNDS) a429 a43e a4dc a575 a59d $(C_INTERPOLATION) a62f a6cd a7e3 a822 a962 abca ac05 ac2d acea ad95 cd7c ce94 d2f1 f090 f275
C_VECTORS := 000b 0013 002b 0043 004b 0053 005b 0063 0083 0093 009b 00a3 00ab
C_DATA := 4a89 5f50 5f57 5f58 5f62 5f6d 5f6e 5f70 5f71 5f77 5f7d 5f7e 5f7f 5f80 5f81 5f84 5f85 5f86 5f87 5f88 5f89 5f8a 5f8b 5f8c 5f8d 5f8e 5f8f 5f90 5f91 5f93 5f94 5f96 5f97 5f99 5f9a 5f9b 5f9c 5f9d 5fa1 5fa2 5fa3 5fa4 5fa6 5fa7 5faa 5fad 5fae 5faf 5fb5 5fd2 5fd5 5fdb 5fdc 5fe0 5fe5 5fe7 5fe8 5fed 5ff2 6051 605d 605e 605f 6060 6061 6062 6067 6068 6069 606a 606b 606d 6076 6077 6078 6079 607a 607b 607c 607d 607e 607f 6080 6081 6086 608a 608b 608d 608f 6091 6093 6098 609d 609e 60a4 60aa 60ad 60ae 60b1 60b5 60ba 60c0 60c7 60c9 60ce 60d5 60d7 60de 60e1 60e3 60e5 60e6 60e9 60ea 60eb 60ed 60ef 60f5 60f8 6104 613c 62ac 63ac 64d3 65d3 6613 6ac8 6eef 6f56 6fc4 6fe4 700b 710b 75ef 76ef 782f 7877 7977 7ac7 7df7 7e07 7f8e 8193 83ba 852f 862f 864f 866f 8796 87e4 8a32 8a59 8a99 8ac0 8c00 8e0a 8e2a 8f2a 924a 926a 93d8 93dd 975d 977d 987d 98a4 98cb 98f2 99b9
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

build/a22e.rel build/596e.rel: C_OPTIONS += --no-peep --peep-file src/c/clear_xdata_page_r1.peep
build/a22e.rel build/596e.rel: src/c/clear_xdata_page_r1.peep

build/abca.rel: C_OPTIONS += --no-peep --peep-file src/c/engine_outputs.peep
build/abca.rel: src/c/engine_outputs.peep

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

build/3c2b.rel: C_OPTIONS += --no-peep --peep-file src/c/power_down.peep
build/3c2b.rel: src/c/power_down.peep src/c/stock_registers.h

build/2a91.rel build/2b1b.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_cycle.peep
build/2a91.rel build/2b1b.rel: src/c/crank_cycle.peep src/c/stock_registers.h

build/2a4a.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_phase.peep
build/2a4a.rel: src/c/knock_phase.peep src/c/stock_registers.h

build/29d0.rel: C_OPTIONS += --no-peep --peep-file src/c/first_cycle.peep
build/29d0.rel: src/c/first_cycle.peep src/c/stock_registers.h

build/28f7.rel: C_OPTIONS += --no-peep --peep-file build/startup_purge.peep
build/28f7.rel: build/startup_purge.peep src/c/stock_registers.h

build/startup_purge.peep: src/c/retained_dptr.peep src/c/startup_purge.peep
	mkdir -p build
	cat $^ > "$@"

build/28b5.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_sensor_init.peep
build/28b5.rel: src/c/startup_sensor_init.peep src/c/stock_registers.h

build/2818.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_sensor_scale.peep
build/2818.rel: src/c/startup_sensor_scale.peep src/c/stock_registers.h

build/9dac.rel build/ce94.rel: C_OPTIONS += --no-peep --peep-file src/c/retained_dptr.peep
build/9dac.rel build/ce94.rel: src/c/retained_dptr.peep src/c/stock_registers.h

build/277e.rel build/27d3.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_restore.peep
build/277e.rel build/27d3.rel: src/c/startup_restore.peep src/c/stock_registers.h

build/277e.rel: src/c/retained_ram.h

build/252b.rel build/26d2.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_config.peep
build/252b.rel build/26d2.rel: src/c/startup_config.peep src/c/stock_registers.h

build/2474.rel build/acea.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_io.peep
build/2474.rel build/acea.rel: src/c/knock_io.peep src/c/stock_registers.h

build/240e.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_attenuation.peep
build/240e.rel: src/c/startup_attenuation.peep src/c/stock_registers.h

build/2328.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_knock.peep
build/2328.rel: src/c/startup_knock.peep src/c/stock_registers.h

build/518f.rel: C_OPTIONS += --no-peep --peep-file build/service_save.peep
build/518f.rel: build/service_save.peep

build/service_save.peep: src/c/retained_ram.peep src/c/service_save.peep
	mkdir -p build
	cat $^ > "$@"

build/2291.rel build/3bb1.rel build/ac2d.rel: C_OPTIONS += --no-peep --peep-file src/c/retained_ram.peep
build/2291.rel build/3bb1.rel build/ac2d.rel: src/c/retained_ram.peep

build/2291.rel build/3bb1.rel build/518f.rel: src/c/retained_ram.h src/c/stock_registers.h

build/4077.rel: C_OPTIONS += --no-peep --peep-file src/c/sensor_fault_dispatch.peep
build/4077.rel: src/c/sensor_fault_dispatch.peep

build/409a.rel: C_OPTIONS += --no-peep --peep-file src/c/sensor_fault_targets.peep
build/409a.rel: src/c/sensor_fault_targets.peep src/c/stock_registers.h

build/4119.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_heater_control.peep
build/4119.rel: src/c/oxygen_heater_control.peep src/c/stock_registers.h

build/41aa.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_heater_resistance.peep
build/41aa.rel: src/c/oxygen_heater_resistance.peep src/c/stock_registers.h

build/421a.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_heater_pulse.peep
build/421a.rel: src/c/oxygen_heater_pulse.peep src/c/stock_registers.h

build/4a95.rel: C_OPTIONS += --no-peep --peep-file src/c/ignition_slot_writer.peep
build/4a95.rel: src/c/ignition_slot_writer.peep src/c/stock_registers.h

build/a6cd.rel: C_OPTIONS += --no-peep --peep-file src/c/descending_curve.peep
build/a6cd.rel: src/c/descending_curve.peep src/c/stock_registers.h

build/49cc.rel: C_OPTIONS += --no-peep --peep-file src/c/ignition_output.peep
build/49cc.rel: src/c/ignition_output.peep src/c/stock_registers.h

build/4912.rel: C_OPTIONS += --no-peep --peep-file src/c/injection_phase_step.peep
build/4912.rel: src/c/injection_phase_step.peep src/c/stock_registers.h

build/4866.rel: C_OPTIONS += --no-peep --peep-file src/c/injection_adaptation.peep
build/4866.rel: src/c/injection_adaptation.peep src/c/stock_registers.h

build/47d4.rel: C_OPTIONS += --no-peep --peep-file src/c/fuel_mixture_publish.peep
build/47d4.rel: src/c/fuel_mixture_publish.peep src/c/stock_registers.h

build/47b1.rel: C_OPTIONS += --no-peep --peep-file src/c/catalyst_enrichment.peep
build/47b1.rel: src/c/catalyst_enrichment.peep src/c/stock_registers.h

build/a7e3.rel: C_OPTIONS += --no-peep --peep-file src/c/mixture_rate_limit.peep
build/a7e3.rel: src/c/mixture_rate_limit.peep src/c/stock_registers.h

build/a822.rel: C_OPTIONS += --no-peep --peep-file src/c/fuel_mixture_ramp.peep
build/a822.rel: src/c/fuel_mixture_ramp.peep src/c/stock_registers.h

build/ad95.rel: C_OPTIONS += --no-peep --peep-file src/c/exhaust_temperature.peep
build/ad95.rel: src/c/exhaust_temperature.peep src/c/stock_registers.h

build/3a85.rel: C_OPTIONS += --no-peep --peep-file src/c/sensor_ticks.peep
build/3a85.rel: src/c/sensor_ticks.peep

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

build/2b42.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_rpm_scale.peep
build/2b42.rel: src/c/crank_rpm_scale.peep src/c/stock_registers.h

build/2bcd.rel: C_OPTIONS += --no-peep --peep-file src/c/rpm_axis.peep
build/2bcd.rel: src/c/rpm_axis.peep

build/2be6.rel: C_OPTIONS += --no-peep --peep-file src/c/starter_relay.peep
build/2be6.rel: src/c/starter_relay.peep

build/2c3f.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_noise_zone.peep
build/2c3f.rel: src/c/knock_noise_zone.peep

build/2c6c.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_thresholds.peep
build/2c6c.rel: src/c/misfire_thresholds.peep

build/2cff.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_state.peep
build/2cff.rel: src/c/misfire_state.peep

build/2e0e.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_rpm.peep
build/2e0e.rel: src/c/misfire_rpm.peep src/c/stock_registers.h

build/2e9e.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_conditions.peep
build/2e9e.rel: src/c/misfire_conditions.peep src/c/stock_registers.h

build/2f70.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_secondary.peep
build/2f70.rel: src/c/misfire_secondary.peep src/c/stock_registers.h

build/2ff0.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_event.peep
build/2ff0.rel: src/c/misfire_event.peep src/c/stock_registers.h

build/30c3.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_event_flags.peep
build/30c3.rel: src/c/misfire_event_flags.peep src/c/stock_registers.h

build/3187.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_outputs.peep
build/3187.rel: src/c/misfire_outputs.peep src/c/stock_registers.h

build/328f.rel: C_OPTIONS += --no-peep --peep-file src/c/sensor_batch.peep
build/328f.rel: src/c/sensor_batch.peep src/c/stock_registers.h

build/32f2.rel: C_OPTIONS += --no-peep --peep-file src/c/coolant_samples.peep
build/32f2.rel: src/c/coolant_samples.peep src/c/stock_registers.h

build/33c2.rel: C_OPTIONS += --no-peep --peep-file src/c/running_voltage.peep
build/33c2.rel: src/c/running_voltage.peep src/c/stock_registers.h

build/34a3.rel: C_OPTIONS += --no-peep --peep-file src/c/throttle_samples.peep
build/34a3.rel: src/c/throttle_samples.peep src/c/stock_registers.h

build/3561.rel: C_OPTIONS += --no-peep --peep-file src/c/maf_baseline.peep
build/3561.rel: src/c/maf_baseline.peep src/c/stock_registers.h

build/5952.rel: C_OPTIONS += --no-peep --peep-file src/c/maf_filter.peep
build/5952.rel: src/c/maf_filter.peep src/c/stock_registers.h

build/35c9.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_signal.peep
build/35c9.rel: src/c/oxygen_signal.peep src/c/stock_registers.h

build/3630.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_diagnostics.peep
build/3630.rel: src/c/oxygen_diagnostics.peep src/c/stock_registers.h

build/36de.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_monitor.peep
build/36de.rel: src/c/oxygen_monitor.peep src/c/stock_registers.h

build/37aa.rel: C_OPTIONS += --no-peep --peep-file src/c/throttle_position.peep
build/37aa.rel: src/c/throttle_position.peep src/c/stock_registers.h

build/381d.rel: C_OPTIONS += --no-peep --peep-file src/c/barometric_zone.peep
build/381d.rel: src/c/barometric_zone.peep src/c/stock_registers.h

build/38bd.rel: C_OPTIONS += --no-peep --peep-file src/c/barometric_weight.peep
build/38bd.rel: src/c/barometric_weight.peep src/c/stock_registers.h

build/3927.rel: C_OPTIONS += --no-peep --peep-file src/c/barometric_learning.peep
build/3927.rel: src/c/barometric_learning.peep src/c/stock_registers.h

build/39f2.rel: C_OPTIONS += --no-peep --peep-file src/c/barometric_publish.peep
build/39f2.rel: src/c/barometric_publish.peep src/c/stock_registers.h

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
