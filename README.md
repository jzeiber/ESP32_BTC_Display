# Bitcoin Display for ESP32 with OLED

![BTC Display on ESP32 with OLED](https://github.com/jzeiber/ESP32_BTC_Display/raw/master/images/btc_display.jpg "BTC Display on ESP32 with OLED")

Features:
* Configure WiFi via web interface (save and loads settings when rebooted) (/wificonfig)
* Display bitcoin price and total value held in selectable currency (/btcconfig)
* Scrolling text on OLED
* Web based MY-BASIC interpreter (/mybasic) http://paladin-t.github.io/my_basic/
* Rudimentary SPIFFS filesystem browser (/fsbrowse)
* OTA update (/update)

Arduino IDE Configuration
1. Add URL for additional boards manager in preferences : https://dl.espressif.com/dl/package_esp32_index.json
2. In boards manager find and install **esp32** by **Espressif Systems**
3. In library manager search for SSD1306 and install **ESP8266 and ESP32 Oled Driver for SSD1306 display** by **Daniel Eichhorn, Fabrice Weinberg**
4. Load btc_display.ino in Arduino IDE
5. Compile and Upload to ESP32 device

How to use
* Power on the ESP32 device.
* On first use, the device will initialize and show a WiFi network to connect to.
* Connect to WiFi network and open the url displayed on device.
* Configure the device WiFi settings to connect to a WiFi network.  These settings will be saved and use on the next bootup.  If the WiFi can't connect, it will fallback to starting in Access Point mode to set up a new WiFi connection.
* When connected to a WiFi network with internet connectivity, the device will retrieve the current Bitcoin price.
* Bitcoin settings can be configured at http://deviceip/btcconfig.
