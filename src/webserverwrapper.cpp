#include "webserverwrapper.h"
	
WebServerWrapper::WebServerWrapper(IPAddress addr, int port):WebServer(addr,port)
{

}

WebServerWrapper::WebServerWrapper(int port):WebServer(port)
{

}

WebServerWrapper::~WebServerWrapper()
{

}

const bool WebServerWrapper::hasVar(const String &name)
{
	return (hasHeader(name) ? true : hasArg(name));
}

String WebServerWrapper::var(const String &name)
{
	if(hasVar(name)==false)
	{
		return String("");
	}
	if(hasHeader(name)==true)
	{
		return header(name);
	}
	else
	{
		return arg(name);
	}
}

const bool WebServerWrapper::removeHandler(RequestHandler *handler)
{
	return _removeRequestHandler(handler);
}

const bool WebServerWrapper::_removeRequestHandler(RequestHandler *handler)
{
	RequestHandler *prev=0;
	RequestHandler *h=_firstHandler;
	while(h && h!=handler)
	{
		prev=h;
		h=h->next();
	}
	if(h==handler)
	{
		if(_firstHandler==h)
		{
			_firstHandler=_firstHandler->next();
		}
		if(_lastHandler==h)
		{
			_lastHandler=prev;
		}
		if(prev)
		{
			prev->next(handler->next());
		}
		return true;
	}
	return false;
}
