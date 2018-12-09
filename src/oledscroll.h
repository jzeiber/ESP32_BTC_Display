#ifndef _oledscroll_
#define _oledscroll_

#include <OLEDDisplay.h>
#include <WString.h>

#include <chrono>
#include <vector>

class OLEDScroll
{
public:
	OLEDScroll();
	~OLEDScroll();

	void Set(OLEDDisplay &display, const String &text, const uint8_t *font, const int speedx=1, const int speedms=50, bool restartscroll=false, const int pausetimems=2000);

	void Display(OLEDDisplay &display, const int y);

protected:

	void UpdateOffset();

private:
	String m_text;
	int m_displaywidth;
	int m_totalwidth;
	int m_lastoffset;
	int m_speedx;
	int m_speedms;
	std::chrono::system_clock::time_point m_lastupdate;
	int64_t m_msremainder;
	const uint8_t *m_font;
	int m_dir=0;
	bool m_restartscroll;
	int m_pausetimems;
	bool m_pausing;
	std::vector<int> m_charwidths;

};



struct scrollline
{
public:
  scrollline():m_text(""),m_displaywidth(0),m_totalwidth(0),m_lastoffset(0),m_speed(0),m_msremainder(0),m_lastupdate(std::chrono::system_clock::now()),m_font(0),m_dir(0),m_restartscroll(false),m_pausetimems(0) { }
  String m_text;
  int m_displaywidth;
  int m_totalwidth;
  int m_lastoffset;
  int m_speed;
  std::chrono::system_clock::time_point m_lastupdate;
  int64_t m_msremainder;
  const uint8_t *m_font;
  int m_dir=0;
  bool m_restartscroll=false;
  int m_pausetimems=0;

  void Set(OLEDDisplay &display, const String &text, const uint8_t *font, const int speed=2, bool restartscroll=false, const int pausetimems=2000)
  {
    m_text=text;
    m_font=font;
    display.setFont(font);
    m_displaywidth=display.getWidth();
    m_totalwidth=display.getStringWidth(text);
    m_lastoffset=0;
    m_speed=speed;
    m_lastupdate=std::chrono::system_clock::now();
    m_msremainder=0;
    m_dir=1;
    m_restartscroll=restartscroll;
    m_pausetimems=pausetimems;
    m_pausing=false;
    Serial.println("Updated scroll text to \""+text+"\"  totalwidth="+String(m_totalwidth)+"  displaywidth="+String(m_displaywidth));
    m_charwidths.clear();
    m_charwidths.resize(text.length(),0);

    for(int i=0; i<text.length(); ++i)
    {
      m_charwidths[i]=display.getStringWidth(text.substring(i,i+1));
    }
  }

  void Display(OLEDDisplay &display, const int y)
  {
    if(m_font)
    {
      UpdateOffset();
      display.setFont(m_font);
      // find substring and startpos/endpos that will fit on screen - use array of char widths
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
      //Serial.println("Scroll offset "+String(thisoffset)+" s("+String(startpos)+","+String(endpos)+")   tw="+String(totalwidth)+"   otw="+String(m_totalwidth)+"  loffset="+String(m_lastoffset)+"  dispw="+String(m_displaywidth));
      display.drawString(-thisoffset,y,m_text.substring(startpos,endpos));
    }
  }

  const bool UpdateOffset()
  {
    std::chrono::system_clock::time_point now=std::chrono::system_clock::now();
    int64_t ms=m_msremainder+std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastupdate).count();
    if(ms>100)
    {
      bool hitbound=false;
      m_msremainder=0;
      while(ms>100)
      {
        // scoll right or left until hit limit
        if(m_totalwidth>m_displaywidth)
        {
          if(m_lastoffset+(m_speed*m_dir) < 0)
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
          else if(m_lastoffset+m_displaywidth+(m_speed*m_dir)>m_totalwidth)
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
            m_lastoffset+=(m_speed*m_dir);
          }
        }
        ms-=100;
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

private:
  bool m_pausing=false;
  std::vector<int> m_charwidths;
  
};

#endif	// _oledscroll_
