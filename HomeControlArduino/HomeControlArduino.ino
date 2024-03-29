#include <SoftwareSerial\SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Timer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoThread\Thread.h>
#include <Commands.h>
#include <EEPROM\EEPROM.h>
#include <EEPROMAddressess.h>
#include <GPIO.h>
#include <Time\Time.h>

Timer timer;
tmElements_t tm;
String message = "";


int brightness;
float tempInside = -127;
float tempOutside = -127;
boolean alarmMovement= false;
boolean autoSwitchOn= false;
boolean smokeAlarm = false;
boolean monoxideAlarm = false;

boolean alarmCustomSettings = false;
byte alarmWeekDays=0;
byte alarmSinceMinute = 0;
byte alarmSinceHour = 0;
byte alarmToMinute = 0;
byte alarmToHour = 0;


boolean autoLightCustomSettings = false;
byte autoLightWeekDays = 0;
byte autoLighSinceMinute = 0;
byte autoLightSinceHour = 0;
byte autoLightToMinute = 0;
byte autoLightToHour = 0;

byte smokeLevel = 1;
byte monoxideLevel = 1;

boolean touchFlag = false;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

OneWire oneWire(ONE_WIRE_BUS);
SoftwareSerial softSerial(RX, TX);
DallasTemperature sensors(&oneWire);
Thread alarmThread = Thread();
int alarmDuration = 1000;
boolean isAlarmRunning = false;

void updateTemp() {
	lcd.setCursor(0, 0);
	lcd.print("WEW ");
	sensors.requestTemperatures();

	tempInside = sensors.getTempCByIndex(0);
	tempOutside = sensors.getTempCByIndex(1);
	//Serial.println(tempInside);
	//Serial.println(tempOutside);
	softSerial.print("TTT");
	softSerial.print(tempInside);
	softSerial.print(";");
	softSerial.println(tempOutside);
	lcd.print(tempInside);
	lcd.moveCursorLeft();
	lcd.print(" ZEW ");
	lcd.print((int)tempOutside);
}

#define NOTE_F2  87
#define NOTE_E3  165

int alarmTone[] = {
	NOTE_F2, NOTE_E3, NOTE_F2, NOTE_E3
};

int noteDurations[] = {
	8,8,8,8
};

void playAlarm() {
	//Serial.println("Hello from alarm player!");
	long startedTime = millis();
	while (startedTime + alarmDuration > millis()) {
		for (int thisNote = 0; thisNote < 8; thisNote++) {
			int noteDuration = 1000 / noteDurations[thisNote];
			tone(ALARM_OUTPUT, alarmTone[thisNote], noteDuration);

			int pauseBetweenNotes = noteDuration * 1.30;
			delay(pauseBetweenNotes);
			noTone(ALARM_OUTPUT);
		}
	}
	isAlarmRunning = false;
}

int isEEPROMData = 0;

void setup() {

	pinMode(SMOKE_IND, OUTPUT);
	pinMode(MONOXIDE_IND, OUTPUT);
	pinMode(ALARM_IND, OUTPUT);
	pinMode(AUTO_LIGHT_IND, OUTPUT);
	pinMode(LIGHT, OUTPUT);
	pinMode(PIR_SENSOR, INPUT);
	pinMode(ALARM_OUTPUT, OUTPUT);
	pinMode(TOUCH_SENSOR, INPUT);
	softSerial.begin(9600);
	Serial.begin(9600);


	while (!Serial);
	
	setTimeFromComputer();
	isEEPROMData = EEPROM.read(0);
	if (isEEPROMData == 0) {
		Serial.println("Saving data");
		EEPROM.write(ADDR_BRIGHTNESS, brightness);
		EEPROM.write(ADDR_ALARM, alarmMovement);
		EEPROM.write(ADDR_AUTO_ON, autoSwitchOn);
		EEPROM.write(ADDR_SMOKE, smokeAlarm);
		EEPROM.write(ADDR_MONOXIDE, monoxideAlarm);
		EEPROM.write(ADDR_NOTFIRST, 1);
		EEPROM.write(ADDR_ALARM + 11, alarmCustomSettings);
		EEPROM.write(ADDR_ALARM + 12, alarmSinceHour);
		EEPROM.write(ADDR_ALARM + 13, alarmSinceMinute);
		EEPROM.write(ADDR_ALARM + 14, alarmToHour);
		EEPROM.write(ADDR_ALARM + 15, alarmToMinute);
		EEPROM.write(ADDR_ALARM + 16, alarmWeekDays);
		EEPROM.write(ADDR_AUTO_ON + 21, alarmCustomSettings);
		EEPROM.write(ADDR_AUTO_ON + 22, autoLightSinceHour);
		EEPROM.write(ADDR_AUTO_ON + 23, autoLighSinceMinute);
		EEPROM.write(ADDR_AUTO_ON + 24, autoLightToHour);
		EEPROM.write(ADDR_AUTO_ON + 25, autoLightToMinute);
		EEPROM.write(ADDR_AUTO_ON + 26, autoLightWeekDays);
	}
	else if(isEEPROMData==1){
		Serial.println("Reading data");
		brightness=EEPROM.read(ADDR_BRIGHTNESS);
		alarmMovement=EEPROM.read(ADDR_ALARM);
		autoSwitchOn = EEPROM.read(ADDR_AUTO_ON);
		smokeAlarm = EEPROM.read(ADDR_SMOKE );
		monoxideAlarm= EEPROM.read(ADDR_MONOXIDE);
		alarmCustomSettings = EEPROM.read(ADDR_ALARM + 11);
		alarmSinceHour = EEPROM.read(ADDR_ALARM + 12);
		alarmSinceMinute= EEPROM.read(ADDR_ALARM + 13);
		alarmToHour=EEPROM.read(ADDR_ALARM + 14);
		alarmToMinute=EEPROM.read(ADDR_ALARM + 15);
		alarmWeekDays=EEPROM.read(ADDR_ALARM + 16);
		alarmCustomSettings=EEPROM.read(ADDR_AUTO_ON + 21);
		autoLightSinceHour=EEPROM.read(ADDR_AUTO_ON + 22);
		autoLighSinceMinute=EEPROM.read(ADDR_AUTO_ON + 23);
		autoLightToHour=EEPROM.read(ADDR_AUTO_ON + 24);
		autoLightToMinute=EEPROM.read(ADDR_AUTO_ON + 25);
		autoLightWeekDays=EEPROM.read(ADDR_AUTO_ON + 26);
	}
	Serial.println(isEEPROMData);


	delay(10000);
	Serial.println("Hello from Serial");
	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("Initialize LCD!");
	displayIP();
	delay(1500);
	sensors.begin();
	updateTemp();
	delay(1500);
	timer.every(15*60000, updateTemp);
	alarmThread.onRun(playAlarm);
	
}

void setTimeFromComputer() {
	bool parse = false;
	bool config = false;

	if (getDate(__DATE__) && getTime(__TIME__)) {
		parse = true;
	}


	if (parse) {
		//Serial.print("DS1307 configured Time=");
		//Serial.print(__TIME__);
		//Serial.print(", Date=");
		//Serial.println(__DATE__);
	}
	else if (parse) {
		//Serial.println("DS1307 Communication Error :-{");
		//Serial.println("Please check your circuitry");
	}
	else {
		//Serial.print("Could not parse info from the compiler, Time=\"");
		//Serial.print(__TIME__);
		//Serial.print("\", Date=\"");
		//Serial.print(__DATE__);
		//Serial.println("\"");
	}

	//Serial.print("weekDay: ");
	//Serial.println(tm.Wday);
}


bool getTime(const char *str)
{
	int Hour, Min, Sec;

	if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
	tm.Hour = Hour;
	tm.Minute = Min;
	tm.Second = Sec;
	return true;
}

bool getDate(const char *str)
{
	char Month[12];
	int Day, Year;
	uint8_t monthIndex;

	if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;

	tm.Day = Day;
	tm.Month = monthIndex + 1;
	tm.Year = CalendarYrToTm(Year);
	return true;
}

void reset_alarm() {
	digitalWrite(ALARM_OUTPUT, LOW);
}

void reset_light() {
	digitalWrite(LIGHT, LOW);
	brightness = 0;
}

void loop() {
	
	timer.update();

		analogWrite(LIGHT, map(brightness, 0, 100, 0, 255));

		if (analogRead(A0) < 333) {
			smokeLevel = 1;
		}
		else if (analogRead(A0) < 666) {
			smokeLevel = 2;
		}
		else if(smokeAlarm)
		{
			isAlarmRunning = true;
			alarmThread.run();
			smokeLevel = 3;
		}

		if (analogRead(A1) < 333) {
			monoxideLevel = 1;
		}
		else if (analogRead(A1) < 666) {
			monoxideLevel = 2;
		}
		else if(monoxideAlarm)
		{
			isAlarmRunning = true;
			alarmThread.run();
			monoxideLevel = 3;
		}

		digitalWrite(SMOKE_IND, smokeAlarm);

		digitalWrite(MONOXIDE_IND, monoxideAlarm);

		digitalWrite(ALARM_IND, alarmMovement);

		digitalWrite(AUTO_LIGHT_IND, autoSwitchOn);
		

	if (digitalRead(TOUCH_SENSOR) == HIGH && touchFlag) {
		touchFlag = false;
		Serial.println(".");
		if (brightness == 0) {
			updateLight(255);
		}
		else{
			updateLight(0);
		}
	}
	else if (digitalRead(TOUCH_SENSOR) == LOW){
		touchFlag = true;
	}

	
	if (digitalRead(PIR_SENSOR) == HIGH) {

		if (alarmMovement && !alarmCustomSettings) {
			isAlarmRunning = true;
			alarmThread.run();
			Serial.print("running alarm");
		}
		else if (alarmMovement) {
			if (alarmSinceHour<tm.Hour && alarmSinceMinute < tm.Minute
				&& alarmToHour> tm.Hour && alarmToHour > tm.Minute) {
				isAlarmRunning = true;
				alarmThread.run();
				Serial.print("running alarm");
			}
		}

		if (autoSwitchOn) {
			brightness = 255;
			timer.after(15000, reset_light);
		}

	}

	if (softSerial.available() > 4) {
		message = "";
		//Serial.print("getting...");
		char c;
		for (int i = 0; i < 3; i++) {
			c = softSerial.read();
			message.concat(c);
		}

		if (message.equals("MSG")) {
			//Serial.print("Wiadomosc...");
			//Serial.println(message);

			c = softSerial.read();
			if (c == '=') {
				c = softSerial.read();
				switch (c) {
				case CMD_SWITCH_LIGHT:
					Serial.println("");
					Serial.print("Swiatlo ");
					c = softSerial.read();
					if (c == '1') {
						Serial.println("ON");
						updateLight(255);
					}
					else if (c == '0') {
						Serial.println("OFF");
						updateLight(0);
					}
					break;
				case CMD_CHANGE_BRIGHTNESS:

					updateLight(softSerial.readString().toInt());

					Serial.print("Jasnosc: ");
					Serial.println(brightness);
					break;
				case CMD_GET_DATA: {
					
					softSerial.print("{\"STATUS\":\"OK\", \"BRIGHTNESS\":");
					softSerial.print(brightness);
					softSerial.print(", \"TEMP_IN\":");
					softSerial.print(tempInside);
					softSerial.print(", \"TEMP_OUT\":");
					softSerial.print(tempOutside);
					softSerial.print(", \"ALARM\":");
					softSerial.print(alarmMovement);
					softSerial.print(", \"ALARM1\":");
					softSerial.print(alarmCustomSettings);
					softSerial.print(", \"ALARM2\":");
					softSerial.print(alarmSinceHour);
					softSerial.print(", \"ALARM3\":");
					softSerial.print(alarmSinceMinute);
					softSerial.print(", \"ALARM4\":");
					softSerial.print(alarmToHour);
					softSerial.print(", \"ALARM5\":");
					softSerial.print(alarmToMinute);
					softSerial.print(", \"ALARM6\":");
					softSerial.print(alarmWeekDays);
					softSerial.print(", \"AUTO_ON\":");
					softSerial.print(autoSwitchOn);
					softSerial.print(", \"AUTO_ON1\":");
					softSerial.print(autoLightCustomSettings);
					softSerial.print(", \"AUTO_ON2\":");
					softSerial.print(autoLightSinceHour);
					softSerial.print(", \"AUTO_ON3\":");
					softSerial.print(autoLighSinceMinute);
					softSerial.print(", \"AUTO_ON4\":");
					softSerial.print(autoLightToHour);
					softSerial.print(", \"AUTO_ON5\":");
					softSerial.print(autoLightToMinute);
					softSerial.print(", \"AUTO_ON6\":");
					softSerial.print(autoLightWeekDays);
					softSerial.print(", \"SMOKE\":");
					softSerial.print(smokeLevel);
					softSerial.print(", \"MONOXIDE\":");
					softSerial.print(monoxideLevel);
					softSerial.print(", \"SMOKE_ALARM\":");
					softSerial.print(smokeAlarm);
					softSerial.print(", \"MONOXIDE_ALARM\":");
					softSerial.print(monoxideAlarm);
					softSerial.println("}");
					break;
				}
				case CMD_ALARM: {
					String response;
					Serial.println("Switching alarm");
					char c = softSerial.read();
					if (c == '1') 
						alarmMovement = true;
					else
						alarmMovement = false;

					Serial.print("Response: ");
					response = softSerial.readString();
					Serial.println(response);
					if (response.charAt(0) == '1') {
						alarmCustomSettings = true;
					}
					else {
						alarmCustomSettings = false;
					}
					
					alarmSinceHour = response.substring(4,6).toInt();
					alarmSinceMinute = response.substring(6, 8).toInt();
					alarmToHour = response.substring(8, 10).toInt();
					alarmToMinute = response.substring(10, 12).toInt();
					alarmWeekDays = response.substring(1, 4).toInt();
						Serial.println("Data:");
						Serial.println(alarmSinceHour);
						Serial.println(alarmSinceMinute);
						Serial.println(alarmToHour);
						Serial.println(alarmToMinute);
						Serial.println(alarmWeekDays);

						EEPROM.update(ADDR_ALARM, alarmMovement);
						EEPROM.update(ADDR_ALARM+11, alarmCustomSettings);
						EEPROM.update(ADDR_ALARM+12, alarmSinceHour);
						EEPROM.update(ADDR_ALARM+13, alarmSinceMinute);
						EEPROM.update(ADDR_ALARM+14, alarmToHour);
						EEPROM.update(ADDR_ALARM+15, alarmToMinute);
						EEPROM.update(ADDR_ALARM+16, alarmWeekDays);
					break;
				}
				case CMD_AUTO_LIGHT_ON: {
					Serial.println("Switching auto light");
					char c = softSerial.read();
					if (c == '1')
						autoSwitchOn = true;
					else
						autoSwitchOn = false;

					Serial.print("Response: ");
					String response = softSerial.readString();
					Serial.println(response);
					if (response.charAt(0) == '1') {
						autoLightCustomSettings = true;
					}
					else {
						autoLightCustomSettings = false;
					}

					autoLightSinceHour = response.substring(4, 6).toInt();
					autoLighSinceMinute = response.substring(6, 8).toInt();
					autoLightToHour = response.substring(8, 10).toInt();
					autoLightToMinute = response.substring(10, 12).toInt();
					autoLightWeekDays = response.substring(1, 4).toInt();
					Serial.println("Data:");
					Serial.println(alarmSinceHour);
					Serial.println(alarmSinceMinute);
					Serial.println(alarmToHour);
					Serial.println(alarmToMinute);
					Serial.println("weekDays:");
					Serial.println(alarmWeekDays);
					//Serial.println(response.substring(1, 4).toInt());

					EEPROM.write(ADDR_AUTO_ON, autoSwitchOn);
					EEPROM.write(ADDR_AUTO_ON + 21, autoSwitchOn);
					EEPROM.write(ADDR_AUTO_ON + 22, autoLightSinceHour);
					EEPROM.write(ADDR_AUTO_ON + 23, autoLighSinceMinute);
					EEPROM.write(ADDR_AUTO_ON+24, autoLightToHour);
					EEPROM.write(ADDR_AUTO_ON+25, autoLightToMinute);
					EEPROM.write(ADDR_AUTO_ON+26, autoLightWeekDays);
					break;
				}
				case CMD_SMOKE_ALARM: {
					Serial.println("Switching smoke alarm");
					char c = softSerial.read();
					if (c == '1')
						smokeAlarm = true;
					else
						smokeAlarm = false;
					EEPROM.write(ADDR_SMOKE, smokeAlarm);
					break;
				}

				case CMD_MONOXIDE_ALARM:{
					Serial.println("Switching monoxide alarm");
					char c = softSerial.read();
					if (c == '1') 
						monoxideAlarm = true;
					else
						monoxideAlarm = false;
					EEPROM.write(ADDR_MONOXIDE, monoxideAlarm);
					break;
			    }

				case 'I':

					break;
				}
			}
			while (softSerial.available())
				softSerial.read();
		}
	}
	else {
	}
	
}
/*
int validateByte(String value) {

	for (int i = 0; i < value.length(); i++) {
		if (value.charAt(i) != '0')
			return value.substring(i).toInt();
	}
}
*/
void updateLight(int i) {
	if (i == 1) {
		brightness = i;
		EEPROM.update(ADDR_BRIGHTNESS, brightness);
	}
	else {
		brightness = i;
		EEPROM.update(ADDR_BRIGHTNESS, brightness);
	}
}

void displayIP() {

	Serial.println("Getting IP");
	while (softSerial.available())
	{
		softSerial.read();
	}
	softSerial.println("III");
	
	message = "";
	unsigned long start = millis();

	while (millis() - start < 3000) {
		if (softSerial.available()>5) {
			message = softSerial.readString();
			break;
		}
	}
	message.remove(message.length()-2);
	Serial.println(message);
	lcd.setCursor(0, 1);
	lcd.print(message);
	message = "";
}