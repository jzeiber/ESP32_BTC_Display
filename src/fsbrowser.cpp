#include "fsbrowser.h"

#include "stringfunctions.h"

FSBrowser::FSBrowser():m_web(0),m_uri(String(F("/fsbrowse")))
{
	SetupContentTypes();
}

FSBrowser::~FSBrowser()
{
	if(m_web)
	{
		m_web->removeHandler(this);
	}
}

void FSBrowser::SetHandler(WebServerWrapper &web, const String &uri)
{
	if(m_web)
	{
		m_web->removeHandler(this);
	}
	m_web=&web;
	if(m_web)
	{
		m_web->addHandler(this);
	}
	m_uri=uri;
}

bool FSBrowser::canHandle(HTTPMethod method, String uri)
{
	if(uri!=m_uri)
	{
		return false;
	}
	return true;
}

bool FSBrowser::canUpload(String uri)
{
	if(!canHandle(HTTP_POST,uri))
	{
		return false;
	}
	return true;	
}

bool FSBrowser::handle(WebServer& server, HTTPMethod requestMethod, String requestUri)
{
	if(!canHandle(requestMethod,requestUri))
	{
		return false;
	}
	
	String path=m_web->var("path");
	//Serial.println("Initial path="+path);
	
	if(m_web->hasVar("download") && m_web->hasVar("path"))
	{
		File f=SPIFFS.open(m_web->var("path"));
		if(f && f.isDirectory()==false)
		{
			server.streamFile(f,ContentType(f.name()));
			f.close();
			return true;
		}
		else
		{
			server.send(404, F("text/plain"), F("File not found"));
			return false;
		}
	}
	if(m_web->hasVar("delete") && m_web->hasVar("path"))
	{
		File f=SPIFFS.open(m_web->var("path"));
		if(f)
		{
			if(f.isDirectory())
			{
				f.close();
				SPIFFS.rmdir(m_web->var("path"));
			}
			else
			{
				f.close();
				SPIFFS.remove(m_web->var("path"));
			}
			int spos=path.lastIndexOf('/');
			if(spos>-1)
			{
				path=path.substring(0,spos);
			}
			//Serial.println("After delete path="+path);
		}
		else
		{
			server.send(404, F("text/plain"), F("File not found"));
			return false;
		}
	}
	/*
	if(m_web->hasVar("createdir") && m_web->hasVar("newpath"))
	{
		String newpath(path);
		if(newpath.lastIndexOf('/')!=newpath.length()-1)
		{
			newpath+='/';
		}
		newpath+=m_web->var("newpath");
		Serial.println("Trying to create path "+newpath);
		SPIFFS.mkdir(newpath);
	}
	*/
	
	// TODO - finish handling
	
	String html=String(F("<html><head><style type=\"text/css\">td.fname,th.fname {text-align:left;} td.fsize,th.fsize {text-align:right;}</style></head><body>"));
	if(path.length()==0)
	{
		path="/";
	}
	
	File d=SPIFFS.open(path);
	if(d && d.isDirectory())
	{
		String fullpath(d.name());
		if(fullpath.lastIndexOf('/')!=fullpath.length()-1)
		{
			fullpath+='/';
		}
		//Serial.println("fullpath="+fullpath);
		html+="<table>";
		html+="<thead><tr><th colspan=\"4\">Browsing "+htmlspecialchars(fullpath)+"</th></tr>";
		html+="<tr><th>Name</th><th>Size</th><th>Type</th></tr>";
		html+="</thead>";
		if(path!="/")
		{
			String parent("/");
			int spos=path.lastIndexOf('/');
			if(spos>-1)
			{
				parent=path.substring(0,spos);
			}
			html+="<tr><td class=\"fname\"><a href=\"?path="+uriencode(parent)+"\">..</a></td></tr>";
		}
		File f=d.openNextFile();
		while(f)
		{
			String fname(f.name());
			if(fname.length()>0 && fname.charAt(0)=='/')
			{
				fname=fname.substring(1);
			}
			html+="<tr><td>";

			html+="<a href=\"?path="+uriencode(fullpath+fname)+(f.isDirectory() ? "" : "&download=Y")+"\">";
			
			html+=htmlspecialchars(f.name());
			if(f.isDirectory())
			{
				html+="</a>";
			}
			html+="</td><td class=\"fsize\">"+String(f.size())+"</td><td>"+(f.isDirectory() ? "DIR" : "FILE")+"</td>";
			html+="<td><a href=\"?path="+uriencode(fullpath+fname)+"&delete=Y\" onClick=\"return alert('Delete this entry from the file system?');\">delete</a></td>";
			html+="</tr>";
			f=d.openNextFile();
		}
		size_t usedbytes=SPIFFS.usedBytes();
		size_t totalbytes=SPIFFS.totalBytes();
		html+="<tr><th class=\"fname\">File System Used</th><th class=\"fsize\">"+String(usedbytes)+"</th></tr>";
		html+="<tr><th class=\"fname\">File System Size</th><th class=\"fsize\">"+String(totalbytes)+"</th></tr>";
		html+="</table>";
		
		html+="<form method=\"post\" enctype=\"multipart/form-data\">";
		html+="Upload File ";
		html+="<input type=\"file\" name=\"name\">";
		html+="<input type=\"submit\" value=\"Upload\">";
		html+="</form>";
		
		/*
		html+="<form method=\"post\">";
		html+="Create Directory ";
		html+="<input type=\"hidden\" name=\"path\" value=\""+htmlspecialchars(path)+"\">";
		html+="<input type=\"hidden\" name=\"createdir\" value=\"Y\">";
		html+="<input type=\"text\" name=\"newpath\" placeholder=\"Directory Name\">";
		html+="<input type=\"submit\" value=\"Create\">";
		html+="</form>";
		*/
		
	}

	html+=String(F("</body></html>"));
	
	server.send(200, F("text/html"), html);
	
	return true;
}

void FSBrowser::upload(WebServer& server, String requestUri, HTTPUpload& upload)
{
	if(requestUri!=m_uri || upload.filename.length()==0)
	{
		return;
	}
	if(upload.status==UPLOAD_FILE_START)
	{
		if(m_uploadfile)
		{
			m_uploadfile.close();
		}
		String filename=upload.filename;
		if(filename.charAt(0)!='/')
		{
			filename=String('/')+filename;
		}
		m_uploadfile=SPIFFS.open(filename,"w");
	}
	else if(upload.status==UPLOAD_FILE_WRITE)
	{
		if(m_uploadfile)
		{
			m_uploadfile.write(upload.buf,upload.currentSize);
		}
	}
	else if(upload.status==UPLOAD_FILE_END)
	{
		if(m_uploadfile)
		{
			m_uploadfile.close();
		}
	}
}

void FSBrowser::SetupContentTypes()
{
	m_contenttypes.clear();
	m_contenttypes[".css"]=String(F("text/css"));
	m_contenttypes[".gif"]=String(F("image/gif"));
	m_contenttypes[".gz"]=String(F("application/x-gzip"));
	m_contenttypes[".htm"]=String(F("text/html"));
	m_contenttypes[".html"]=String(F("text/html"));
	m_contenttypes[".ico"]=String(F("image/x-icon"));
	m_contenttypes[".ini"]=String(F("text/plain"));
	m_contenttypes[".jpeg"]=String(F("image/jpeg"));
	m_contenttypes[".jpg"]=String(F("image/jpeg"));
	m_contenttypes[".js"]=String(F("application/javascript"));
	m_contenttypes[".pdf"]=String(F("application/x-pdf"));
	m_contenttypes[".png"]=String(F("image/png"));
	m_contenttypes[".txt"]=String(F("text/plain"));
	m_contenttypes[".xml"]=String(F("text/xml"));
	m_contenttypes[".zip"]=String(F("application/x-zip"));
}

const String FSBrowser::ContentType(const String &filename) const
{
	String fn=filename;
	fn.toLowerCase();
	for(std::map<String,String>::const_iterator i=m_contenttypes.begin(); i!=m_contenttypes.end(); ++i)
	{
		if(fn.endsWith((*i).first))
		{
			return (*i).second;
		}
	}
	return F("application/octet-stream");
}
