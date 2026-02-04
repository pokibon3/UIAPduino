// morse message keyer for UIAPduino
// 2025.12.05 Kimiwo JA1AOQ
// for Arduino IDE 2.3.3
// Board Manager : UIAPduino 1.0.42

#include <Arduino.h>
#define LED_PIN D2            // for arduino IDE
#define KEY_OUT D3            // for arduino ide
#define SPEED   50            // 50 about 25 WPM

const	char	*table[] = {
	"A.-",
	"B-...",
	"C-.-.",
	"D-..",
	"E.",
	"F..-.",
	"G--.",
	"H....",
	"I..",
	"J.---",
	"K-.-",
	"L.-..",
	"M--",
	"N-.",
	"O---",
	"P.--.",
	"Q--.-",
	"R.-.",
	"S...",
	"T-",
	"U..-",
	"V...-",
	"W.--",
	"X-..-",
	"Y-.--",
	"Z--..",
	"1.----",
	"2..---",
	"3...--",
	"4....-",
	"5.....",
	"6-....",
	"7--...",
	"8---..",
	"9----.",
	"0-----",

	",--..--",
	"?..--..",
	"/-..-.",

	"a.-.-.",
	"b-...-",
	"k-.--.",
	"v...-.-",
/*
	"..-.-.-",
	",--..--",
	":---...",
	"?..--..",
	"'.----.",
	"--....-",
	"(-.--.",
	")-.--.-",
	"/-..-.",
	"=-...-",
	"+.-.-.",
	"\".-..-.",
	"*-..-",
	"@.--.-."
*/
};

const	char	*msgs[] = {
	"CQ CQ CQ DE JA1AOQ PSE K",
	"JA1AOQ DE JA9OIR K",
	"JA9OIR DE JA1AOQ GM TNX FER UR CALL b UR RST 599 ES QTH SAGAMIHARA CITY ES NAME KIMIWO HW? JA9OIR DE JA1AOQ k",
	"R JA1AOQ DE JA9OIR GM DR KIMIWO SAN TKS FB REPT b UR RST 599 ES QTH UOZU CITY ES NAME POKI HW? JA1AOQ DE JA9OIR k",
	"R DE JA1AOQ TNX FB 1ST QSO, HPE CU AGN DR POKI SAN 73 JA9OIR DE JA1AOQ TU v E E",
	"DE JA9OIR TNX FB QSO ES HVE A NICE DAY KIMI SAN 73 a JA1AOQ DE JA9OIR TU v E E",
};

void	short_beep()
{
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(KEY_OUT, LOW);
 	delay(SPEED);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(KEY_OUT, HIGH);
 	delay(SPEED);
}

void	long_beep()
{
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(KEY_OUT, LOW);
 	delay(SPEED * 3);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(KEY_OUT, HIGH);
 	delay(SPEED);
}

void	morse_chr(char chr)
{
	static	int16_t	cnt, i;

	if (chr == ' ') {
		delay(SPEED * 5);
		return;
	}

	for (cnt = 0; cnt < (int)(sizeof(table) / sizeof(table[0])); cnt++) {    // may be 43
		if (table[cnt][0] == chr) {
			for (i = 1; table[cnt][i] != 0x00; i++) {
				if (table[cnt][i] == '.')
					short_beep();
				else
					long_beep();
			}
			delay(SPEED * 3);
			return;
		}
	}
}

void	morse_str(const char* str)
{
	while (*str != 0x00) {
		morse_chr(*str);
		str++;
	}
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(KEY_OUT, OUTPUT_OD);      // for UIAPduino ARDUINO IDE
//    pinMode(KEY_OUT, OUTPUT);       
    morse_str("HI");
    delay(2000);
}

void loop()
{
	for (int i = 0; i < (int)(sizeof(msgs) / sizeof(msgs[0])); i++) {
		morse_str(msgs[i]);
		delay(1000);
	}
}

