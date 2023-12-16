   /*
 Name:		Ongeki_Xinput.ino
 Created:	2022/4/4 18:00:34
 Author:	lpp
*/
#include <FastLED.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <HID-Project.h>
#include "raw_hid.hpp"

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

const uint8_t PIN_MAP[10] = {
	// L: A B C SIDE MENU
	BUTTON_L_R,BUTTON_L_G,BUTTON_L_B,BUTTON_L_WAD ,BUTTON_BEGIN,
	// R: A B C SIDE MENU
	BUTTON_R_R,BUTTON_R_G,BUTTON_R_B,BUTTON_R_WAD ,BUTTON_SERVICE
};

const uint8_t LED_PIN_MAP[6] = {
	6,7,8,3,4,5,
};
  

typedef struct configData
{
	int FLICK_LOW;
	int FLICK_HIGH;
	int mode;
	raw_hid::aimi_id_t aimi_id;
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

LowPassFilte::LowPassFilte(float time_constant)
	: Tf(time_constant)
	, y_prev(0.0f)
{
	timestamp_prev = micros();
}

const int ADC_Max = 1023;
CRGB LED[LED_NUM];
LowPassFilte LPF(0.01);
void(*resetFunc) (void) = 0;
void (*loopFunc)(void);

void read_io(raw_hid::output_data_t* data)
{
	for (auto i = 0; i < 10; i++) {
		uint8_t temp = !digitalRead(PIN_MAP[i]);
		data->buttons[i] = temp;
		//Serial.print(temp);
	}
	//Serial.println();

	data->lever = (uint16_t)LPF(analogRead(FLICK));
	if (data->buttons[4])
	{
		if(data->buttons[0]) data->optButtons = 0b001;
		if (data->buttons[1]) data->optButtons = 0b010;
		if (data->buttons[2]) data->optButtons = 0b100;
	}
	else {
		memset(&data->aimi_id, 0, 10);
		data->optButtons = 0b000;
	}
}


void set_led(raw_hid::led_t &data) {
	FastLED.setBrightness(40);
	CRGB lightColors[6];
	for (int i = 0; i < 3; i++) {
		memcpy(&lightColors[i], &data.ledColors[i], 3);
		memcpy(&lightColors[i + 3], &data.ledColors[i + 5], 3);
	}

	for (int i = 0; i < 6; i++)
	{
		LED[LED_PIN_MAP[i]] = lightColors[i];
	}
	LED[0] = LED[1] = LED[2] = LED[9] = LED[10] = LED[11] = CRGB(255, 40, 255);
	
	FastLED.show();
}

long LEDRefreshTimer = 0;
void ong_mode_loop()
{
	//delay(50);
	read_io(raw_hid::getO());
	/*if (Serial.availableForWrite())
	{
		Serial.write(data->buffer);
	}*/

	set_led(raw_hid::getI()->led);
	LEDRefreshTimer = millis();
	raw_hid::update();
}

int LEDCount = 0;
long timer = 0;
void test_mode_loop()
{
	if (millis() - timer >= 500)
	{
		if (LEDCount == LED_NUM)
		{
			for (int i = 0; i < LED_NUM; i++)
			{
				LED[i]= CRGB(0, 0, 0);
			}
			LEDCount = 0;
		}
		else
		{
			LED[LEDCount] = CRGB(255, 255, 255);
			LEDCount++;
		}
		FastLED.show();
		timer = millis();
	}
}



// the setup function runs once when you press reset or power the board
void setup() { 
	//Serial.begin(115200);
	for (unsigned char i : PIN_MAP) {
		pinMode(i, INPUT_PULLUP);
	}
	delay(500);
	FastLED.addLeds<WS2812B, LED_PIN, GRB>(LED, LED_NUM);
	

	loadConfig();

	/*if (!digitalRead(BUTTON_SERVICE))
	{
		LED[3] = CRGB::Red; FastLED.show();
		int high = 0, low = 65535;
		while (digitalRead(BUTTON_R_R))
		{
			if (analogRead(FLICK) > high) high = analogRead(FLICK);
			if (analogRead(FLICK) < low) low = analogRead(FLICK);
		}
		configdata.FLICK_HIGH = high;
		configdata.FLICK_LOW = low;
		saveConfig();
	}*/

	//Serial.println("booting");

	if (!digitalRead(BUTTON_BEGIN))
	{
		LED[6] = CRGB::Red; LED[7] = CRGB::Green; LED[8] = CRGB::Blue; FastLED.show();
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
			if (!digitalRead(BUTTON_L_B)) 
			{
				configdata.mode = 2;
				break;
			}
		}
		//configdata.mode = 0;
		saveConfig();
	}
	//boot amine
	//configdata.mode = 0;
	switch (configdata.mode)
	{
		/*case 2:
		{
			loopFunc = test_mode_loop;
			break;
		}
		case 1:
		{
			loopFunc = test_mode_loop;
			break;
		}*/
		case 0:
		default:
		{
		LED[6] = CRGB::Red;   LED[5] = CRGB::Blue;
		FastLED.show();
		delay(500);
		LED[4] = LED[7] = CRGB::Green;
		FastLED.show();
		delay(500);
		LED[3] = CRGB::Red; LED[8] = CRGB::Blue;
		FastLED.show();
		delay(500);
		LED[0] = LED[1] = LED[2] = LED[9] = LED[11] = LED[10] = CRGB(255, 40, 255);
		FastLED.show();
		delay(500);
		loopFunc = ong_mode_loop;
		raw_hid::start();
		FastLED.clear();
		delay(500);
		//Serial.println("enter loop");
		break;
		}
		break;
	}
}

// the loop function runs over and over again until power down or reset
void loop() {
	loopFunc();
}
