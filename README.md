# Simple 7segment clock
Simple esp8266 + tm1637 NTP clock
Converting a 7 segment STC Chinese clock to ESP8266 + tm1637 with optional BME280/BMP280 Humidity and temperature sensor on separate display.

*No major hardware changes required: just a small bunch of wires between ESP8266, clock PCB, TM1637 module and BME280/BMP280 module.*

**Attention: the following project contains fast-and-dirty firmware code. Made for myself so you probably would need to adapt it for your needs.**

## Features
* 24 hour time display
* display auto-dim
* relative humidity (%), temperature (C) and atmospheric pressure (hPa) display on a second optional TM1637 module
* webserver with http page showing the time, humidity, temperature, pressure; Time zone, tempreture bias setup, manual clock setup.

List of unnecessary for me and therefore absent functions:
* **no** date display/set
* **no** alarm
* **no RTC** (the clock doesn't work without electricity anyway so why bother)
* **no etc**

## The bodging diagram


