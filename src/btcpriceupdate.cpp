#include "btcpriceupdate.h"
#include "simplejson.h"

#include <WiFiClientSecure.h>

namespace BTCPrice
{

PriceUpdater::PriceUpdater():
m_price(0),
m_updateinterval(60000),
m_lastpriceupdate(""),
m_printer(nullptr),
m_currency("USD")
{
	m_lastupdated=std::chrono::system_clock::now() - std::chrono::milliseconds(m_updateinterval);
}

PriceUpdater::~PriceUpdater()
{

}

const float PriceUpdater::Price() const
{
	return m_price;
}

void PriceUpdater::SetUpdateInterval(const int ms)
{
	m_updateinterval=ms;
}

void PriceUpdater::SetPrinter(Print &printer)
{
	m_printer=&printer;
}

void PriceUpdater::SetCurrency(const String &currency)
{
	if(currency.length()>0)
	{
		m_currency=currency;
		m_price=0;
		m_lastupdated=std::chrono::system_clock::now() - std::chrono::milliseconds(m_updateinterval);
		m_lastpriceupdate="";
	}
}

const String PriceUpdater::Currency() const
{
	return m_currency;
}

const String PriceUpdater::LastPriceUpdate() const
{
	return m_lastpriceupdate;
}

void PriceUpdater::PrintText(const String &text)
{
	if(m_printer)
	{
		m_printer->print(text);
	}
}

void PriceUpdater::PrintTextLn(const String &text)
{
	if(m_printer)
	{
		m_printer->println(text);
	}
}

CoinDeskPriceUpdater::CoinDeskPriceUpdater()
{

}

CoinDeskPriceUpdater::~CoinDeskPriceUpdater()
{

}

const bool CoinDeskPriceUpdater::Update()
{
	int64_t ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_lastupdated).count();
	if(ms>=m_updateinterval)
	{
		WiFiClientSecure client;
		if(!client.connect(String(F("api.coindesk.com")).c_str(),443))
		{
			return false;
		}
		
		client.print(String(F("GET /v1/bpi/currentprice.json HTTP/1.1\r\nHost: api.coindesk.com\r\nConnection: close\r\n\r\n")));
		delay(50);
		String answer;
		while(client.connected())
		{
			answer+=client.readStringUntil('\r');
			if(client.connected())
			{
				answer+='\r';
			}
			delay(1);
		}
		client.stop();
		
		PrintTextLn(String(F("CoinDeskPriceUpdater::Update response="))+answer);
		
		const int bodypos=answer.indexOf("\r\n\r\n");
		if(!(bodypos>0 && answer.indexOf("{",bodypos)>0))
		{
			PrintTextLn(String(F("CoinDeskPriceUpdater::Update response not in expected format  body="))+String(bodypos)+" brace="+String(answer.indexOf("{",bodypos)));
			return false;
		}
		
		SimpleJSON::JSONContainer json;
		if(json.Parse(answer.substring(bodypos+4)))
		{
			if(json["time"]["updatedISO"].StringValue().length()>0 && json["bpi"][m_currency]["rate_float"].FloatValue()>0)
			{
				m_lastpriceupdate=json["time"]["updatedISO"].StringValue();
				m_price=json["bpi"][m_currency]["rate_float"].FloatValue();
				m_lastupdated=std::chrono::system_clock::now();
				return true;
			}
			
		}
		else
		{
			PrintTextLn(String(F("CoinDeskPriceUpdater::Update unable to parse json")));
			return false;
		}
	}
	return false;
}

}	// namespace BTCPrice
