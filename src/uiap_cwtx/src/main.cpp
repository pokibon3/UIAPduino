//==================================================================//
// UIAP 7MHz CW Transmitter & 700Hz Sin wave DDS generator
// Written by Kimio Ohe JA9OIR/JA1AOQ
//==================================================================
#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "uiap_hal.h"
#include <cstdio>
#include "si5351.h"

Si5351 si5351;

static const char kHeaderText[] = "7MHz CW Transmitter";
static const uint32_t kMinFreqHz = 7000000;
static const uint32_t kMaxFreqHz = 7030000;
static const uint32_t kStepNormalHz = 1000;
static const uint32_t kStepFineHz = 100;
static const uint32_t kDebounceMs = 30;
static const uint32_t kRepeatDelayMs = 1000;
static const uint32_t kRepeatRateMs = 150;

static uint32_t g_freq_hz = 7015000;
static bool g_step_fine = false;

static uint32_t millis_now()
{
	return SysTick->CNT / DELAY_MS_TIME;
}

static void format_freq(char* out, size_t out_len, uint32_t hz)
{
	const uint32_t mhz = hz / 1000000;
	const uint32_t khz = (hz / 1000) % 1000;
	const uint32_t hz_rem = hz % 1000;
	snprintf(out, out_len, "%lu.%03lu.%03lu",
	         (unsigned long)mhz, (unsigned long)khz, (unsigned long)hz_rem);
}

static void draw_header()
{
	const uint8_t font_w = 6;
	const uint8_t font_h = 8;
	const uint8_t header_scale = 2;
	const uint16_t header_h = 20;
	const uint8_t header_char_w = (uint8_t)(font_w * header_scale);
	const uint16_t header_x = (TFT_WIDTH - (uint16_t)(strlen(kHeaderText) * header_char_w)) / 2;
	const uint16_t header_y = (uint16_t)((header_h - (font_h * header_scale)) / 2);

	tft_fill_rect(0, 0, TFT_WIDTH, header_h, DARKBLUE);
	tft_set_color(YELLOW);
	tft_set_cursor(header_x, header_y);
	tft_print(kHeaderText, header_scale);
	tft_draw_line(0, header_h, TFT_WIDTH - 1, header_h, SKYBLUE);
}

static void draw_step_line()
{
	const uint8_t font_w = 6;
	const uint8_t font_h = 8;
	const uint8_t footer_scale = 1;
	const uint16_t footer_h = (uint16_t)(font_h * footer_scale + 4);
	const uint16_t footer_y = (uint16_t)(TFT_HEIGHT - footer_h);
	const char* step_text = g_step_fine ? "STEP:FINE" : "STEP:NORMAL";
	const uint16_t text_w = (uint16_t)(strlen(step_text) * font_w * footer_scale);
	const uint16_t text_x = (TFT_WIDTH - text_w) / 2;
	const uint16_t text_y = (uint16_t)(footer_y + (footer_h - (font_h * footer_scale)) / 2);

	tft_fill_rect(0, footer_y, TFT_WIDTH, footer_h, DARKGREEN);
	tft_set_color(WHITE);
	tft_set_cursor(text_x, text_y);
	tft_print(step_text, footer_scale);
}

static void draw_frequency_box()
{
	const uint8_t font_w = 6;
	const uint8_t font_h = 8;
	const uint16_t header_h = 20;
	const uint16_t footer_h = (uint16_t)(font_h + 4);
	const uint16_t margin = 4;
	const uint16_t pad_x = 4;
	const uint16_t pad_y = 8;
	char freq_text[16];

	format_freq(freq_text, sizeof(freq_text), g_freq_hz);

	const uint16_t avail_w = (uint16_t)(TFT_WIDTH - (margin * 2));
	const uint16_t avail_h = (uint16_t)(TFT_HEIGHT - header_h - footer_h - (margin * 2));
	uint8_t max_scale_w = (uint8_t)(avail_w / (uint16_t)(strlen(freq_text) * font_w));
	uint8_t max_scale_h = (uint8_t)((avail_h - (pad_y * 2)) / font_h);
	uint8_t freq_scale = (max_scale_w < max_scale_h) ? max_scale_w : max_scale_h;
	if (freq_scale < 1) {
		freq_scale = 1;
	}
	if (freq_scale > 5) {
		freq_scale = 5;
	}

	const uint16_t freq_w = (uint16_t)(strlen(freq_text) * font_w * freq_scale);
	const uint16_t freq_h = (uint16_t)(font_h * freq_scale);
	const uint16_t box_w = (uint16_t)(freq_w + (pad_x * 2));
	const uint16_t box_h = (uint16_t)(freq_h + (pad_y * 2));
	const uint16_t box_x = (TFT_WIDTH - box_w) / 2;
	const uint16_t box_y = (uint16_t)(header_h + margin + (avail_h - box_h) / 2);
	const uint16_t freq_x = (uint16_t)(box_x + pad_x);
	const uint16_t freq_y = (uint16_t)(box_y + pad_y);

	tft_fill_rect(box_x, box_y, box_w, box_h, NAVY);
	tft_draw_rect(box_x, box_y, box_w, box_h, CYAN);
	if (box_w > 6 && box_h > 6) {
		tft_draw_rect((uint16_t)(box_x + 2), (uint16_t)(box_y + 2),
		              (uint16_t)(box_w - 4), (uint16_t)(box_h - 4), DARKGREY);
	}
	tft_set_color(SKYBLUE);
	tft_set_cursor(freq_x, freq_y);
	tft_print(freq_text, freq_scale);
}

static void set_frequency(uint32_t new_freq_hz)
{
	if (new_freq_hz == g_freq_hz) {
		return;
	}
	g_freq_hz = new_freq_hz;
	si5351.set_freq((uint64_t)g_freq_hz * 100ULL, SI5351_CLK0);
	draw_frequency_box();
}

struct ButtonState {
	uint8_t stable_level;
	uint8_t last_read;
	uint32_t last_change_ms;
	uint32_t press_start_ms;
	uint32_t last_repeat_ms;
};

static void init_button(ButtonState* state, uint8_t level, uint32_t now_ms)
{
	state->stable_level = level;
	state->last_read = level;
	state->last_change_ms = now_ms;
	state->press_start_ms = now_ms;
	state->last_repeat_ms = now_ms;
}

static void update_button(ButtonState* state, uint8_t level, uint32_t now_ms,
                          bool* pressed, bool* released, bool* repeat)
{
	*pressed = false;
	*released = false;
	*repeat = false;

	if (level != state->last_read) {
		state->last_read = level;
		state->last_change_ms = now_ms;
	}

	if (level != state->stable_level &&
	    (uint32_t)(now_ms - state->last_change_ms) >= kDebounceMs) {
		state->stable_level = level;
		if (state->stable_level == 0) {
			state->press_start_ms = now_ms;
			state->last_repeat_ms = now_ms;
			*pressed = true;
		} else {
			*released = true;
		}
	}

	if (state->stable_level == 0 &&
	    (uint32_t)(now_ms - state->press_start_ms) >= kRepeatDelayMs &&
	    (uint32_t)(now_ms - state->last_repeat_ms) >= kRepeatRateMs) {
		state->last_repeat_ms = now_ms;
		*repeat = true;
	}
}

void disp_title()
{
	tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, BLACK);
	draw_header();
	draw_frequency_box();
	draw_step_line();
}

// main function
//

void setup()
{
	SystemInit();				// ch32v003 Setup
	GPIO_setup();				// gpio Setup;    
	tft_init();					// TFT Init

    // Start serial and initialize the Si5351
    bool i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if(!i2c_found){
        printf("Device not found on I2C bus!\n");
    }

    // Set CLK0 to output initial frequency (Hz * 100)
    si5351.set_freq((uint64_t)g_freq_hz * 100ULL, SI5351_CLK0);
	si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
    si5351.output_enable(SI5351_CLK0, false);
    // Query a status update and wait a bit to let the Si5351 populate the
    // status flags correctly.
    si5351.update_status();
	disp_title();
}

int main()
{
    setup();

    uint32_t now_ms = millis_now();
    ButtonState sw1_state;
    ButtonState sw2_state;
    ButtonState sw3_state;
    init_button(&sw1_state, GPIO_digitalRead(SW1_PIN), now_ms);
    init_button(&sw2_state, GPIO_digitalRead(SW2_PIN), now_ms);
    init_button(&sw3_state, GPIO_digitalRead(SW3_PIN), now_ms);
    while (1) {
		uint8_t key_now = GPIO_digitalRead(PIN_KEYIN);
		uint8_t sw1_now = GPIO_digitalRead(SW1_PIN);
		uint8_t sw2_now = GPIO_digitalRead(SW2_PIN);
		uint8_t sw3_now = GPIO_digitalRead(SW3_PIN);
		bool sw1_pressed;
		bool sw1_released;
		bool sw1_repeat;
		bool sw2_pressed;
		bool sw2_released;
		bool sw2_repeat;
		bool sw3_pressed;
		bool sw3_released;
		bool sw3_repeat;
//        printf("Key: %d\r\n", key_now);

		const bool key_pressed = (key_now == 0);
		if (key_pressed) {
			GPIO_digitalWrite(PIN_LED, high);
            si5351.output_enable(SI5351_CLK0, true);
        } else {
			GPIO_digitalWrite(PIN_LED, low);
            si5351.output_enable(SI5351_CLK0, false);
		}

		now_ms = millis_now();
		update_button(&sw1_state, sw1_now, now_ms, &sw1_pressed, &sw1_released, &sw1_repeat);
		update_button(&sw2_state, sw2_now, now_ms, &sw2_pressed, &sw2_released, &sw2_repeat);
		update_button(&sw3_state, sw3_now, now_ms, &sw3_pressed, &sw3_released, &sw3_repeat);

		if (sw1_pressed) {
			g_step_fine = !g_step_fine;
			draw_step_line();
		}
		if (sw2_pressed || sw2_repeat) {
			const uint32_t step_hz = g_step_fine ? kStepFineHz : kStepNormalHz;
			const uint32_t new_freq = (g_freq_hz > kMinFreqHz + step_hz - 1)
				? (g_freq_hz - step_hz)
				: kMinFreqHz;
			set_frequency(new_freq);
		}
		if (sw3_pressed || sw3_repeat) {
			const uint32_t step_hz = g_step_fine ? kStepFineHz : kStepNormalHz;
			const uint32_t new_freq = (g_freq_hz + step_hz <= kMaxFreqHz)
				? (g_freq_hz + step_hz)
				: kMaxFreqHz;
			set_frequency(new_freq);
		}
    }

    return 0;
}
