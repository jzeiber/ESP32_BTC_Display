#ifndef _webserverwrapper_
#define _webserverwrapper_

#include <WebServer.h>
  
class WebServerWrapper:public WebServer
{
public:
	WebServerWrapper(IPAddress addr, int port = 80);
	WebServerWrapper(int port = 80);
	virtual ~WebServerWrapper();

	const bool removeHandler(RequestHandler *handler);
	
	const bool hasVar(const String &name);
	String var(const String &name);
	
private:

	const bool _removeRequestHandler(RequestHandler *handler);

};

#endif	// _webserverwrapper_
