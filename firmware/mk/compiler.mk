# C509-L compile options and dependencies for the stock instruction layout.
build/0000.rel: VECTOR_ENTRY := reset_vector
build/0000.rel: VECTOR_TARGET := restart_firmware
build/0003.rel: VECTOR_ENTRY := irq_vector_0003
build/0003.rel: VECTOR_TARGET := crank_interrupt_entry
build/001b.rel: VECTOR_ENTRY := irq_vector_001b
build/001b.rel: VECTOR_TARGET := timer1_interrupt_entry
build/0023.rel: VECTOR_ENTRY := irq_vector_0023
build/0023.rel: VECTOR_TARGET := uart0_interrupt
build/006b.rel: VECTOR_ENTRY := irq_vector_006b
build/006b.rel: VECTOR_TARGET := crank_capture_interrupt

$(addprefix build/,$(addsuffix .rel,$(C_ENTRY_VECTORS))): build/%.rel: src/target/c509/entry_vector.c $(BUILD_RULES)
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_INCLUDES) --codeseg code_$* -DSTOCK_VECTOR_ENTRY=$(VECTOR_ENTRY) \
		-DSTOCK_VECTOR_TARGET=$(VECTOR_TARGET) -c "$<" -o "$@"

$(addprefix build/,$(addsuffix .rel,$(C_VECTORS))): build/%.rel: src/target/c509/unused_vector.c $(BUILD_RULES)
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_INCLUDES) --codeseg code_$* -DC509_UNUSED_ISR=irq_unused_$* -c "$<" -o "$@"

build/f286.rel: C_OPTIONS += --peep-file src/monitor/serial/monitor_read.peep
build/f286.rel: src/monitor/serial/monitor_read.peep src/include/stock_registers.h

build/fffa.rel build/fffe.rel: src/include/flash_checksum.h

build/a352.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/multiply.peep
build/a352.rel: src/target/c509/multiply.peep src/include/stock_registers.h

build/a314.rel: C_OPTIONS += --no-peep --peep-file src/math/byte_bounds.peep
build/a314.rel: src/math/byte_bounds.peep src/include/stock_registers.h

build/a22e.rel build/596e.rel: C_OPTIONS += --no-peep --peep-file src/engine/clear_xdata_page_r1.peep
build/a22e.rel build/596e.rel: src/engine/clear_xdata_page_r1.peep

build/abca.rel: C_OPTIONS += --no-peep --peep-file src/engine/outputs/engine_outputs.peep
build/abca.rel: src/engine/outputs/engine_outputs.peep

build/2041.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/fuel_history.peep
build/2041.rel: src/engine/fuel/fuel_history.peep

build/5736.rel: src/engine/knock/knock_history.c src/engine/knock/knock_history_alternate.peep src/include/stock_registers.h $(BUILD_RULES)
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_INCLUDES) --no-peep --peep-file src/engine/knock/knock_history_alternate.peep --codeseg code_5736 --constseg const_5736 -DKNOCK_CYLINDER_ONLY -c "$<" -o "$@"

build/0bbd.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_score_history.peep
build/0bbd.rel: src/engine/misfire/misfire_score_history.peep src/include/stock_registers.h

build/0c47.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/crank_roughness.peep
build/0c47.rel: src/engine/misfire/crank_roughness.peep src/include/stock_registers.h

build/0daa.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_fuel_cut.peep
build/0daa.rel: src/engine/misfire/misfire_fuel_cut.peep src/include/stock_registers.h

build/0ea8.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_period_selector.peep
build/0ea8.rel: src/engine/misfire/misfire_period_selector.peep src/include/stock_registers.h

build/0fa5.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_period_bin.peep
build/0fa5.rel: src/engine/misfire/misfire_period_bin.peep src/include/stock_registers.h

build/109b.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_period_deviation.peep
build/109b.rel: src/engine/misfire/misfire_period_deviation.peep src/include/stock_registers.h

build/1272.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_period_reset.peep
build/1272.rel: src/engine/misfire/misfire_period_reset.peep src/include/stock_registers.h

build/1347.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_period_countdown.peep
build/1347.rel: src/engine/misfire/misfire_period_countdown.peep src/include/stock_registers.h

build/13e0.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_window.peep
build/13e0.rel: src/engine/misfire/misfire_window.peep src/include/stock_registers.h

build/152a.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/crank_airflow_input.peep
build/152a.rel: src/engine/crank/crank_airflow_input.peep src/include/stock_registers.h

build/1638.rel: C_OPTIONS += --no-peep --peep-file src/engine/air_charge/crank_charge_base.peep
build/1638.rel: src/engine/air_charge/crank_charge_base.peep src/include/stock_registers.h

build/1702.rel: C_OPTIONS += --no-peep --peep-file src/engine/air_charge/crank_charge_filter.peep
build/1702.rel: src/engine/air_charge/crank_charge_filter.peep src/include/stock_registers.h

build/17e7.rel: C_OPTIONS += --no-peep --peep-file src/engine/air_charge/throttle_charge_deltas.peep
build/17e7.rel: src/engine/air_charge/throttle_charge_deltas.peep src/include/stock_registers.h

build/19b4.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/cranking_fuel.peep
build/19b4.rel: src/engine/fuel/cranking_fuel.peep src/include/stock_registers.h

build/1b56.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/running_fuel.peep
build/1b56.rel: src/engine/fuel/running_fuel.peep src/include/stock_registers.h

build/1d15.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/injection_dose.peep
build/1d15.rel: src/engine/fuel/injection_dose.peep src/include/stock_registers.h

build/1db8.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/injector_schedule.peep
build/1db8.rel: src/engine/fuel/injector_schedule.peep src/include/injector_timer.h src/include/stock_registers.h

build/1e88.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/async_enrichment.peep
build/1e88.rel: src/engine/fuel/async_enrichment.peep src/include/injector_timer.h src/include/stock_registers.h

build/1fe6.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/crank_interrupt_exit.peep
build/1fe6.rel: src/engine/crank/crank_interrupt_exit.peep src/include/stock_registers.h

build/204d.rel: C_OPTIONS += --no-peep --peep-file src/startup/startup_reset.peep
build/204d.rel: src/startup/startup_reset.peep src/include/stock_registers.h

build/2102.rel build/218d.rel: C_OPTIONS += --no-peep --peep-file src/startup/runtime_init.peep
build/2102.rel build/218d.rel: src/startup/runtime_init.peep src/include/stock_registers.h

build/b08e.rel build/b17c.rel: C_OPTIONS += --no-peep --peep-file src/storage/eeprom_bus.peep
build/b08e.rel build/b17c.rel: src/storage/eeprom_bus.peep src/include/stock_registers.h

build/b24c.rel: C_OPTIONS += --no-peep --peep-file src/service/serial_startup.peep
build/b24c.rel: src/service/serial_startup.peep src/include/stock_registers.h

build/b2ed.rel: C_OPTIONS += --no-peep --peep-file src/service/serial_buffers.peep
build/b2ed.rel: src/service/serial_buffers.peep src/include/stock_registers.h

build/b3d9.rel: C_OPTIONS += --no-peep --peep-file src/storage/eeprom_records.peep
build/b3d9.rel: src/storage/eeprom_records.peep src/include/stock_registers.h

build/b977.rel: C_OPTIONS += --no-peep --peep-file src/storage/shutdown_eeprom.peep
build/b977.rel: src/storage/shutdown_eeprom.peep src/include/stock_registers.h

build/bb42.rel: C_OPTIONS += --no-peep --peep-file src/storage/startup_history.peep
build/bb42.rel: src/storage/startup_history.peep src/include/eeprom_triplet.h src/include/stock_registers.h

build/be7f.rel: C_OPTIONS += --no-peep --peep-file src/storage/history_recovery.peep
build/be7f.rel: src/storage/history_recovery.peep src/include/stock_registers.h

build/bd97.rel build/c33d.rel: C_OPTIONS += --no-peep --peep-file src/storage/misfire_persistence.peep
build/bd97.rel build/c33d.rel: src/storage/misfire_persistence.peep src/include/stock_registers.h

build/c3f1.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/eeprom_immobilizer_store.peep
build/c3f1.rel: src/service/immobilizer/eeprom_immobilizer_store.peep src/include/eeprom_triplet.h src/include/stock_registers.h

build/b73f.rel: C_OPTIONS += --no-peep --peep-file src/service/state/extra_state_record.peep
build/b73f.rel: src/service/state/extra_state_record.peep src/include/stock_registers.h

build/b7b9.rel: C_OPTIONS += --no-peep --peep-file src/service/state/extra_state_scratch.peep
build/b7b9.rel: src/service/state/extra_state_scratch.peep src/include/stock_registers.h

build/b81c.rel: C_OPTIONS += --no-peep --peep-file src/service/state/extra_state_lookup.peep
build/b81c.rel: src/service/state/extra_state_lookup.peep src/include/stock_registers.h

build/b943.rel: C_OPTIONS += --no-peep --peep-file src/service/state/extra_state_history.peep
build/b943.rel: src/service/state/extra_state_history.peep src/include/stock_registers.h

build/d7d5.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/service_read.peep
build/d7d5.rel: src/diagnostics/protocol/service_read.peep src/include/stock_registers.h

build/c11c.rel: C_OPTIONS += --no-peep --peep-file src/service/service_probe_reply.peep
build/c11c.rel: src/service/service_probe_reply.peep src/include/stock_registers.h

build/c1ad.rel: C_OPTIONS += --no-peep --peep-file src/service/service_e0.peep
build/c1ad.rel: src/service/service_e0.peep src/include/eeprom_triplet.h src/include/stock_registers.h

build/c232.rel: C_OPTIONS += --no-peep --peep-file src/service/service_ea.peep
build/c232.rel: src/service/service_ea.peep src/include/stock_registers.h

build/c2c0.rel: C_OPTIONS += --no-peep --peep-file src/service/service_record_compare.peep
build/c2c0.rel: src/service/service_record_compare.peep src/include/stock_registers.h

build/c470.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/eeprom_immobilizer_restore.peep
build/c470.rel: src/service/immobilizer/eeprom_immobilizer_restore.peep src/include/eeprom_triplet.h src/include/stock_registers.h

build/c550.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/eeprom_record_buffer.peep
build/c550.rel: src/service/immobilizer/eeprom_record_buffer.peep src/include/eeprom_triplet.h src/include/stock_registers.h

build/c573.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/immobilizer_dispatch.peep
build/c573.rel: src/service/immobilizer/immobilizer_dispatch.peep src/include/stock_registers.h

build/c59a.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/immobilizer_pulses.peep
build/c59a.rel: src/service/immobilizer/immobilizer_pulses.peep src/include/stock_registers.h

build/c640.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/immobilizer_message.peep
build/c640.rel: src/service/immobilizer/immobilizer_message.peep src/include/stock_registers.h

build/c696.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/immobilizer_compare.peep
build/c696.rel: src/service/immobilizer/immobilizer_compare.peep src/include/stock_registers.h

build/c6e5.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/immobilizer_init.peep
build/c6e5.rel: src/service/immobilizer/immobilizer_init.peep src/include/stock_registers.h

build/c713.rel: C_OPTIONS += --no-peep --peep-file src/service/immobilizer/immobilizer_nibble.peep
build/c713.rel: src/service/immobilizer/immobilizer_nibble.peep src/include/stock_registers.h

build/c010.rel build/cf92.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/service_status.peep
build/c010.rel: src/target/c509/service_status.peep src/include/eeprom_triplet.h src/include/stock_registers.h
build/cf92.rel: src/target/c509/service_status.peep src/include/stock_registers.h

build/ae0a.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_score_sum.peep
build/ae0a.rel: src/engine/misfire/misfire_score_sum.peep src/include/stock_registers.h

build/0a99.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_period_score.peep
build/0a99.rel: src/engine/misfire/misfire_period_score.peep src/include/stock_registers.h

build/09c0.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_period_delta.peep
build/09c0.rel: src/engine/misfire/misfire_period_delta.peep src/include/stock_registers.h

build/0916.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_sample_finish.peep
build/0916.rel: src/engine/knock/knock_sample_finish.peep src/include/stock_registers.h

build/078c.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/crank_interrupt_entry.peep
build/078c.rel: src/engine/crank/crank_interrupt_entry.peep

build/07a4.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_sample_prep.peep
build/07a4.rel: src/engine/knock/knock_sample_prep.peep src/include/stock_registers.h

build/084a.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_sample.peep
build/084a.rel: src/engine/knock/knock_sample.peep src/include/stock_registers.h

build/581c.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_attenuation.peep
build/581c.rel: src/engine/knock/knock_attenuation.peep src/include/stock_registers.h

build/592e.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_filter_rounding.peep
build/592e.rel: src/engine/knock/knock_filter_rounding.peep src/include/stock_registers.h

build/58dd.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_noise.peep
build/58dd.rel: src/engine/knock/knock_noise.peep

build/5616.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_history.peep
build/5616.rel: src/engine/knock/knock_history.peep src/include/stock_registers.h

build/54e0.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/roughness_limit_23.peep
build/54e0.rel: src/engine/misfire/roughness_limit_23.peep src/include/stock_registers.h

build/543c.rel: C_OPTIONS += --no-peep --peep-file src/engine/outputs/roughness_limit_1.peep
build/543c.rel: src/engine/outputs/roughness_limit_1.peep src/include/stock_registers.h

build/53b1.rel build/a8b7.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/output_service.peep
build/53b1.rel build/a8b7.rel: src/target/c509/output_service.peep

build/e000.rel: C_OPTIONS += --no-peep --peep-file src/monitor/alternate_startup.peep
build/e000.rel: src/monitor/alternate_startup.peep src/include/stock_registers.h

build/e21e.rel: C_OPTIONS += --no-peep --peep-file src/monitor/monitor_events.peep
build/e21e.rel: src/monitor/monitor_events.peep src/include/stock_registers.h

build/e2a5.rel: C_OPTIONS += --no-peep --peep-file src/monitor/io/monitor_driver_inputs.peep
build/e2a5.rel: src/monitor/io/monitor_driver_inputs.peep src/include/stock_registers.h

build/e323.rel: C_OPTIONS += --no-peep --peep-file src/monitor/monitor_request_wait.peep
build/e323.rel: src/monitor/monitor_request_wait.peep src/include/stock_registers.h

build/eae0.rel build/ec0c.rel: C_OPTIONS += --no-peep --peep-file src/monitor/serial/monitor_serial_line.peep
build/eae0.rel build/ec0c.rel: src/monitor/serial/monitor_serial_line.peep src/include/stock_registers.h

build/ebc1.rel: C_OPTIONS += --no-peep --peep-file src/monitor/monitor_mdu_test.peep
build/ebc1.rel: src/monitor/monitor_mdu_test.peep src/include/stock_registers.h

build/eb0f.rel: C_OPTIONS += --no-peep --peep-file src/monitor/adc/monitor_adc_correction.peep
build/eb0f.rel: src/monitor/adc/monitor_adc_correction.peep src/include/stock_registers.h

build/ea58.rel: C_OPTIONS += --no-peep --peep-file src/monitor/eeprom/monitor_eeprom_commands.peep
build/ea58.rel: src/monitor/eeprom/monitor_eeprom_commands.peep src/include/stock_registers.h

build/e9de.rel: C_OPTIONS += --no-peep --peep-file src/monitor/eeprom/monitor_verified_write.peep
build/e9de.rel: src/monitor/eeprom/monitor_verified_write.peep src/include/stock_registers.h

build/e978.rel build/ed13.rel: C_OPTIONS += --no-peep --peep-file src/monitor/monitor_threshold.peep
build/e978.rel build/ed13.rel: src/monitor/monitor_threshold.peep src/include/stock_registers.h
build/e35e.rel build/e978.rel build/ed22.rel: src/include/monitor_port9_io.h

build/e35e.rel: C_OPTIONS += --no-peep --peep-file src/monitor/monitor_dispatch.peep
build/e35e.rel: src/monitor/monitor_dispatch.peep src/include/stock_registers.h

build/e470.rel: C_OPTIONS += --no-peep --peep-file src/monitor/io/monitor_output_mirror.peep
build/e470.rel: src/monitor/io/monitor_output_mirror.peep src/include/stock_registers.h

build/e665.rel: C_OPTIONS += --no-peep --peep-file src/monitor/io/monitor_mirror.peep
build/e665.rel: src/monitor/io/monitor_mirror.peep src/include/stock_registers.h

build/ed22.rel: C_OPTIONS += --no-peep --peep-file src/monitor/io/monitor_io.peep
build/ed22.rel: src/monitor/io/monitor_io.peep src/include/stock_registers.h

build/eda9.rel: C_OPTIONS += --no-peep --peep-file src/monitor/monitor_arithmetic.peep
build/eda9.rel: src/monitor/monitor_arithmetic.peep src/include/stock_registers.h

build/edc2.rel: src/math/multiply_word_by_byte.c src/target/c509/multiply.peep src/include/stock_registers.h $(BUILD_RULES)
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_INCLUDES) --no-peep --peep-file src/target/c509/multiply.peep --codeseg code_edc2 --constseg const_edc2 -DMONITOR_MULTIPLY_COPY -c "$<" -o "$@"

build/edf1.rel: src/math/register_pair_arithmetic.c src/target/c509/stock_registers.peep src/include/stock_registers.h $(BUILD_RULES)
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_INCLUDES) --std-sdcc23 --no-peep --peep-file src/target/c509/stock_registers.peep --codeseg code_edf1 --constseg const_edf1 -DMONITOR_ADD_COPY -c "$<" -o "$@"

build/edf8.rel: C_OPTIONS += --no-peep --peep-file src/monitor/eeprom/monitor_eeprom.peep
build/edf8.rel: src/monitor/eeprom/monitor_eeprom.peep src/include/stock_registers.h

build/eec6.rel: C_OPTIONS += --no-peep --peep-file src/monitor/eeprom/monitor_eeprom_block.peep
build/eec6.rel: src/monitor/eeprom/monitor_eeprom_block.peep src/include/stock_registers.h

build/ef83.rel build/f063.rel: C_OPTIONS += --no-peep --peep-file src/monitor/monitor_checksums.peep
build/ef83.rel build/f063.rel: src/monitor/monitor_checksums.peep src/include/stock_registers.h

build/efb0.rel: C_OPTIONS += --no-peep --peep-file src/monitor/serial/monitor_uart.peep
build/efb0.rel: src/monitor/serial/monitor_uart.peep src/include/stock_registers.h

build/f0db.rel: C_OPTIONS += --no-peep --peep-file src/monitor/serial/monitor_serial_dispatch.peep
build/f0db.rel: src/monitor/serial/monitor_serial_dispatch.peep src/include/stock_registers.h

build/f154.rel: C_OPTIONS += --no-peep --peep-file src/monitor/serial/monitor_serial_internal_block.peep
build/f154.rel: src/monitor/serial/monitor_serial_internal_block.peep src/include/stock_registers.h

build/f1a1.rel: C_OPTIONS += --no-peep --peep-file src/monitor/serial/monitor_serial_mapped_block.peep
build/f1a1.rel: src/monitor/serial/monitor_serial_mapped_block.peep src/include/stock_registers.h

build/f225.rel: C_OPTIONS += --no-peep --peep-file src/monitor/serial/monitor_serial_register.peep
build/f225.rel: src/monitor/serial/monitor_serial_register.peep src/include/stock_registers.h

build/f094.rel build/f278.rel: C_OPTIONS += --no-peep --peep-file src/monitor/serial/monitor_frame.peep
build/f094.rel build/f278.rel: src/monitor/serial/monitor_frame.peep src/include/stock_registers.h

build/e6b0.rel: C_OPTIONS += --no-peep --peep-file src/monitor/io/monitor_edge_count.peep
build/e6b0.rel: src/monitor/io/monitor_edge_count.peep src/include/stock_registers.h

build/e7c0.rel: C_OPTIONS += --no-peep --peep-file src/monitor/adc/monitor_window.peep
build/e7c0.rel: src/monitor/adc/monitor_window.peep src/include/stock_registers.h
build/e000.rel build/e7c0.rel: src/include/monitor_port9_config.h

build/e70c.rel: C_OPTIONS += --no-peep --peep-file src/monitor/adc/monitor_adc_capture.peep
build/e70c.rel: src/monitor/adc/monitor_adc_capture.peep src/include/stock_registers.h

build/e763.rel: C_OPTIONS += --no-peep --peep-file src/monitor/adc/monitor_adc.peep
build/e763.rel: src/monitor/adc/monitor_adc.peep src/include/stock_registers.h
build/e6ee.rel build/e763.rel build/e7bc.rel build/e7c0.rel build/eb0f.rel: src/include/monitor_adc.h

build/e41b.rel build/e453.rel: C_OPTIONS += --no-peep --peep-file src/monitor/service_monitor.peep
build/e41b.rel build/e453.rel: src/monitor/service_monitor.peep src/include/stock_registers.h

build/521b.rel: C_OPTIONS += --no-peep --peep-file src/service/rom_checksum.peep
build/521b.rel: src/service/rom_checksum.peep src/include/stock_registers.h

build/50fa.rel: C_OPTIONS += --no-peep --peep-file src/service/service_cycle.peep
build/50fa.rel: src/service/service_cycle.peep

build/b5ef.rel build/b645.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/fault_report.peep
build/b5ef.rel build/b645.rel: src/diagnostics/fault_report.peep
build/b5ef.rel: src/include/stock_registers.h

build/4fab.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/fault_history.peep
build/4fab.rel: src/diagnostics/fault_history.peep

build/507f.rel build/b5de.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/diagnostic_lamp.peep
build/507f.rel build/b5de.rel: src/target/c509/diagnostic_lamp.peep

build/4ef9.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/pending_faults.peep
build/4ef9.rel: src/diagnostics/pending_faults.peep src/include/stock_registers.h

build/4e6a.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/fuel_learning_sweep.peep
build/4e6a.rel: src/engine/fuel/fuel_learning_sweep.peep src/include/stock_registers.h

build/4dfb.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/vehicle_speed.peep
build/4dfb.rel: src/engine/sensors/vehicle_speed.peep src/include/stock_registers.h

build/4d49.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/reported_fuel_usage.peep
build/4d49.rel: src/engine/fuel/reported_fuel_usage.peep src/include/stock_registers.h

build/4cd2.rel: C_OPTIONS += --no-peep --peep-file src/engine/outputs/diagnostic_masks.peep
build/4cd2.rel: src/engine/outputs/diagnostic_masks.peep src/include/stock_registers.h

build/4c5b.rel: C_OPTIONS += --no-peep --peep-file src/engine/outputs/ac_idle_compensation.peep
build/4c5b.rel: src/engine/outputs/ac_idle_compensation.peep src/include/stock_registers.h

build/4b81.rel: C_OPTIONS += --no-peep --peep-file src/engine/outputs/ac_clutch_gate.peep
build/4b81.rel: src/engine/outputs/ac_clutch_gate.peep src/include/stock_registers.h

build/4b1a.rel: C_OPTIONS += --no-peep --peep-file src/engine/outputs/fan.peep
build/4b1a.rel: src/engine/outputs/fan.peep

build/a39e.rel: C_OPTIONS += --no-peep --peep-file src/math/shift_left.peep
build/a39e.rel: src/math/shift_left.peep src/include/stock_registers.h

build/a3d9.rel: C_OPTIONS += --no-peep --peep-file src/math/shift_right.peep
build/a3d9.rel: src/math/shift_right.peep src/include/stock_registers.h

$(addprefix build/,$(addsuffix .rel,$(C_SIGNED_BOUNDS))): C_OPTIONS += \
	--no-peep --peep-file src/math/signed_bounds.peep
$(addprefix build/,$(addsuffix .rel,$(C_SIGNED_BOUNDS))): src/math/signed_bounds.peep src/include/stock_registers.h

build/a429.rel: C_OPTIONS += --peep-file src/math/accumulator.peep
build/a429.rel: src/math/accumulator.peep

build/a43e.rel build/a4dc.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/iac_airflow.peep
build/a43e.rel build/a4dc.rel: src/engine/idle/iac_airflow.peep src/include/stock_registers.h

build/a575.rel: C_OPTIONS += --no-peep --peep-file src/math/curve_q4.peep
build/a575.rel: src/math/curve_q4.peep src/include/stock_registers.h

build/a62f.rel: C_OPTIONS += -DBILINEAR_XDATA
build/a59d.rel build/a62f.rel: build/%.rel: src/math/bilinear_interp.c src/math/bilinear.peep src/include/stock_registers.h $(BUILD_RULES)
	mkdir -p build
	$(SDCC) $(C_FLAGS) $(C_INCLUDES) $(C_OPTIONS) --no-peep --peep-file src/math/bilinear.peep --codeseg code_$* -c "$<" -o "$@"

$(addprefix build/,$(addsuffix .rel,$(C_INTERPOLATION))): C_OPTIONS += \
	--no-peep --peep-file src/math/curve_interp.peep
$(addprefix build/,$(addsuffix .rel,$(C_INTERPOLATION))): src/math/curve_interp.peep src/include/stock_registers.h

build/3c2b.rel: C_OPTIONS += --no-peep --peep-file src/startup/power_down.peep
build/3c2b.rel: src/startup/power_down.peep src/include/stock_registers.h

build/2a91.rel build/2b1b.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/crank_cycle.peep
build/2a91.rel build/2b1b.rel: src/engine/crank/crank_cycle.peep src/include/stock_registers.h

build/2a4a.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_phase.peep
build/2a4a.rel: src/engine/knock/knock_phase.peep src/include/stock_registers.h

build/29d0.rel: C_OPTIONS += --no-peep --peep-file src/startup/first_cycle.peep
build/29d0.rel: src/startup/first_cycle.peep src/include/stock_registers.h

build/28f7.rel: C_OPTIONS += --no-peep --peep-file build/startup_purge.peep
build/28f7.rel: build/startup_purge.peep src/include/stock_registers.h

build/startup_purge.peep: src/target/c509/retained_dptr.peep src/target/c509/startup_purge.peep
	mkdir -p build
	cat $^ > "$@"

build/28b5.rel: C_OPTIONS += --no-peep --peep-file src/startup/startup_sensor_init.peep
build/28b5.rel: src/startup/startup_sensor_init.peep src/include/stock_registers.h

build/2818.rel: C_OPTIONS += --no-peep --peep-file src/startup/startup_sensor_scale.peep
build/2818.rel: src/startup/startup_sensor_scale.peep src/include/stock_registers.h

build/9dac.rel build/ce94.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/retained_dptr.peep
build/9dac.rel build/ce94.rel: src/target/c509/retained_dptr.peep src/include/stock_registers.h

build/277e.rel build/27d3.rel: C_OPTIONS += --no-peep --peep-file src/startup/startup_restore.peep
build/277e.rel build/27d3.rel: src/startup/startup_restore.peep src/include/stock_registers.h

build/277e.rel: src/include/retained_ram.h

build/252b.rel build/26d2.rel: C_OPTIONS += --no-peep --peep-file src/startup/startup_config.peep
build/252b.rel build/26d2.rel: src/startup/startup_config.peep src/include/stock_registers.h

build/2474.rel build/acea.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_io.peep
build/2474.rel build/acea.rel: src/engine/knock/knock_io.peep src/include/stock_registers.h

build/240e.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/startup_attenuation.peep
build/240e.rel: src/engine/knock/startup_attenuation.peep src/include/stock_registers.h

build/2328.rel: C_OPTIONS += --no-peep --peep-file src/startup/startup_knock.peep
build/2328.rel: src/startup/startup_knock.peep src/include/stock_registers.h

build/518f.rel: C_OPTIONS += --no-peep --peep-file build/service_save.peep
build/518f.rel: build/service_save.peep

build/service_save.peep: src/startup/retained_ram.peep src/target/c509/service_save.peep
	mkdir -p build
	cat $^ > "$@"

build/2291.rel build/3bb1.rel build/ac2d.rel: C_OPTIONS += --no-peep --peep-file src/startup/retained_ram.peep
build/2291.rel build/3bb1.rel build/ac2d.rel: src/startup/retained_ram.peep

build/2291.rel build/3bb1.rel build/518f.rel: src/include/retained_ram.h src/include/stock_registers.h

build/4077.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/sensor_fault_dispatch.peep
build/4077.rel: src/engine/sensors/sensor_fault_dispatch.peep

build/409a.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/sensor_fault_targets.peep
build/409a.rel: src/engine/sensors/sensor_fault_targets.peep src/include/stock_registers.h

build/4119.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/oxygen_heater_control.peep
build/4119.rel: src/engine/oxygen/oxygen_heater_control.peep src/include/stock_registers.h

build/41aa.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/oxygen_heater_resistance.peep
build/41aa.rel: src/engine/oxygen/oxygen_heater_resistance.peep src/include/stock_registers.h

build/421a.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/oxygen_heater_pulse.peep
build/421a.rel: src/engine/oxygen/oxygen_heater_pulse.peep src/include/stock_registers.h

build/4a95.rel: C_OPTIONS += --no-peep --peep-file src/engine/ignition/ignition_slot_writer.peep
build/4a95.rel: src/engine/ignition/ignition_slot_writer.peep src/include/stock_registers.h

build/a69b.rel: C_OPTIONS += --no-peep --peep-file src/math/sensor_math.peep
build/a69b.rel: src/math/sensor_math.peep src/include/stock_registers.h

build/a6cd.rel: C_OPTIONS += --no-peep --peep-file src/math/descending_curve.peep
build/a6cd.rel: src/math/descending_curve.peep src/include/stock_registers.h

build/49cc.rel: C_OPTIONS += --no-peep --peep-file src/engine/ignition/ignition_output.peep
build/49cc.rel: src/engine/ignition/ignition_output.peep src/include/stock_registers.h

build/4912.rel: C_OPTIONS += --no-peep --peep-file src/engine/ignition/injection_phase_step.peep
build/4912.rel: src/engine/ignition/injection_phase_step.peep src/include/stock_registers.h

build/4866.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/injection_adaptation.peep
build/4866.rel: src/engine/fuel/injection_adaptation.peep src/include/stock_registers.h

build/47d4.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/fuel_mixture_publish.peep
build/47d4.rel: src/engine/fuel/fuel_mixture_publish.peep src/include/stock_registers.h

build/a96f.rel build/aa28.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/driver_diagnostics.peep
build/a96f.rel build/aa28.rel: src/diagnostics/driver_diagnostics.peep src/include/stock_registers.h

build/00e0.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/crank_capture.peep
build/00e0.rel: src/engine/crank/crank_capture.peep

build/0247.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/crank_tooth.peep
build/0247.rel: src/engine/crank/crank_tooth.peep

build/02c1.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/tooth_dispatch.peep
build/02c1.rel: src/engine/crank/tooth_dispatch.peep

build/0390.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/angular_outputs.peep
build/0390.rel: src/engine/crank/angular_outputs.peep

build/c274.rel build/c31b.rel: C_OPTIONS += --no-peep --peep-file src/service/service_probe_end.peep
build/c274.rel build/c31b.rel: src/service/service_probe_end.peep src/include/stock_registers.h

build/cd7f.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/diagnostic_record_dispatch.peep
build/cd7f.rel: src/diagnostics/protocol/diagnostic_record_dispatch.peep src/include/stock_registers.h

build/cddc.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/tester_present.peep
build/cddc.rel: src/diagnostics/protocol/tester_present.peep src/include/stock_registers.h

build/cc16.rel build/ceaf.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/identification.peep
build/cc16.rel build/ceaf.rel: src/diagnostics/protocol/identification.peep src/include/stock_registers.h

build/d36c.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/injector_coil_controls.peep
build/d36c.rel: src/diagnostics/protocol/injector_coil_controls.peep src/include/stock_registers.h

build/d52f.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/relay_controls.peep
build/d52f.rel: src/diagnostics/protocol/relay_controls.peep src/include/stock_registers.h

build/d697.rel build/d78c.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/idle_purge_controls.peep
build/d697.rel build/d78c.rel: src/diagnostics/protocol/idle_purge_controls.peep src/include/stock_registers.h

build/d650.rel build/d6ff.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/output_control_reply.peep
build/d650.rel build/d6ff.rel: src/diagnostics/protocol/output_control_reply.peep src/include/output_control_reply.h src/include/stock_registers.h

build/ccaf.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/output_control_dispatch.peep
build/ccaf.rel: src/diagnostics/protocol/output_control_dispatch.peep src/include/stock_registers.h

build/ce19.rel build/cf78.rel build/cfe8.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/service_timeouts.peep
build/ce19.rel build/cf78.rel build/cfe8.rel: src/diagnostics/protocol/service_timeouts.peep
build/ce19.rel build/cfe8.rel: src/include/negative_service_reply.h src/include/stock_registers.h

build/cb89.rel build/ce54.rel build/cfa3.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/fault_services.peep
build/cb89.rel build/ce54.rel build/cfa3.rel: src/diagnostics/protocol/fault_services.peep src/include/stock_registers.h

build/ca00.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/service_dispatch.peep
build/ca00.rel: src/diagnostics/protocol/service_dispatch.peep src/include/stock_registers.h

build/caad.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/diag_session.peep
build/caad.rel: src/diagnostics/protocol/diag_session.peep src/include/stock_registers.h

build/c930.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/frame_validation.peep
build/c930.rel: src/diagnostics/protocol/frame_validation.peep src/include/stock_registers.h

build/d27f.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/diagnostic_record_writers.peep
build/d27f.rel: src/diagnostics/protocol/diagnostic_record_writers.peep src/include/stock_registers.h

build/cc2d.rel build/d139.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/diag_parameters.peep
build/cc2d.rel build/d139.rel: src/diagnostics/protocol/diag_parameters.peep src/include/stock_registers.h

build/d022.rel: C_OPTIONS += --no-peep --peep-file src/diagnostics/protocol/diag_responses.peep
build/d022.rel: src/diagnostics/protocol/diag_responses.peep src/include/stock_registers.h

build/5a25.rel: C_OPTIONS += --no-peep --peep-file src/service/uart0.peep
build/5a25.rel: src/service/uart0.peep

build/04b4.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/timer1_entry.peep
build/04b4.rel: src/target/c509/timer1_entry.peep

build/060a.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/timer1_iac.peep
build/060a.rel: src/target/c509/timer1_iac.peep

build/06fe.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/multispark.peep
build/06fe.rel: src/engine/crank/multispark.peep

build/3db5.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/async_cranking.peep
build/3db5.rel: src/engine/fuel/async_cranking.peep src/include/stock_registers.h

build/3c8b.rel: C_OPTIONS += --no-peep --peep-file src/startup/cranking_dispatch.peep
build/3c8b.rel: src/startup/cranking_dispatch.peep src/include/stock_registers.h

build/3edb.rel build/ac3c.rel: C_OPTIONS += --no-peep --peep-file src/startup/state_reset.peep
build/3edb.rel build/ac3c.rel: src/startup/state_reset.peep src/include/stock_registers.h

build/9bca.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/oxygen_feedback.peep
build/9bca.rel: src/engine/oxygen/oxygen_feedback.peep src/include/stock_registers.h

build/a6ef.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/power_mixture.peep
build/a6ef.rel: src/engine/fuel/power_mixture.peep src/include/stock_registers.h

build/461f.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/oxygen_mixture.peep
build/461f.rel: src/engine/fuel/oxygen_mixture.peep src/include/stock_registers.h

build/47b1.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/catalyst_enrichment.peep
build/47b1.rel: src/engine/fuel/catalyst_enrichment.peep src/include/stock_registers.h

build/a7e3.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/mixture_rate_limit.peep
build/a7e3.rel: src/engine/fuel/mixture_rate_limit.peep src/include/stock_registers.h

build/a822.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/fuel_mixture_ramp.peep
build/a822.rel: src/engine/fuel/fuel_mixture_ramp.peep src/include/stock_registers.h

build/ad95.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/exhaust_temperature.peep
build/ad95.rel: src/engine/oxygen/exhaust_temperature.peep src/include/stock_registers.h

build/3a85.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/sensor_ticks.peep
build/3a85.rel: src/engine/sensors/sensor_ticks.peep

build/3aea.rel: C_OPTIONS += --no-peep --peep-file src/startup/engine_shutdown.peep
build/3aea.rel: src/startup/engine_shutdown.peep

build/324f.rel build/ac05.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/ignition_supply.peep
build/324f.rel build/ac05.rel: src/target/c509/ignition_supply.peep

build/324f.rel: src/include/stock_registers.h

build/9bb9.rel: C_OPTIONS += --no-peep --peep-file src/target/c509/adc.peep
build/9bb9.rel: src/target/c509/adc.peep

build/a23a.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/iac_shutdown.peep
build/a23a.rel: src/engine/idle/iac_shutdown.peep

build/2990.rel build/a2db.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/iac_startup.peep
build/2990.rel build/a2db.rel: src/engine/idle/iac_startup.peep src/include/stock_registers.h

build/2b42.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/crank_rpm_scale.peep
build/2b42.rel: src/engine/crank/crank_rpm_scale.peep src/include/stock_registers.h

build/2bcd.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/rpm_axis.peep
build/2bcd.rel: src/engine/crank/rpm_axis.peep

build/2be6.rel: C_OPTIONS += --no-peep --peep-file src/engine/crank/starter_relay.peep
build/2be6.rel: src/engine/crank/starter_relay.peep

build/2c3f.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/knock_noise_zone.peep
build/2c3f.rel: src/engine/knock/knock_noise_zone.peep

build/2c6c.rel: C_OPTIONS += --no-peep --peep-file src/engine/knock/misfire_thresholds.peep
build/2c6c.rel: src/engine/knock/misfire_thresholds.peep

build/2cff.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_state.peep
build/2cff.rel: src/engine/misfire/misfire_state.peep

build/2e0e.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_rpm.peep
build/2e0e.rel: src/engine/misfire/misfire_rpm.peep src/include/stock_registers.h

build/2e9e.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_conditions.peep
build/2e9e.rel: src/engine/misfire/misfire_conditions.peep src/include/stock_registers.h

build/2f70.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_secondary.peep
build/2f70.rel: src/engine/misfire/misfire_secondary.peep src/include/stock_registers.h

build/2ff0.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_event.peep
build/2ff0.rel: src/engine/misfire/misfire_event.peep src/include/stock_registers.h

build/30c3.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_event_flags.peep
build/30c3.rel: src/engine/misfire/misfire_event_flags.peep src/include/stock_registers.h

build/3187.rel: C_OPTIONS += --no-peep --peep-file src/engine/misfire/misfire_outputs.peep
build/3187.rel: src/engine/misfire/misfire_outputs.peep src/include/stock_registers.h

build/328f.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/sensor_batch.peep
build/328f.rel: src/engine/sensors/sensor_batch.peep src/include/stock_registers.h

build/32f2.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/coolant_samples.peep
build/32f2.rel: src/engine/sensors/coolant_samples.peep src/include/stock_registers.h

build/33c2.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/running_voltage.peep
build/33c2.rel: src/engine/sensors/running_voltage.peep src/include/stock_registers.h

build/34a3.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/throttle_samples.peep
build/34a3.rel: src/engine/sensors/throttle_samples.peep src/include/stock_registers.h

build/3561.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/maf_baseline.peep
build/3561.rel: src/engine/sensors/maf_baseline.peep src/include/stock_registers.h

build/5952.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/maf_filter.peep
build/5952.rel: src/engine/sensors/maf_filter.peep src/include/stock_registers.h

build/35c9.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/oxygen_signal.peep
build/35c9.rel: src/engine/oxygen/oxygen_signal.peep src/include/stock_registers.h

build/3630.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/oxygen_diagnostics.peep
build/3630.rel: src/engine/oxygen/oxygen_diagnostics.peep src/include/stock_registers.h

build/36de.rel: C_OPTIONS += --no-peep --peep-file src/engine/oxygen/oxygen_monitor.peep
build/36de.rel: src/engine/oxygen/oxygen_monitor.peep src/include/stock_registers.h

build/37aa.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/throttle_position.peep
build/37aa.rel: src/engine/sensors/throttle_position.peep src/include/stock_registers.h

build/381d.rel: C_OPTIONS += --no-peep --peep-file src/engine/air_charge/barometric_zone.peep
build/381d.rel: src/engine/air_charge/barometric_zone.peep src/include/stock_registers.h

build/38bd.rel: C_OPTIONS += --no-peep --peep-file src/engine/air_charge/barometric_weight.peep
build/38bd.rel: src/engine/air_charge/barometric_weight.peep src/include/stock_registers.h

build/3927.rel: C_OPTIONS += --no-peep --peep-file src/engine/air_charge/barometric_learning.peep
build/3927.rel: src/engine/air_charge/barometric_learning.peep src/include/stock_registers.h

build/39f2.rel: C_OPTIONS += --no-peep --peep-file src/engine/air_charge/barometric_publish.peep
build/39f2.rel: src/engine/air_charge/barometric_publish.peep src/include/stock_registers.h

build/3396.rel: C_OPTIONS += --no-peep --peep-file src/engine/sensors/coolant_axis.peep
build/3396.rel: src/engine/sensors/coolant_axis.peep

build/428d.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_speed.peep
build/428d.rel: src/engine/idle/idle_speed.peep src/include/stock_registers.h

build/433a.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_airflow.peep
build/433a.rel: src/engine/idle/idle_airflow.peep src/include/stock_registers.h

build/4383.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_dispatch.peep
build/4383.rel: src/engine/idle/idle_dispatch.peep

build/4427.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_load.peep
build/4427.rel: src/engine/idle/idle_load.peep src/include/stock_registers.h

build/4453.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_entry.peep
build/4453.rel: src/engine/idle/idle_entry.peep

build/9dc8.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_feedback.peep
build/9dc8.rel: src/engine/idle/idle_feedback.peep src/include/stock_registers.h

build/9e62.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_ignition_feedback.peep
build/9e62.rel: src/engine/idle/idle_ignition_feedback.peep src/include/stock_registers.h

build/9ed3.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_iac_delta.peep
build/9ed3.rel: src/engine/idle/idle_iac_delta.peep src/include/stock_registers.h

build/9f32.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/idle_iac_pi.peep
build/9f32.rel: src/engine/idle/idle_iac_pi.peep src/include/stock_registers.h

build/a020.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/iac_adaptation_samples.peep
build/a020.rel: src/engine/idle/iac_adaptation_samples.peep src/include/stock_registers.h

build/a0b3.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/iac_adaptation_eval.peep
build/a0b3.rel: src/engine/idle/iac_adaptation_eval.peep src/include/stock_registers.h

build/a153.rel build/a1de.rel: C_OPTIONS += --no-peep --peep-file src/engine/idle/iac_adaptation_apply.peep
build/a153.rel build/a1de.rel: src/engine/idle/iac_adaptation_apply.peep src/include/stock_registers.h

build/44db.rel build/454e.rel build/4598.rel: C_OPTIONS += --no-peep --peep-file src/engine/fuel/coast_cut.peep
build/44db.rel build/454e.rel build/4598.rel: src/engine/fuel/coast_cut.peep

$(addprefix build/,$(addsuffix .rel,$(C_IGNITION))): C_OPTIONS += \
	--no-peep --peep-file src/target/c509/ignition.peep
$(addprefix build/,$(addsuffix .rel,$(C_IGNITION))): src/target/c509/ignition.peep
build/3f74.rel build/3f95.rel build/3fbb.rel build/400d.rel build/4081.rel: src/include/stock_registers.h

$(addprefix build/,$(addsuffix .rel,$(C_REGISTER_HELPERS))): C_OPTIONS += \
	--std-sdcc23 --no-peep --peep-file src/target/c509/stock_registers.peep
$(addprefix build/,$(addsuffix .rel,$(C_REGISTER_HELPERS))): \
	src/include/stock_registers.h src/target/c509/stock_registers.peep
