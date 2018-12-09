#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SSD1306.h>
#include <SPIFFS.h>
#include <Update.h>

#include <vector>
#include <chrono>
#include <iomanip>

#include "global.h"
#include "stringfunctions.h"
#include "configfile.h"
#include "webserverwrapper.h"
#include "fsbrowser.h"

// pins for display
#define SDA 5
#define SCL 4
#define I2C 0x3C

// create display
SSD1306 display(I2C, SDA, SCL);

WebServerWrapper web(80);
FSBrowser fsbrowse;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);

  display.init();
  display.flipScreenVertically();
  display.clear();
  display.display();

  global::btcprice.SetCurrency("USD");
  global::btcprice.SetPrinter(Serial);
  global::btcholding=0;
  global::basic.SetPrinter(Serial);
  global::basic.SetDebugPrinter(Serial);
  global::basic.DebugPrint(true);
  global::basic.SetImportHandler(global::spiffsimporter);
  global::basic.SetMaximumPrintCache(100);
  //global::basic.SetOLEDDisplay(&display);
  //global::basic.SetOLEDParameters(I2C,SDA,SCL);

  SPIFFS.begin(true);

  String currency("");
  //loadbtcconfig(global::currencycode,global::btcholding);
  loadbtcconfig(currency,global::btcholding);
  global::btcprice.SetCurrency(currency);

  // read WiFi settings from config file
  // if config doesn't exist, or cannot connect to WiFi - start AP mode

  String ssid("");
  String password("");
  String hostname("");
  if(loadwificonfig(ssid,password,hostname))
  {
    Serial.println("Loaded config");
  }
  else
  {
    Serial.println("Couldn't log config");
  }

  global::wifi.m_ssid=ssid;
  global::wifi.m_password=password;
  global::wifi.m_hostname=hostname;

  if(!(ssid.length()>0 && startclientmode(ssid,password,hostname)==true))
  {
    startapmode();
  }

  // handle web requests for ap or client wifi mode
  web.on("/",handlewificonfig);
  web.on(String(F("/wificonfig")),handlewificonfig);
  web.on(String(F("/btcconfig")),handlebtcconfig);
  web.on(String(F("/update")),HTTP_GET,updatepage);
  web.on(String(F("/update")),HTTP_POST,updatestatus,doupdate);
  fsbrowse.SetHandler(web,String(F("/fsbrowse")));
  global::basic.SetWebHandler(web,String(F("/mybasic")));
  web.begin();

  Serial.println("Setup Finished");
  Serial.println("Free Memory "+String(ESP.getFreeHeap()));
  Serial.println("CPU Speed "+String(ESP.getCpuFreqMHz())+" Mhz");
  printchipinfo();

}

void printchipinfo()
{
  esp_chip_info_t chip;
  memset(&chip,0,sizeof(esp_chip_info_t));
  esp_chip_info(&chip);
  Serial.println("CPU Cores "+String(chip.cores));
  Serial.println("Chip Revision "+String(chip.revision));
  Serial.println("Features");
  Serial.println("Embedded Flash:    "+String(chip.features & CHIP_FEATURE_EMB_FLASH ? "Yes" : "No"));
  Serial.println("WiFi b/g/n:        "+String(chip.features & CHIP_FEATURE_WIFI_BGN ? "Yes" : "No"));
  Serial.println("Bluetooth LE:      "+String(chip.features & CHIP_FEATURE_BLE ? "Yes" : "No"));
  Serial.println("Bluetooth Classic: "+String(chip.features & CHIP_FEATURE_BT ? "Yes" : "No"));
  
  /**
 * @brief The structure represents information about the chip
 *//*
typedef struct {
    esp_chip_model_t model;  //!< chip model, one of esp_chip_model_t
    uint32_t features;       //!< bit mask of CHIP_FEATURE_x feature flags
    uint8_t cores;           //!< number of CPU cores
    uint8_t revision;        //!< chip revision number
} esp_chip_info_t;

/*#define CHIP_FEATURE_EMB_FLASH      BIT(0)
#define CHIP_FEATURE_WIFI_BGN       BIT(1)
#define CHIP_FEATURE_BLE            BIT(4)
#define CHIP_FEATURE_BT             BIT(5)
 */
/**
 * @brief Fill an esp_chip_info_t structure with information about the chip
 * @param[out] out_info structure to be filled
 */
/*void esp_chip_info(esp_chip_info_t* out_info);*/
}

void loop() {
  // put your main code here, to run repeatedly:

  web.handleClient();
  global::basic.Update();

  //if(global::basic.UsingOLED()==false)
  {
    if(WiFi.getMode()==WIFI_MODE_STA && WiFi.status()==WL_CONNECTED)
    {
	 if(global::btcprice.Update()==true)
	  {
	    global::scroll.Set(display,trimtrailingzero(String(global::btcholding,6))+String(F(" BTC Worth "))+String(global::btcprice.Price()*global::btcholding)+" "+global::btcprice.Currency(),ArialMT_Plain_16);
	  }
	  displaybtcprice();
    }
  }
  
  delay(50);
}

bool loadwificonfig(String &ssid, String &password, String &hostname)
{
  ConfigFile config;

  if(!config.LoadFromSPIFFS(FPSTR(wificonfigfilename)))
  {
    return false;
  }

  if(config.HasOption(F("ssid")))
  {
    ssid=config.GetOption(F("ssid"));
  }
  else
  {
    ssid="";
  }
  if(config.HasOption(F("password")))
  {
    password=config.GetOption(F("password"));
  }
  else
  {
    password="";
  }
  if(config.HasOption(F("hostname")))
  {
    hostname=config.GetOption(F("hostname"));
  }
  else
  {
    hostname="";
  }
  return true;
}

void savewificonfig(const String &ssid, const String &password, const String &hostname)
{
  ConfigFile config;
  config[F("ssid")]=ssid;
  if(password.length()>0)
  {
    config[F("password")]=password;
  }
  if(hostname.length()>0)
  {
    config[F("hostname")]=hostname;
  }
  config.SaveToSPIFFS(FPSTR(wificonfigfilename));
}

bool loadbtcconfig(String &currency, float &holding)
{
  ConfigFile config;
  if(!config.LoadFromSPIFFS(FPSTR(btcconfigfilename)))
  {
    return false;
  }

  if(config.HasOption(F("currency")))
  {
    currency=config.GetOption(F("currency"));
  }
  else
  {
    currency="USD";
  }
  if(config.HasOption(F("btcholding")))
  {
    holding=config.GetOption(F("btcholding")).toFloat();
  }
  else
  {
    holding=0;
  }
  return true;
}

void savebtcconfig(const String &currency, const String &holding)
{
  ConfigFile config;
  config[F("currency")]=currency;
  if(holding.length()>0)
  {
    config[F("btcholding")]=holding;
  }
  config.SaveToSPIFFS(FPSTR(btcconfigfilename));
}

void startapmode()
{
  // clear out WiFi connection first
  WiFi.disconnect();
  WiFi.begin();

  scanwifi();

  // esp_random only after WiFi started
  srand(esp_random());

  String ap(F("ESP32-"));
  randomstring(ap,4,'0','9');
  String pass("");
  randomstring(pass,8,'0','9');

  WiFi.softAP(ap.c_str(),pass.c_str());

  global::wifi.m_ssid=ap;
  global::wifi.m_password=pass;
  global::wifi.m_mode=WiFi.getMode();

  displayconnectionstatus();
}

bool startclientmode(const String &ssid, const String &password, const String &hostname)
{
  // clear out WiFi connection first
  WiFi.disconnect();
  WiFi.begin();

  if(hostname.length()>0)
  {
    WiFi.setHostname(hostname.c_str());
  }
  
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, F("Connecting to"));
  display.drawString(0, 12, (password.length()>0) ? F("Secured WiFi Network") : F("Unsecured WiFi Network"));
  display.drawString(0, 24, String(F("SSID: "))+ssid);
  display.display();
  
  if(password.length()>0)
  {
    WiFi.begin(ssid.c_str(),password.c_str());
  }
  else
  {
    WiFi.begin(ssid.c_str());
  }

  int count=0;
  while(WiFi.status()!=WL_CONNECTED && (count++)<40)
  {

    //Serial.println("count="+String(count));

    // Draws a rounded progress bar with the outer dimensions given by width and height. Progress is
    // a unsigned byte value between 0 and 100
    //void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress);
    display.drawProgressBar(5,40,display.width()-10,10,(100*count)/40);
    display.display();
    
    delay(250);
  }

  if(WiFi.getMode()==WIFI_MODE_APSTA)
  {
    WiFi.softAPdisconnect(true);
  }

  if(WiFi.status()==WL_CONNECTED)
  {
    global::wifi.m_ssid=ssid;
    global::wifi.m_password=password;
    global::wifi.m_mode=WiFi.getMode();
  }
  displayconnectionstatus();

  return (WiFi.status()==WL_CONNECTED);
  
}

void randomstring(String &str, const unsigned int len, const char startchar, const char endchar)
{
  str.reserve(str.length()+len);
  for(int i=0; i<len; ++i)
  {
    str+=char((rand() % (endchar - startchar)) + startchar);
  }
}

void handlewificonfig()
{
  if(web.method()==HTTP_POST)
  {
    String formaction(web.arg("formaction"));
    if(formaction=="savesettings" && web.hasArg("bssid")==true)
    {
      String ssid("");
      std::vector<wifinetworkinfo>::iterator i=global::wifiinfo.begin();
      while(i!=global::wifiinfo.end() && (*i).m_bssidstr!=web.arg("bssid"))
      {
        ++i;
      }
      if(i!=global::wifiinfo.end() && (*i).m_bssidstr==web.arg("bssid"))
      {
        ssid=(*i).m_ssid;
      }

      savewificonfig(ssid,web.arg("password"),web.arg("hostname"));
      if(startclientmode(ssid,web.arg("password"),web.arg("hostname"))==false)
      {
        Serial.println("Couldn't start client WiFi mode");
        startapmode();
      }
      return;
    }
    else if(formaction=="rescanwifi")
    {
      scanwifi();
      displayconnectionstatus();
    }
    else if(formaction="resetsettings")
    {
      SPIFFS.remove(FPSTR(wificonfigfilename));
    }
  }
  
  String header(FPSTR(html_header));
  String footer(FPSTR(html_footer));
  String body(FPSTR(html_connecttowifi));

  String ssid("");
  String password("");
  String hostname("");
  loadwificonfig(ssid,password,hostname);

  header.replace(F("<!--{TITLE}-->"),F("Select WiFi Network"));

  String networkoptions;
  networkoptions+=String(F("<option value=\"\"></option>"));
  for(std::vector<wifinetworkinfo>::const_iterator wifi=global::wifiinfo.begin(); wifi!=global::wifiinfo.end(); ++wifi)
  {
    networkoptions+=String(F("<option value=\""))+htmlspecialchars((*wifi).m_bssidstr)+"\"";
    if((*wifi).m_ssid==ssid)
    {
      networkoptions+=" selected";
    }
    networkoptions+=">"+htmlspecialchars((*wifi).m_ssid)+((*wifi).m_authmode==WIFI_AUTH_OPEN ? String(F(" (Unsecured)")): String(F(" (Secured)")))+String(F("</option>"));
  }
  body.replace(F("<!--{WIFINETWORKS}-->"),networkoptions);
  body.replace(F("<!--{PASSWORD}-->"),password);
  body.replace(F("<!--{HOSTNAME}-->"),hostname);
  
  web.send(200, F("text/html"), header+body+footer);
}

void handlebtcconfig()
{
  if(web.method()==HTTP_POST)
  {
    String formaction(web.arg("formaction"));
    if(formaction=="savesettings")
    {
      savebtcconfig(web.arg("currency"),web.arg("btcholding"));
	  global::btcprice.SetCurrency(web.arg("currency"));
      if(web.arg("btcholding")!="")
      {
        global::btcholding=web.arg("btcholding").toFloat();
      }
      else
      {
        global::btcholding=0;
      }
	  if(global::btcprice.Update()==true)
	  {
	    global::scroll.Set(display,trimtrailingzero(String(global::btcholding,6))+String(F(" BTC Worth "))+String(global::btcprice.Price()*global::btcholding)+" "+global::btcprice.Currency(),ArialMT_Plain_16);
	  }
    }
    else if(formaction="resetsettings")
    {
      SPIFFS.remove(FPSTR(btcconfigfilename));
    }
  }
  
  String header(FPSTR(html_header));
  String footer(FPSTR(html_footer));
  String body(FPSTR(html_btcconfig));

  header.replace(F("<!--{TITLE}-->"),F("Configure BTC"));

  std::vector<String> currencies;
  currencies.push_back("USD");
  currencies.push_back("GBP");
  currencies.push_back("EUR");
  String currencyoptions("");
  for(std::vector<String>::const_iterator c=currencies.begin(); c!=currencies.end(); ++c)
  {
    currencyoptions+=String(F("<option value=\""))+htmlspecialchars((*c))+"\"";
    //if(global::currencycode==(*c))
	if(global::btcprice.Currency()==(*c))
    {
      currencyoptions+=" selected";
    }
    currencyoptions+=">"+htmlspecialchars((*c))+"</option>";
  }
  body.replace(F("<!--{CURRENCY}-->"),currencyoptions);

  if(global::btcholding>0)
  {
    body.replace(F("<!--{BTCHOLDING}-->"),trimtrailingzero(String(global::btcholding,6)));
  }
  else
  {
    body.replace(F("<!--{BTCHOLDING}-->"),"");
  }
  
  web.send(200, F("text/html"), header+body+footer);
}

void clearwifiinfo()
{
  global::wifiinfo.clear();
}

void scanwifi()
{
  //get list of available networks
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 24, F("Finding WiFi Networks"));
  display.display();
  int16_t netcount=WiFi.scanNetworks();

  Serial.print(netcount);
  Serial.println(F(" WiFi networks found"));
  
  clearwifiinfo();
  for(int16_t i=0; i<netcount; ++i)
  {
    wifinetworkinfo net;
    net.m_ssid=WiFi.SSID(i);
    net.m_authmode=WiFi.encryptionType(i);
    net.m_rssi=WiFi.RSSI(i);
    net.m_bssidstr=WiFi.BSSIDstr(i);
    global::wifiinfo.push_back(net);
  }
}

void displayconnectionstatus()
{
  if(WiFi.getMode()==WIFI_MODE_STA)
  {
    if(WiFi.status()==WL_CONNECTED)
    {
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, F("Connected to"));
      display.drawString(0, 12, (global::wifi.m_password.length()>0) ? F("Secured WiFi Network") : F("Unsecured WiFi Network"));
      display.drawString(0, 24, String(F("SSID: "))+global::wifi.m_ssid);
      display.drawString(0, 36, String(F("Hostname: "))+String(WiFi.getHostname()));
      display.drawString(0, 48, String(F("IP: "))+WiFi.localIP().toString());
      display.display();
    }    
  }
  else if(WiFi.getMode()==WIFI_MODE_AP || WiFi.getMode()==WIFI_MODE_APSTA)
  {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, F("Connect to this network"));
    display.drawString(0, 12, String(F("SSID: "))+global::wifi.m_ssid);
    display.drawString(0, 24, String(F("Password: "))+global::wifi.m_password);
    display.drawString(0, 36, F("Then open"));
    display.drawString(0, 48, String(F("http://"))+WiFi.softAPIP().toString());
    display.display();
  }
}

void displaybtcprice()
{
  /*
  std::ostringstream ostr;
  std::time_t t=std::chrono::system_clock::to_time_t(global::lastupdatedbtcprice);
  ostr << std::put_time(std::localtime(&t),"%Y-%m-%d %H:%M");
  String timestring(ostr.str().c_str());
  */

  //String timestring(global::btcupdatetimestr);
  String timestring(global::btcprice.LastPriceUpdate());
  timestring.replace("T"," ");
  timestring=timestring.substring(0,16);
  
  //void drawXbm(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *xbm);
  // Display on OLED
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawXbm(-1,-1,btc_image_width,btc_image_height,btc_image_bits);
  display.drawString(34,0,"BTC Price");
  display.setFont(ArialMT_Plain_10);
  display.drawString(34,18,timestring);
  display.setFont(ArialMT_Plain_16);
  //display.drawString(0,32,String(global::btcprice)+" "+global::currencycode);
  display.drawString(0,32,String(global::btcprice.Price(),2)+" "+global::btcprice.Currency());
  if(global::btcholding>0)
  {
    global::scroll.Display(display,48);
  }
  display.display();
}

void updatepage()
{
  web.sendHeader("Connection","close");
  web.send(200,"text/html",F("<html><body>Upload new firmware <form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form></body></html>"));
}

void updatestatus()
{
  web.sendHeader("Connection","close");
  web.send(200,"text/plain",(Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}

void doupdate()
{
  HTTPUpload &upload=web.upload();
  if(upload.status==UPLOAD_FILE_START)
  {
    Serial.setDebugOutput(true);
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if(!Update.begin())
    {
      Update.printError(Serial);
    }
  }
  else if(upload.status==UPLOAD_FILE_WRITE)
  {
    if(Update.write(upload.buf,upload.currentSize)!=upload.currentSize)
    {
      Update.printError(Serial);
    }
  }
  else if(upload.status==UPLOAD_FILE_END)
  {
    if(Update.end(true))
    {
      Serial.printf("Update Success: %u\nRebooting...\n",upload.totalSize);
    }
    else
    {
      Update.printError(Serial);
    }
  }
}

String trimtrailingzero(const String &text)
{
  String ret(text);
  while(ret.length()>0 && ret[ret.length()-1]=='0')
  {
    ret.remove(ret.length()-1);
  }
  return ret;
}
