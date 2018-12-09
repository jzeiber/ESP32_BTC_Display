#ifndef _fsbrowser_
#define _fsbrowser_

#include "webserverwrapper.h"

#include <SPIFFS.h>

#include <map>

class FSBrowser:public RequestHandler
{
public:
	FSBrowser();
	virtual ~FSBrowser();
	
	void SetHandler(WebServerWrapper &web, const String &uri);
	
    virtual bool canHandle(HTTPMethod method, String uri) override;
    virtual bool canUpload(String uri) override;
    virtual bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) override;
    virtual void upload(WebServer& server, String requestUri, HTTPUpload& upload) override;
	
private:

	void SetupContentTypes();
	const String ContentType(const String &filename) const;
	
	std::map<String,String> m_contenttypes;

	WebServerWrapper *m_web;
	String m_uri;
	File m_uploadfile;
};

#endif	// _fsbrowser_
