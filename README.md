# Simple 7segment clock
Simple esp8266 + tm1637 NTP clock
Converting a 7 segment STC Chinese clock to ESP8266 + tm1637 with optional BME280/BMP280 Humidity and temperature sensor on separate display.

*No major hardware changes required: just a small bunch of wires between ESP8266, clock PCB, TM1637 module and BME280/BMP280 module.*

![webpage](https://github.com/onivan/clock-esp-tm1637/blob/main/20200731_182114.jpg)

**Attention: the following project contains fast-and-dirty firmware code. Made for myself so you probably would need to adapt it for your needs.**

## Features
* 24 hour time display
* display auto-dim
* relative humidity (%), temperature (C) and atmospheric pressure (hPa) display on a second optional TM1637 module
* webserver with http page showing the time, humidity, temperature, pressure; Time zone, tempreture bias setup, manual clock setup.

List of unnecessary for me and therefore absent functions:
* no date display/set
* no alarm
* no RTC (the clock doesn't work without electricity anyway so why bother)
* no etc

# The bodging diagram
* Blue: connect to Wemos board
* Purple: connect to TM1637 module
* Look trough other photos to get a hints

![webpage](https://github.com/onivan/clock-esp-tm1637/blob/main/Clock-TM1637connections.jpg)

# Firmware
## Flash precompiled bin with ESPRESSIF Flash Download Tools (just google how to)
or

# Compile it 
Google how to use ESP in Arduino envoriment
Choose a board "LOLIN(WEMOS) D1 R2 & mini"
Flash size: "4MB (FS:1MB OTA:~1019KB)"

**Required libraries:**
* TM1637 => 1.2.0
* Ticker => 1.0
* ESP8266WiFi => 1.0 
* ESP8266WebServer => 1.0
* ESP8266mDNS => 1.2
* EEPROM => 1.0
* Adafruit_Unified_Sensor => 1.0.3
* Adafruit_BME280_Library => 1.0.10
* ezTime => 0.8.2

**Important steps**
* In the code around 32 line change the SSID and password variables according to your WIFI credentials (there is no AP mode here for simplicity)
* Do not forget to upload static html from *data* folder! 
(Tools -> ESP8266 Sketch Data Upload)
* Open the clock's IP (find it in the serial terminal) in your web browser and set your timezone and temperature correction.

**I could forget something or do mistakes so use your brain and google to solve problems. Or just write your own firmware from the scratch**

There are also Gerbers for those who would like to do like this:
![webpage](https://github.com/onivan/clock-esp-tm1637/blob/main/20201208_160532.jpg)

The compatible 1" one-digit 7-segment display models are TOYOLED E11001-G-UR3-0-W or Kingbright SA10-21SRWA (Common Anode )
