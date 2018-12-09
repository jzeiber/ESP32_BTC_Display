#ifndef _btcpriceupdate_
#define _btcpriceupdate_

#include <WString.h>
#include <Print.h>

#include <chrono>

namespace BTCPrice
{

class PriceUpdater
{
public:
	PriceUpdater();
	~PriceUpdater();
	
	void SetUpdateInterval(const int ms);
	void SetPrinter(Print &printer);
	void SetCurrency(const String &currency);
	
	virtual const bool Update()=0;
	const float Price() const;
	const String LastPriceUpdate() const;
	const String Currency() const;
	
protected:
	float m_price;
	int m_updateinterval;
	String m_lastpriceupdate;
	std::chrono::system_clock::time_point m_lastupdated;
	Print *m_printer;
	String m_currency;
	
	void PrintText(const String &text);
	void PrintTextLn(const String &text);
	
private:

};

class CoinDeskPriceUpdater:public PriceUpdater
{
public:
	CoinDeskPriceUpdater();
	~CoinDeskPriceUpdater();
	
	const bool Update();
};

}	// namespace BTCPrice

#endif	// _btcpriceupdate_
