/*
 Name:		Ongeki_Xinput.ino
 Created:	2022/4/4 18:00:34
 Author:	lpp
*/
#include <FastLED.h>
#include <XInput.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#define BUTTON_BEGIN 2
#define BUTTON_SERVICE 3
#define BUTTON_L_R 4
#define BUTTON_L_G 5
#define BUTTON_L_B 6
#define BUTTON_R_R 7
#define BUTTON_R_G 8
#define BUTTON_R_B 9
#define FLICK A3
#define BUTTON_L_WAD 20
#define BUTTON_R_WAD 19
#define LED_PIN 10
#define LED_NUM 12
  
typedef struct configData
{
	int FLICK_LOW;
	int FLICK_HIGH;
	int mode;
}configData;
configData configdata;

void saveConfig()
{
	uint8_t* p = (uint8_t*)(&configdata);
	for (int i = 0; i < sizeof(configdata); i++)
	{
		EEPROM.write(i, *(p + i));
	}
}

void loadConfig()
{
	uint8_t* p = (uint8_t*)(&configdata);
	for (int i = 0; i < sizeof(configdata); i++)
	{
		*(p + i) = EEPROM.read(i);
	}
}

class LowPassFilte {
public:
	LowPassFilte(float Tf);
	~LowPassFilte() = default;
	float operator() (float x);
	float Tf;
protected:
	unsigned long timestamp_prev; 
	float y_prev; 
};

LowPassFilte::LowPassFilte(float time_constant)
	: Tf(time_constant)
	, y_prev(0.0f)
{
	timestamp_prev = micros();
}

const int ADC_Max = 1023;
CRGB LED[LED_NUM];
LowPassFilte LPF(0.015);
void(*resetFunc) (void) = 0;
void (*loopFunc)(void);


void ong_mode_loop()
{
	XInput.setJoystickX(JOY_LEFT, map((long)LPF(analogRead(FLICK)), configdata.FLICK_HIGH, configdata.FLICK_LOW, 25000, 45000));
	XInput.setDpad(!digitalRead(BUTTON_L_G), 0, !digitalRead(BUTTON_L_R), !digitalRead(BUTTON_L_B), false);
	XInput.setButton(BUTTON_B, !digitalRead(BUTTON_R_B));
	XInput.setButton(BUTTON_X, !digitalRead(BUTTON_R_R));
	XInput.setButton(BUTTON_Y, !digitalRead(BUTTON_R_G));
	XInput.setButton(BUTTON_LB, !digitalRead(BUTTON_L_WAD));
	XInput.setButton(BUTTON_RB, !digitalRead(BUTTON_R_WAD));
	XInput.setButton(BUTTON_BACK, !digitalRead(BUTTON_BEGIN));
	XInput.setButton(BUTTON_START, !digitalRead(BUTTON_SERVICE));
	XInput.send();
}

void diva_mode_loop()
{
	XInput.setJoystickX(JOY_LEFT, map((long)LPF(analogRead(FLICK)), configdata.FLICK_HIGH, configdata.FLICK_LOW, 0, 65535));
	XInput.setDpad(!digitalRead(BUTTON_L_G), !digitalRead(BUTTON_L_B), !digitalRead(BUTTON_R_R), !digitalRead(BUTTON_R_G), false);
	XInput.setButton(BUTTON_B, !digitalRead(BUTTON_R_B));
	XInput.setButton(BUTTON_A, !digitalRead(BUTTON_L_R));
	XInput.setButton(BUTTON_LB, !digitalRead(BUTTON_L_WAD));
	XInput.setButton(BUTTON_RB, !digitalRead(BUTTON_R_WAD));
	XInput.setButton(BUTTON_BACK, !digitalRead(BUTTON_BEGIN));
	XInput.setButton(BUTTON_START, !digitalRead(BUTTON_SERVICE));
	XInput.send();
}

float LowPassFilte::operator() (float x)
{
	unsigned long timestamp = micros();
	float dt = (timestamp - timestamp_prev) * 1e-6f;

	if (dt < 0.0f || dt > 0.5f)
		dt = 1e-3f;

	float alpha = Tf / (Tf + dt);
	float y = alpha * y_prev + (1.0f - alpha) * x;

	y_prev = y;
	timestamp_prev = timestamp;
	return y;
}

// the setup function runs once when you press reset or power the board
void setup() { 
	pinMode(BUTTON_L_R, INPUT_PULLUP);
	pinMode(BUTTON_L_G, INPUT_PULLUP);
	pinMode(BUTTON_L_B, INPUT_PULLUP);
	pinMode(BUTTON_R_R, INPUT_PULLUP);
	pinMode(BUTTON_R_G, INPUT_PULLUP);
	pinMode(BUTTON_R_B, INPUT_PULLUP);
	pinMode(BUTTON_L_WAD, INPUT_PULLUP);
	pinMode(BUTTON_R_WAD, INPUT_PULLUP);
	pinMode(BUTTON_BEGIN, INPUT_PULLUP);
	pinMode(BUTTON_SERVICE, INPUT_PULLUP);
	delay(500);
	FastLED.addLeds<WS2812B, LED_PIN, GRB>(LED, LED_NUM);

	loadConfig();

	if (!digitalRead(BUTTON_SERVICE))
	{
		LED[3] = CRGB::Red; FastLED.show();
		int high = 0, low = 999999;
		while (digitalRead(BUTTON_R_R))
		{
			if (analogRead(FLICK) > high) high = analogRead(FLICK);
			if (analogRead(FLICK) < low) low = analogRead(FLICK);
		}
		configdata.FLICK_HIGH = high;
		configdata.FLICK_LOW = low;
		saveConfig();
	}

	if (!digitalRead(BUTTON_BEGIN))
	{
		LED[6] = CRGB::Red; LED[7] = CRGB::Green; FastLED.show();
		while (true)
		{
			if (!digitalRead(BUTTON_L_R))
			{
				configdata.mode = 0;
				break;
			}
			if (!digitalRead(BUTTON_L_G))
			{
				configdata.mode = 1;
				break;
			}
		}
		saveConfig();
	}
	//boot amine

	switch (configdata.mode)
	{
	case 0:
	{
		LED[6] = CRGB::Red;   LED[5] = CRGB::Blue;
		FastLED.show();
		delay(150);
		LED[4] = LED[7] = CRGB::Green;
		FastLED.show();
		delay(150);
		LED[3] = CRGB::Red; LED[8] = CRGB::Blue;
		FastLED.show();
		delay(150);
		LED[0] = LED[1] = LED[2] = LED[9] = LED[11] = LED[10] = CRGB(255, 40, 255);
		FastLED.show();
		delay(150);
		loopFunc = ong_mode_loop;
		break;
	}
	case 1:
	{
		LED[8] = CRGB::Blue;
		LED[3] = CRGB::Pink;
		FastLED.show();
		delay(150);
		LED[7] = CRGB::Green;
		LED[4] = CRGB::Red;
		FastLED.show();
		delay(150);
		LED[0] = LED[1] = LED[2] = LED[5] = LED[6] = LED[9] = LED[11] = LED[10] = CRGB(255, 40, 255);
		FastLED.show();
		loopFunc = diva_mode_loop;
		break;
	}
	default:
		loopFunc = ong_mode_loop;
		break;
	}


	XInput.setJoystickRange(0, 65535);
	XInput.setAutoSend(false);
	XInput.begin();
}

// the loop function runs over and over again until power down or reset
void loop() {
	loopFunc();
	//if (!digitalRead(BUTTON_BEGIN) && !digitalRead(BUTTON_SERVICE) && !digitalRead(BUTTON_L_WAD) && !digitalRead(BUTTON_R_WAD)) resetFunc();
}

