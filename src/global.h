#ifndef _global_
#define _global_

#include <WString.h>
#include <OLEDDisplay.h>

#include <chrono>

#include "btc_image.h"
#include "mybasicesp.h"
#include "oledscroll.h"
#include "btcpriceupdate.h"

struct wifinetworkinfo
{
  String m_ssid;
  wifi_auth_mode_t m_authmode;
  int32_t m_rssi;
  String m_bssidstr;
};

struct wificonnection
{
  String m_ssid;
  String m_password;
  wifi_mode_t m_mode;
  String m_hostname;
};

namespace global
{
  //std::chrono::system_clock::time_point lastupdatedbtcprice;
  //String currencycode;
  //float btcprice;
  float btcholding;
  //String btcupdatetimestr;
  std::vector<wifinetworkinfo> wifiinfo;
  wificonnection wifi;
  //scrollline scroll;
  OLEDScroll scroll;
  MyBasicESP::MyBasic basic;
  MyBasicESP::SPIFFSImportHandler spiffsimporter;
  BTCPrice::CoinDeskPriceUpdater btcprice;
}

const char wificonfigfilename[] PROGMEM="/wifi_disp.ini";
const char btcconfigfilename[] PROGMEM="/btc_disp.ini";
const char html_header[] PROGMEM="<html><head><title><!--{TITLE}--></title><style type=\"text/css\">body {font-family:Helvetica,San-serif;}</style><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head></html>";
const char html_footer[] PROGMEM="</html>";
const char html_connecttowifi[] PROGMEM="<body>Connect To WiFi"
"<form method=\"post\"><input type=\"hidden\" name=\"formaction\" value=\"savesettings\">"
"Network : <select id=\"bssid\" name=\"bssid\">"
"<!--{WIFINETWORKS}-->"
"</select>"
"<br />"
"Password : <input type=\"password\" id=\"password\" name=\"password\" placeholder=\"WiFi Password\" title=\"Leave empty for unsecured network\" value=\"<!--{PASSWORD}-->\">"
"<br />"
"Host Name : <input type=\"text\" id=\"hostname\" name=\"hostname\" placeholder=\"Host Name\" title=\"Leave blank for default\" value=\"<!--{HOSTNAME}-->\">"
"<br />"
"<input type=\"submit\" value=\"Save\">"
"</form>"
"<form method=\"post\"><input type=\"hidden\" name=\"formaction\" value=\"rescanwifi\">"
"<input type=\"submit\" value=\"Rescan WiFi Networks\">"
"</form>"
"<form method=\"post\"><input type=\"hidden\" name=\"formaction\" value=\"resetsettings\">"
"<input type=\"submit\" value=\"Reset Settings\">"
"</form><a href=\"/btcconfig\">BTC Config</a></body>";
const char html_btcconfig[] PROGMEM="<body>Configure BTC"
"<form method=\"post\">"
"<input type=\"hidden\" name=\"formaction\" value=\"savesettings\">"
"Currency : <select id=\"currency\" name=\"currency\">"
"<!--{CURRENCY}-->"
"</select>"
"<br />"
"<input type=\"text\" id=\"btcholding\" name=\"btcholding\" value=\"<!--{BTCHOLDING}-->\" placeholder=\"Total BTC\">"
"<br />"
"<input type=\"submit\" value=\"Save\">"
"</form><a href=\"/wificonfig\">WiFi Config</a></body>";

#endif	// _global_
