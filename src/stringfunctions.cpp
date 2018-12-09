#include "stringfunctions.h"

String htmlspecialchars(const String &in)
{
	String out(in);
	out.replace("&","&amp;");
	out.replace("\"","&quot;");
	out.replace("'","&apos;");
	out.replace("<","&lt;");
	out.replace(">","&gt;");
	return out;
}

String uriencode(const String &in)
{
  String encoded("");
  encoded.reserve(in.length());
  char c;
  char code0;
  char code1;
  for(int i=0; i<in.length(); ++i)
  {
    c=in.charAt(i);
    if(c==' ')
    {
      encoded+='+';
    }
    else if(isalnum(c))
    {
      encoded+=c;
    }
    else
    {
      code1=(c & 0xf)+'0';
      if((c & 0xf) > 9)
      {
        code1=(c & 0xf) - 10 + 'A';
      }
      c=(c >> 4) & 0xf;
      code0=c+'0';
      if(c > 9)
      {
        code0=c - 10 + 'A';
      }
      encoded+=String('%')+String(code0)+String(code1);
    }
  }
  return encoded;
}
