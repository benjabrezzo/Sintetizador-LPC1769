# Tests de lógica del firmware en la PC (gcc + make, sin frameworks).
# Cada test_*.c es un programa que devuelve 0 si pasa y distinto de 0 si falla.

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Werror -I../inc
LDLIBS  := -lm
BUILD   := build
SOURCES := $(wildcard test_*.c)
TESTS   := $(SOURCES:%.c=$(BUILD)/%)

.PHONY: test clean

test: $(TESTS)
	@for t in $(TESTS); do echo "== $$t"; $$t || exit 1; done

# Cada test compila solo los .c de lógica que declara abajo (nunca los de hardware). Todos dependen de config.h:
# cualquier módulo lo puede incluir, y si cambia una constante el test se tiene que recompilar
$(BUILD)/%: %.c test_check.h ../inc/config.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

$(BUILD)/test_debounce: ../src/debounce.c ../inc/debounce.h
$(BUILD)/test_envelope: ../src/envelope.c ../inc/envelope.h ../inc/config.h
$(BUILD)/test_equalizer: ../src/equalizer.c ../inc/equalizer.h
$(BUILD)/test_voice: ../src/voice.c ../inc/voice.h ../inc/oscillator.h ../src/envelope.c ../inc/envelope.h ../src/oscillator.c
$(BUILD)/test_dac_level: ../src/dac_level.c ../inc/dac_level.h
$(BUILD)/test_block_cursor: ../src/block_cursor.c ../inc/block_cursor.h ../src/oscillator.c ../inc/oscillator.h
$(BUILD)/test_tables: ../src/tables.c ../inc/tables.h
$(BUILD)/test_spectrum: ../src/spectrum.c ../inc/spectrum.h ../src/tables.c ../inc/tables.h
$(BUILD)/test_wave_select: ../src/wave_select.c ../inc/wave_select.h ../inc/config.h
$(BUILD)/test_oscillator:../src/tables.c ../inc/tables.h ../src/oscillator.c ../inc/oscillator.h
$(BUILD)/test_pot_filter: ../src/pot_filter.c ../inc/pot_filter.h
$(BUILD)/test_ring_buffer: ../src/ring_buffer.c ../inc/ring_buffer.h
$(BUILD)/test_uart_protocol: ../src/uart_protocol.c ../inc/uart_protocol.h ../inc/tables.h
$(BUILD)/test_led_encoder: ../src/led_encoder.c ../inc/led_encoder.h
$(BUILD)/test_led_patterns: ../src/led_patterns.c ../inc/led_patterns.h ../src/led_encoder.c ../inc/led_encoder.h ../inc/wave_select.h
$(BUILD)/test_led_bars: ../src/led_bars.c ../inc/led_bars.h ../src/led_encoder.c ../inc/led_encoder.h ../src/spectrum.c ../inc/spectrum.h ../src/tables.c ../inc/tables.h
$(BUILD)/test_mixer:../src/mixer.c ../inc/mixer.h ../inc/voice.h ../inc/envelope.h ../src/envelope.c ../src/oscillator.c ../inc/oscillator.h ../src/tables.c ../inc/tables.h ../src/dac_level.c ../inc/dac_level.h

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
