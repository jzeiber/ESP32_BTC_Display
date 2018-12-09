#include "oledscroll.h"
	
OLEDScroll::OLEDScroll():
m_text(""),
m_displaywidth(0),
m_totalwidth(0),
m_lastoffset(0),
m_speedx(0),
m_speedms(0),
m_lastupdate(std::chrono::system_clock::now()),
m_msremainder(0),
m_font(nullptr),
m_dir(0),
m_restartscroll(false),
m_pausetimems(0),
m_pausing(false)
{

}

OLEDScroll::~OLEDScroll()
{

}

void OLEDScroll::Set(OLEDDisplay &display, const String &text, const uint8_t *font, const int speedx, const int speedms, bool restartscroll, const int pausetimems)
{
	m_text=text;
	m_font=font;
	display.setFont(m_font);
	m_displaywidth=display.getWidth();
	m_totalwidth=display.getStringWidth(m_text);
	m_lastoffset=0;
	m_speedx=speedx;
	m_speedms=speedms;
	m_lastupdate=std::chrono::system_clock::now();
	m_msremainder=0;
	m_dir=1;
	m_restartscroll=restartscroll;
	m_pausetimems=pausetimems;
	m_pausing=false;
	m_charwidths.clear();
	m_charwidths.resize(m_text.length(),0);
	
	for(int i=0; i<m_text.length(); ++i)
	{
		m_charwidths[i]=display.getStringWidth(m_text.substring(i,i+1));
	}
}

void OLEDScroll::Display(OLEDDisplay &display, const int y)
{
	if(m_font)
	{
		UpdateOffset();
		display.setFont(m_font);
		int startpos=0;
		int endpos=m_text.length();
		int thisoffset=m_lastoffset;
		int totalwidth=m_totalwidth;
		
		while(startpos<endpos && thisoffset>(m_displaywidth/2))
		{
			thisoffset-=m_charwidths[startpos];
			totalwidth-=m_charwidths[startpos];
			startpos++;
		}
		while(endpos>startpos && totalwidth>(m_displaywidth*2))
		{
			totalwidth-=m_charwidths[endpos-1];
			endpos--;
		}
		
		display.drawString(-thisoffset,y,m_text.substring(startpos,endpos));
	}
}

void OLEDScroll::UpdateOffset()
{
	std::chrono::system_clock::time_point now=std::chrono::system_clock::now();
	int64_t ms=m_msremainder+std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastupdate).count();
	if(ms>m_speedms)
	{
		bool hitbound=false;
		m_msremainder=0;
		while(ms>=m_speedms)
		{
			// scroll right or left until hit limit
			if(m_totalwidth>m_displaywidth)
			{
				if(m_lastoffset+(m_speedx*m_dir) < 0)
				{
					m_lastoffset=0;
					m_dir=-m_dir;
					if(m_pausing==true)
					{
						m_pausing=false;
					}
					else
					{
						hitbound=true;
					}
				}
				else if(m_lastoffset+m_displaywidth+(m_speedx*m_dir) > m_totalwidth)
				{
					if(m_restartscroll==false)
					{
						m_lastoffset=m_totalwidth-m_displaywidth;
						m_dir=-m_dir;
					}
					if(m_pausing==true)
					{
						m_pausing=false;
						if(m_restartscroll==true)
						{
							m_lastoffset=0;
						}
					}
					else
					{
						hitbound=true;
					}
				}
				else
				{
					m_lastoffset+=(m_speedx*m_dir);
				}
			}
			ms-=m_speedms;
		}
		m_msremainder=ms;
		m_lastupdate=now;
		if(hitbound==true)
		{
			m_pausing=true;
			m_lastupdate+=std::chrono::milliseconds(m_pausetimems);
		}
		else
		{
			m_pausing=false;
		}
	}
}
