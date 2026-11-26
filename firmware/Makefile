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
C_FUNCTIONS := 078c 07a4 084a 0916 09c0 0a99 0bbd 0c47 0daa 0ea8 0fa5 109b 1272 1347 13e0 152a 1638 1702 17e7 19b4 1b56 1d15 1db8 1e88 1fe6 2041 204d 2102 218d 2291 2328 240e 2474 252b 26d2 277e 27d3 2818 28b5 28f7 2990 29d0 2a4a 2a91 2b1b 2b42 2bcd 2be6 2c3f 2c6c 2cff 2e0e 2e9e 2f70 2ff0 30c3 3187 324f 328f 32f2 3396 33c2 34a3 3561 35c9 3630 36de 37aa 381d 38bd 3927 39f2 3a85 3aea 3bb1 3c2b 3edb $(C_IGNITION) 4077 409a 4119 41aa 421a 428d 433a 4383 4427 4453 44db 454e 4598 461f 47b1 47d4 4866 4912 49cc 4a95 4b1a 4b81 4c5b 4cd2 4d49 4dfb 4e6a 4ef9 4fab 507f 50fa 518f 521b 53b1 543c 54e0 5616 5736 581c 58dd 592e 5952 596e 9bb9 9bca 9dac 9dc8 9e62 9ed3 9f32 a020 a0b3 a153 a1de a22e a23a a2db a314 $(C_REGISTER_HELPERS) a352 a39e a3d9 $(C_SIGNED_BOUNDS) a429 a43e a4dc a575 a59d $(C_INTERPOLATION) a62f a6cd a6ef a7e3 a822 a8b7 a962 a96f aa28 abca ac05 ac2d ac3c acea ad95 ae0a b08e b17c b24c b2ed b3d9 b5de b5ef b645 b977 bb42 bd97 be7f c010 c33d cd7c ce94 cf92 d2f1 e41b e453 f090 f275
C_VECTORS := 000b 0013 002b 0043 004b 0053 005b 0063 0083 0093 009b 00a3 00ab
C_DATA := 4a89 580c 5a23 5f50 5f55 5f56 5f57 5f58 5f59 5f5a 5f5b 5f5c 5f5d 5f5e 5f5f 5f60 5f61 5f62 5f64 5f65 5f66 5f67 5f6a 5f6b 5f6c 5f6d 5f6e 5f6f 5f70 5f71 5f72 5f73 5f74 5f75 5f76 5f77 5f7d 5f7e 5f7f 5f80 5f81 5f84 5f85 5f86 5f87 5f88 5f89 5f8a 5f8b 5f8c 5f8d 5f8e 5f8f 5f90 5f91 5f93 5f94 5f96 5f97 5f99 5f9a 5f9b 5f9c 5f9d 5fa0 5fa1 5fa2 5fa3 5fa4 5fa5 5fa6 5fa7 5fa8 5faa 5fac 5fad 5fae 5faf 5fb0 5fb1 5fb2 5fb3 5fb4 5fb5 5fb7 5fba 5fbb 5fbc 5fbd 5fbe 5fbf 5fc0 5fc1 5fc2 5fc6 5fc8 5fd2 5fd5 5fd8 5fda 5fdb 5fdc 5fe0 5fe1 5fe2 5fe3 5fe4 5fe5 5fe6 5fe7 5fe8 5fed 5ff0 5ff2 5ff5 5ff6 6051 6054 6057 6058 6059 605a 605c 605d 605e 605f 6060 6061 6062 6063 6064 6066 6067 6068 6069 606a 606b 606d 6072 6074 6075 6076 6077 6078 6079 607a 607b 607c 607d 607e 607f 6080 6081 6086 608a 608b 608d 608f 6091 6093 6098 609d 609e 60a0 60a4 60a6 60a7 60a8 60aa 60ad 60ae 60b1 60b5 60ba 60c0 60c7 60c9 60ce 60d5 60d7 60da 60db 60dc 60dd 60de 60df 60e0 60e1 60e3 60e4 60e5 60e6 60e9 60ea 60eb 60ed 60ef 60f0 60f1 60f2 60f3 60f4 60f5 60f8 6100 6104 613c 62ac 63ac 63d3 64d3 65d3 6613 6ac8 6cc8 6dc8 6eef 6f0f 6f2f 6f56 6fc4 6fe4 700b 710b 726d 746d 75ef 76ef 77ef 782f 7877 7977 7a77 7a87 7aa7 7ac7 7bc7 7cd7 7cf7 7df7 7e07 7f8e 8193 8293 8393 83ba 852f 862f 864f 866f 8696 8796 87bd 87e4 880b 8a32 8a59 8a99 8ac0 8c00 8e0a 8e2a 8f2a 8f4a 904a 914a 924a 926a 928a 92b1 93b1 93d8 93dd 945d 955d 965d 975d 977d 987d 98a4 98cb 98f2 9912 9932 9952 9972 9992 99b9 b67d b6fd b6ff fffe
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

build/5736.rel: src/c/5616.c src/c/knock_history_alternate.peep src/c/stock_registers.h Makefile
	mkdir -p build
	$(SDCC) $(C_FLAGS) --no-peep --peep-file src/c/knock_history_alternate.peep --codeseg code_5736 --constseg const_5736 -DKNOCK_CYLINDER_ONLY -c "$<" -o "$@"

build/0bbd.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_score_history.peep
build/0bbd.rel: src/c/misfire_score_history.peep src/c/stock_registers.h

build/0c47.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_roughness.peep
build/0c47.rel: src/c/crank_roughness.peep src/c/stock_registers.h

build/0daa.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_fuel_cut.peep
build/0daa.rel: src/c/misfire_fuel_cut.peep src/c/stock_registers.h

build/0ea8.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_period_selector.peep
build/0ea8.rel: src/c/misfire_period_selector.peep src/c/stock_registers.h

build/0fa5.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_period_bin.peep
build/0fa5.rel: src/c/misfire_period_bin.peep src/c/stock_registers.h

build/109b.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_period_deviation.peep
build/109b.rel: src/c/misfire_period_deviation.peep src/c/stock_registers.h

build/1272.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_period_reset.peep
build/1272.rel: src/c/misfire_period_reset.peep src/c/stock_registers.h

build/1347.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_period_countdown.peep
build/1347.rel: src/c/misfire_period_countdown.peep src/c/stock_registers.h

build/13e0.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_window.peep
build/13e0.rel: src/c/misfire_window.peep src/c/stock_registers.h

build/152a.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_airflow_input.peep
build/152a.rel: src/c/crank_airflow_input.peep src/c/stock_registers.h

build/1638.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_charge_base.peep
build/1638.rel: src/c/crank_charge_base.peep src/c/stock_registers.h

build/1702.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_charge_filter.peep
build/1702.rel: src/c/crank_charge_filter.peep src/c/stock_registers.h

build/17e7.rel: C_OPTIONS += --no-peep --peep-file src/c/throttle_charge_deltas.peep
build/17e7.rel: src/c/throttle_charge_deltas.peep src/c/stock_registers.h

build/19b4.rel: C_OPTIONS += --no-peep --peep-file src/c/cranking_fuel.peep
build/19b4.rel: src/c/cranking_fuel.peep src/c/stock_registers.h

build/1b56.rel: C_OPTIONS += --no-peep --peep-file src/c/running_fuel.peep
build/1b56.rel: src/c/running_fuel.peep src/c/stock_registers.h

build/1d15.rel: C_OPTIONS += --no-peep --peep-file src/c/injection_dose.peep
build/1d15.rel: src/c/injection_dose.peep src/c/stock_registers.h

build/1db8.rel: C_OPTIONS += --no-peep --peep-file src/c/injector_schedule.peep
build/1db8.rel: src/c/injector_schedule.peep src/c/injector_timer.h src/c/stock_registers.h

build/1e88.rel: C_OPTIONS += --no-peep --peep-file src/c/async_enrichment.peep
build/1e88.rel: src/c/async_enrichment.peep src/c/injector_timer.h src/c/stock_registers.h

build/1fe6.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_interrupt_exit.peep
build/1fe6.rel: src/c/crank_interrupt_exit.peep src/c/stock_registers.h

build/204d.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_reset.peep
build/204d.rel: src/c/startup_reset.peep src/c/stock_registers.h

build/2102.rel build/218d.rel: C_OPTIONS += --no-peep --peep-file src/c/runtime_init.peep
build/2102.rel build/218d.rel: src/c/runtime_init.peep src/c/stock_registers.h

build/b08e.rel build/b17c.rel: C_OPTIONS += --no-peep --peep-file src/c/eeprom_bus.peep
build/b08e.rel build/b17c.rel: src/c/eeprom_bus.peep src/c/stock_registers.h

build/b24c.rel: C_OPTIONS += --no-peep --peep-file src/c/serial_startup.peep
build/b24c.rel: src/c/serial_startup.peep src/c/stock_registers.h

build/b2ed.rel: C_OPTIONS += --no-peep --peep-file src/c/serial_buffers.peep
build/b2ed.rel: src/c/serial_buffers.peep src/c/stock_registers.h

build/b3d9.rel: C_OPTIONS += --no-peep --peep-file src/c/eeprom_records.peep
build/b3d9.rel: src/c/eeprom_records.peep src/c/stock_registers.h

build/b977.rel: C_OPTIONS += --no-peep --peep-file src/c/shutdown_eeprom.peep
build/b977.rel: src/c/shutdown_eeprom.peep src/c/stock_registers.h

build/bb42.rel: C_OPTIONS += --no-peep --peep-file src/c/startup_history.peep
build/bb42.rel: src/c/startup_history.peep src/c/eeprom_triplet.h src/c/stock_registers.h

build/be7f.rel: C_OPTIONS += --no-peep --peep-file src/c/history_recovery.peep
build/be7f.rel: src/c/history_recovery.peep src/c/stock_registers.h

build/bd97.rel build/c33d.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_persistence.peep
build/bd97.rel build/c33d.rel: src/c/misfire_persistence.peep src/c/stock_registers.h

build/c010.rel build/cf92.rel: C_OPTIONS += --no-peep --peep-file src/c/service_status.peep
build/c010.rel: src/c/service_status.peep src/c/eeprom_triplet.h src/c/stock_registers.h
build/cf92.rel: src/c/service_status.peep src/c/stock_registers.h

build/ae0a.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_score_sum.peep
build/ae0a.rel: src/c/misfire_score_sum.peep src/c/stock_registers.h

build/0a99.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_period_score.peep
build/0a99.rel: src/c/misfire_period_score.peep src/c/stock_registers.h

build/09c0.rel: C_OPTIONS += --no-peep --peep-file src/c/misfire_period_delta.peep
build/09c0.rel: src/c/misfire_period_delta.peep src/c/stock_registers.h

build/0916.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_sample_finish.peep
build/0916.rel: src/c/knock_sample_finish.peep src/c/stock_registers.h

build/078c.rel: C_OPTIONS += --no-peep --peep-file src/c/crank_interrupt_entry.peep
build/078c.rel: src/c/crank_interrupt_entry.peep

build/07a4.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_sample_prep.peep
build/07a4.rel: src/c/knock_sample_prep.peep src/c/stock_registers.h

build/084a.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_sample.peep
build/084a.rel: src/c/knock_sample.peep src/c/stock_registers.h

build/581c.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_attenuation.peep
build/581c.rel: src/c/knock_attenuation.peep src/c/stock_registers.h

build/592e.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_filter_rounding.peep
build/592e.rel: src/c/knock_filter_rounding.peep src/c/stock_registers.h

build/58dd.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_noise.peep
build/58dd.rel: src/c/knock_noise.peep

build/5616.rel: C_OPTIONS += --no-peep --peep-file src/c/knock_history.peep
build/5616.rel: src/c/knock_history.peep src/c/stock_registers.h

build/54e0.rel: C_OPTIONS += --no-peep --peep-file src/c/roughness_limit_23.peep
build/54e0.rel: src/c/roughness_limit_23.peep src/c/stock_registers.h

build/543c.rel: C_OPTIONS += --no-peep --peep-file src/c/roughness_limit_1.peep
build/543c.rel: src/c/roughness_limit_1.peep src/c/stock_registers.h

build/53b1.rel build/a8b7.rel: C_OPTIONS += --no-peep --peep-file src/c/output_service.peep
build/53b1.rel build/a8b7.rel: src/c/output_service.peep

build/e41b.rel build/e453.rel: C_OPTIONS += --no-peep --peep-file src/c/service_monitor.peep
build/e41b.rel build/e453.rel: src/c/service_monitor.peep src/c/stock_registers.h

build/521b.rel: C_OPTIONS += --no-peep --peep-file src/c/rom_checksum.peep
build/521b.rel: src/c/rom_checksum.peep src/c/stock_registers.h

build/50fa.rel: C_OPTIONS += --no-peep --peep-file src/c/service_cycle.peep
build/50fa.rel: src/c/service_cycle.peep

build/b5ef.rel build/b645.rel: C_OPTIONS += --no-peep --peep-file src/c/fault_report.peep
build/b5ef.rel build/b645.rel: src/c/fault_report.peep
build/b5ef.rel: src/c/stock_registers.h

build/4fab.rel: C_OPTIONS += --no-peep --peep-file src/c/fault_history.peep
build/4fab.rel: src/c/fault_history.peep

build/507f.rel build/b5de.rel: C_OPTIONS += --no-peep --peep-file src/c/diagnostic_lamp.peep
build/507f.rel build/b5de.rel: src/c/diagnostic_lamp.peep

build/4ef9.rel: C_OPTIONS += --no-peep --peep-file src/c/pending_faults.peep
build/4ef9.rel: src/c/pending_faults.peep src/c/stock_registers.h

build/4e6a.rel: C_OPTIONS += --no-peep --peep-file src/c/fuel_learning_sweep.peep
build/4e6a.rel: src/c/fuel_learning_sweep.peep src/c/stock_registers.h

build/4dfb.rel: C_OPTIONS += --no-peep --peep-file src/c/vehicle_speed.peep
build/4dfb.rel: src/c/vehicle_speed.peep src/c/stock_registers.h

build/4d49.rel: C_OPTIONS += --no-peep --peep-file src/c/reported_fuel_usage.peep
build/4d49.rel: src/c/reported_fuel_usage.peep src/c/stock_registers.h

build/4cd2.rel: C_OPTIONS += --no-peep --peep-file src/c/diagnostic_masks.peep
build/4cd2.rel: src/c/diagnostic_masks.peep src/c/stock_registers.h

build/4c5b.rel: C_OPTIONS += --no-peep --peep-file src/c/ac_idle_compensation.peep
build/4c5b.rel: src/c/ac_idle_compensation.peep src/c/stock_registers.h

build/4b81.rel: C_OPTIONS += --no-peep --peep-file src/c/ac_clutch_gate.peep
build/4b81.rel: src/c/ac_clutch_gate.peep src/c/stock_registers.h

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

build/a96f.rel build/aa28.rel: C_OPTIONS += --no-peep --peep-file src/c/driver_diagnostics.peep
build/a96f.rel build/aa28.rel: src/c/driver_diagnostics.peep src/c/stock_registers.h

build/3edb.rel build/ac3c.rel: C_OPTIONS += --no-peep --peep-file src/c/state_reset.peep
build/3edb.rel build/ac3c.rel: src/c/state_reset.peep src/c/stock_registers.h

build/9bca.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_feedback.peep
build/9bca.rel: src/c/oxygen_feedback.peep src/c/stock_registers.h

build/a6ef.rel: C_OPTIONS += --no-peep --peep-file src/c/power_mixture.peep
build/a6ef.rel: src/c/power_mixture.peep src/c/stock_registers.h

build/461f.rel: C_OPTIONS += --no-peep --peep-file src/c/oxygen_mixture.peep
build/461f.rel: src/c/oxygen_mixture.peep src/c/stock_registers.h

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
