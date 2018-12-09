#ifndef _mybasicesp_
#define _mybasicesp_

#include <WebServer.h>
#include <Print.h>
#include <SPIFFS.h>
#include <OLEDDisplay.h>

#include <thread>
#include <mutex>
#include <vector>
#include <cstdarg>
#include <map>

extern "C" {
#include "my_basic.h"
}

namespace MyBasicESP
{

enum MyBasicStatus
{
	MYBASIC_STOPPED=1,
	MYBASIC_RUNNING=2,
	MYBASIC_REQUIREINPUT=3
};

class ImportHandler
{
public:
	ImportHandler()				{}
	virtual ~ImportHandler()	{}
	
	virtual const bool Import(const String &name, String &code)=0;	
};

class SPIFFSImportHandler:public ImportHandler
{
public:
	SPIFFSImportHandler();
	~SPIFFSImportHandler();
	
	const bool Import(const String &name, String &code);
};

class MyBasic:public RequestHandler
{
public:
	MyBasic();
	virtual ~MyBasic();
	
	void SetPrinter(Print &printer);
	void SetDebugPrinter(Print &debugprinter);
	void SetImportHandler(ImportHandler &importhandler);
	//void SetOLEDDisplay(OLEDDisplay *display);
	//void SetOLEDParameters(uint8_t address, uint8_t sda, uint8_t scl);
	
	/* Methods for web server */
	void SetWebHandler(WebServer &web, const String &baseuri);
    virtual bool canHandle(HTTPMethod method, String uri) override;
    virtual bool canUpload(String uri) override;
    virtual bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) override;
    virtual void upload(WebServer& server, String requestUri, HTTPUpload& upload) override;
	
	virtual void Run(String &code, const bool newthread=true);
	
	virtual void RunTask(String &code);
	
	virtual void Update();	// update things that can't be updated on separate thread - MUST be called in main loop() function, not from another thread
	virtual void Stop();
	
	//const bool UsingOLED();
	
	void SetMaximumPrintCache(const int lines);
	void SetMaxStringLength(const int maxlength);
	
	const uint8_t GetStatus();
	const bool WantStop();
	
	void DebugPrint(const bool debugprint);	// verbose printing of message to debug printer
	
protected:
	void ExternalDebugPrint(const String &str);
	void ExternalDebugPrintLn(const String &str);
	void ExternalPrint(const String &str);
	void ExternalPrintLn(const String &str);
	
	virtual const int WaitingFor();

private:
	struct mb_interpreter_t *m_basic;
	Print *m_debugprinter;
	Print *m_printer;
	ImportHandler *m_importhandler;
	String m_baseuri;
	WebServer *m_web;
	std::mutex m_mutex;
	std::vector<String> m_input;
	std::vector<String> m_output;
	uint8_t m_status;
	bool m_wantstop;
	String m_code;
	String m_inputprompt;
	String m_error;
	bool m_runthreaded;
	bool m_runtask;
	bool m_debugprint;
	int m_maximumprintcache;
	//bool m_usingoled;
	//OLEDDisplay *m_display;
	//uint8_t m_oledaddress;
	//uint8_t m_oledsda;
	//uint8_t m_oledscl;
	
	int m_waitingfor;
	enum WaitingEnum
	{
		WAITING_NONE=0,
		WAITING_IMPORT=1,
		WAITING_GETHTTP=2,
		WAITING_POSTHTTP=3,
		WAITING_HEAPMEM=4,
		WAITING_TEMPERATURE=5,
		WAITING_HALL=6,
		WAITING_DELAY=7
	};
	struct paramdata
	{
		int m_result;
		struct mb_interpreter_t *m_basic;
		void **m_ppvoid;
		const char *m_pcchar;
	} m_paramdata;

	static int SteppedHandlerWrapper(struct mb_interpreter_t *mb, void **ast, const char *sourcefile, int sourcepos, unsigned short sourcerow, unsigned short sourcecol);
	int SteppedHandler(struct mb_interpreter_t *mb, void **ast, const char *sourcefile, int sourcepos, unsigned short sourcerow, unsigned short sourcecol);
	static void ErrorHandlerWrapper(struct mb_interpreter_t *mb, mb_error_e e, const char *m, const char *f, int p, unsigned short row, unsigned short col, int abort_code);
	void ErrorHandler(struct mb_interpreter_t *mb, mb_error_e e, const char *m, const char *f, int p, unsigned short row, unsigned short col, int abort_code);
	static int ImportHandlerWrapper(struct mb_interpreter_t *mb, const char *name);
	int InternalImportHandler(struct mb_interpreter_t *mb, const char *name);
	
	static void RunThreadWrapper(void *);
	
	const bool GetHttpInfo(const String &url, String &method, bool &secure, String &host, String &port, String &path);
	static int GetHttpWrapper(struct mb_interpreter_t *mb, void **l);
	int GetHttp(struct mb_interpreter_t *mb, void **l);
	static int PostHttpWrapper(struct mb_interpreter_t *mb, void **l);
	int PostHttp(struct mb_interpreter_t *mb, void **l);
	static int HeapMemWrapper(struct mb_interpreter_t *mb, void **l);
	int HeapMem(struct mb_interpreter_t *mb, void **l);
	static int TemperatureWrapper(struct mb_interpreter_t *mb, void **l);
	int Temperature(struct mb_interpreter_t *mb, void **l);
	static int HallWrapper(struct mb_interpreter_t *mb, void **l);
	int Hall(struct mb_interpreter_t *mb, void **l);
	static int DelayWrapper(struct mb_interpreter_t *mb, void **l);
	int Delay(struct mb_interpreter_t *mb, void **l);
	static int MaxStringLengthWrapper(struct mb_interpreter_t *mb, void **l);
	
	static int OLEDClearWrapper(struct mb_interpreter_t *mb, void **l);
	int OLEDClear(struct mb_interpreter_t *mb, void **l);
	static int OLEDDisplayWrapper(struct mb_interpreter_t *mb, void **l);
	int OLEDDisplayInternal(struct mb_interpreter_t *mb, void **l);
	static int OLEDPutPixelWrapper(struct mb_interpreter_t *mb, void **l);
	int OLEDPutPixel(struct mb_interpreter_t *mb, void **l);
	static int OLEDPrintWrapper(struct mb_interpreter_t *mb, void **l);
	int OLEDPrint(struct mb_interpreter_t *mb, void **l);
	
	int InternalPrintWrapper(const String &text);
	int InternalInputWrapper(const char *prompt, char *input, int len);
	
	friend int MyBasicPrintWrapper(const char *, ...);
	friend int MyBasicInputWrapper(const char *, char *, int);
	
	//void OLEDInit();
	//void OLEDDestroy();
	
	void RunInternal();
	
	const bool RunThreaded();
	const bool RunAsTask();
	
};

extern MyBasic *mybasic_ptr;
extern int mybasic_maxstringlength;

int MyBasicPrintWrapper(const char *, ...);
int MyBasicInputWrapper(const char *, char *, int);

const char my_basic_help[] PROGMEM=
"<html>\n"
"<head>\n"
"<style type=\"text/css\">\n"
"body\t{ font-family:Helvetica,San-serif; }\n"
"code\t{ font-weight:bolder; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"\n"
"The official documentation can be found here : <a href=\"http://paladin-t.github.io/my_basic/\">MY-BASIC</a>\n"
"<br/>\n"
"In addition to the built in functions found in MY-BASIC, the following function have been added.  Parameters within [] are optional.  Do no include the [] when using the optional parameters.\n"
"<table>\n"
"<tr>\n"
"<th colspan=\"3\">Extended Functions</th>\n"
"</tr>\n"
"<tr>\n"
"<td><code>GETHTTP(\"http://www.example.com\"[,maxlength])</code></td>\n"
"<td>Returns the content of a http/https url.  The output includes any headers and detail that were sent from the server.  The URL must be http or https.  http will default to port 80 and https to port 443.  You may add :port to the host name if you need to connect to a different port.  Please be aware of the limited memory available, so requesting large pages might make the device run out of memory and reboot.  The maxlength parameter is optional, and will limit the length of the output to the specified size.  The maximum returned length will still be limited to the maximum string length regardless of this parameter</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>POSTHTTP(\"http://www.example.com\",\"var1=value1&var2=value2\"[,maxlength])</code></td>\n"
"<td>Posts data to a http/https url.  The second parameter is the url-encoded data that will be posted to the server.  See GETHTTP for a general description of the other parameters.</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>HEAPMEM</code></td>\n"
"<td>Returns the available heap memory of the device</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>TEMERATURE</code></td>\n"
"<td>Returns the CPU temperature in Celcius</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>HALL</code></td>\n"
"<td>Returns the value of the Hall sensor</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>DELAY(10)</code></td>\n"
"<td>Delays execution for the specified number of milliseconds</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>MAXSTRINGLENGTH[(newmaxlength)]</code></td>\n"
"<td>Returns and/or set the maximum string length.  The default can be configured when building the image for the device.  Be careful not to set this too high, or the device may run out of memory when working with large strings.  If changing the default is necessary, you should do so once near the start of your program.</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>OLEDCLEAR</code></td>\n"
"<td>Clears the buffer of the OLED display</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>OLEDDISPLAY</code></td>\n"
"<td>Displays the buffer of the OLED</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>OLEDPUTPIXEL(x,y)</code></td>\n"
"<td>Draws a pixel at the coordinates on the OLED buffer</td>\n"
"</tr>\n"
"<tr>\n"
"<td><code>OLEDPRINT(\"string\",x,y[,size])</code></td>\n"
"<td>Prints a string on the OLED buffer.  Size must be 24, 16, or 10.</td>\n"
"</tr>\n"
"</table>\n"
"\n"
"<table>\n"
"<tr><td>Other Notes</td></tr>\n"
"<tr><td>The Stop button will send a http request to the device to stop the code being executed.  However the single threaded nature of the web server may mean another request is being processed and the stop won't be handled.  In that case you may need to click Stop multiple times before it is successfully handled.</td></tr>\n"
"<tr><td>IMPORT will work as long as you save the basic file to the root of the SPIFFS filesystem on the device.  Other filesystem drivers may also be used in place of SPIFFS, and must be included when compiling the image for the device.</td></tr>\n"
"</table>\n"
"\n"
"</body>\n"
"</html>";

const char my_basic_html[] PROGMEM=
"<html>\n"
"<head>\n"
"<title>MY-BASIC</title>\n"
"<style type=\"text/css\">\n"
"body\t{ font-family:Helvetica,San-serif; }\n"
"div\t\t{ padding:0px; margin:0px; }\n"
"#error\t{ color:red; }\n"
"</style>\n"
"<script type=\"text/javascript\">\n"
"\n"
"var statustimer;\n"
"\n"
"var ajax = {};\n"
"ajax.x = function () {\n"
"    if (typeof XMLHttpRequest !== 'undefined') {\n"
"        return new XMLHttpRequest();\n"
"    }\n"
"    var versions = [\n"
"        \"MSXML2.XmlHttp.6.0\",\n"
"        \"MSXML2.XmlHttp.5.0\",\n"
"        \"MSXML2.XmlHttp.4.0\",\n"
"        \"MSXML2.XmlHttp.3.0\",\n"
"        \"MSXML2.XmlHttp.2.0\",\n"
"        \"Microsoft.XmlHttp\"\n"
"    ];\n"
"\n"
"    var xhr;\n"
"    for (var i = 0; i < versions.length; i++) {\n"
"        try {\n"
"            xhr = new ActiveXObject(versions[i]);\n"
"            break;\n"
"        } catch (e) {\n"
"        }\n"
"    }\n"
"    return xhr;\n"
"};\n"
"\n"
"ajax.send = function (url, callback, method, data, async) {\n"
"    if (async === undefined) {\n"
"        async = true;\n"
"    }\n"
"    var x = ajax.x();\n"
"    x.open(method, url, async);\n"
"    x.onreadystatechange = function () {\n"
"        if (x.readyState == 4) {\n"
"            callback(x.responseText)\n"
"        }\n"
"    };\n"
"    if (method == 'POST') {\n"
"        x.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n"
"    }\n"
"    x.send(data)\n"
"};\n"
"\n"
"ajax.get = function (url, data, callback, async) {\n"
"    var query = [];\n"
"    for (var key in data) {\n"
"        query.push(encodeURIComponent(key) + '=' + encodeURIComponent(data[key]));\n"
"    }\n"
"    ajax.send(url + (query.length ? '?' + query.join('&') : ''), callback, 'GET', null, async)\n"
"};\n"
"\n"
"ajax.post = function (url, data, callback, async) {\n"
"    var query = [];\n"
"    for (var key in data) {\n"
"        query.push(encodeURIComponent(key) + '=' + encodeURIComponent(data[key]));\n"
"    }\n"
"    ajax.send(url, callback, 'POST', query.join('&'), async)\n"
"};\n"
"\n"
"function startstatustimer()\n"
"{\n"
"\tstatustimer=setInterval(function(){ajax.get(window.location.href+\"/status\",{},updatestatus);},1000);\n"
"}\n"
"\n"
"function run()\n"
"{\n"
"\tvar codestr=document.getElementById('code').value;\n"
"\t//var threaded=document.getElementById('threaded').checked;\n"
"\t\n"
"\t/*\n"
"\tif(threaded)\n"
"\t{\n"
"\n"
"\t}\n"
"\telse\n"
"\t{\n"
"\t\t// input won't work\n"
"\t\tif(codestr.indexOf('input')>-1 || codestr.indexOf('INPUT')>-1)\n"
"\t\t{\n"
"\t\t\tif(confirm('INPUT will cause the device to freeze in non-threaded mode.  Do you want to continue anyway?')==false)\n"
"\t\t\t{\n"
"\t\t\t\treturn;\n"
"\t\t\t}\n"
"\t\t}\n"
"\t}\n"
"\t*/\n"
"\t\n"
"\tdocument.getElementById('output').value='';\n"
"\tdocument.getElementById('status').textContent='';\n"
"\tdocument.getElementById('error').textContent='';\n"
"\tdocument.getElementById('prompt').textContent='';\n"
"\t//ajax.post(window.location.href+\"/start\",{code: codestr, threaded: (threaded ? 'Y' : 'N')},function() {});\n"
"\tajax.post(window.location.href+\"/start\",{code: codestr, threaded: 'N'},function() {});\n"
"\tdocument.getElementById('run').disabled=true;\n"
"\tdocument.getElementById('stop').disabled=false;\n"
"\tclearInterval(statustimer);\n"
"\tstartstatustimer();\n"
"}\n"
"\n"
"function stop()\n"
"{\n"
"\tajax.post(window.location.href+\"/stop\",{},function() {});\n"
"\tclearInterval(statustimer);\n"
"\tstartstatustimer();\n"
"}\n"
"\n"
"function sendinput()\n"
"{\n"
"\tif(document.getElementById(\"prompt\").textContent!='')\n"
"\t{\n"
"\t\tdocument.getElementById(\"output\").value+='\\n';\n"
"\t}\n"
"\n"
"\tvar inval=document.getElementById(\"input\").value;\n"
"\tdocument.getElementById(\"input\").value=\"\";\n"
"\tdocument.getElementById(\"input\").readOnly=true;\n"
"\tdocument.getElementById(\"sendinput\").disabled=true;\n"
"\tdocument.getElementById(\"prompt\").textContent=\"\";\n"
"\t\n"
"\tajax.post(window.location.href+\"/input\",{val: inval},function() {});\n"
"\t\n"
"\tstartstatustimer();\n"
"}\n"
"\n"
"function disableinput()\n"
"{\n"
"\tdocument.getElementById(\"input\").value=\"\";\n"
"\tdocument.getElementById(\"input\").readOnly=true;\n"
"\tdocument.getElementById(\"sendinput\").disabled=true;\n"
"\tdocument.getElementById(\"prompt\").textContent=\"\";\n"
"}\n"
"\n"
"function updatestatus(response)\n"
"{\n"
"\tif(response.length>0)\n"
"\t{\n"
"\t\tvar j=JSON.parse(response);\n"
"\t\tdocument.getElementById(\"status\").textContent=j[\"status\"];\n"
"\t\tdocument.getElementById(\"error\").textContent=j[\"error\"];\n"
"\t\tdocument.getElementById(\"output\").value+=j[\"output\"].join(\"\").replace(\"\\r\\n\",\"\\n\");\n"
"\t\tif(document.getElementById(\"autoscroll\").checked && j[\"output\"].length>0)\n"
"\t\t{\n"
"\t\t\tvar elm=document.getElementById(\"output\");\n"
"\t\t\tvar bottom=function()\n"
"\t\t\t{\n"
"\t\t\t\telm.scrollTop=elm.scrollHeight;\n"
"\t\t\t}\n"
"\t\t\tsetTimeout(bottom,0);\n"
"\t\t}\n"
"\t\tdocument.getElementById(\"prompt\").textContent=j[\"prompt\"];\n"
"\t\t\n"
"\t\tif(j[\"status\"]==\"STOPPED\")\n"
"\t\t{\n"
"\t\t\tclearInterval(statustimer);\n"
"\t\t\tdisableinput();\n"
"\t\t\tdocument.getElementById(\"stop\").disabled=true;\n"
"\t\t\tdocument.getElementById(\"run\").disabled=false;\n"
"\t\t}\n"
"\t\telse if(j[\"status\"]==\"NEED INPUT\")\n"
"\t\t{\n"
"\t\t\tclearInterval(statustimer);\n"
"\t\t\tdocument.getElementById(\"input\").readOnly=false;\n"
"\t\t\tdocument.getElementById(\"sendinput\").disabled=false;\n"
"\t\t}\n"
"\t\telse if(j[\"status\"]==\"RUNNING\")\n"
"\t\t{\n"
"\t\t\tdisableinput();\n"
"\t\t\tdocument.getElementById(\"stop\").disabled=false;\n"
"\t\t\tdocument.getElementById(\"run\").disabled=true;\n"
"\t\t}\n"
"\t}\n"
"}\n"
"\n"
"</script>\n"
"</head>\n"
"<body>\n"
"<div width=\"100%\">\n"
"<span><a href=\"http://paladin-t.github.io/my_basic/\">MY-BASIC</a> interpreter</span><span style=\"float:right;\"><a href=\"<!--{ROOTPATH}-->/help\">Help</a></span>\n"
"</div>\n"
"<div style=\"display:inline-block;width:49%;height:95%;vertical-align:top;\"><textarea id=\"code\" placeholder=\"BASIC Code\" style=\"width:100%;height:90%\"></textarea>\n"
"<br />\n"
"<!--<input type=\"checkbox\" id=\"threaded\" title=\"Check to run in background thread.  This could be unstable, but will allow input.\">Run in thread --><button id=\"run\" onclick=\"run();\">Run</button><button id=\"stop\" disabled onclick=\"stop();\">Stop</button> <span id=\"status\"></span><br /><span id=\"error\"></span></div>\n"
"\n"
"<div style=\"display:inline-block;width:49%;height:95%;vertical-align:top;\"><textarea id=\"output\" readonly style=\"width:100%;height:90%\" placeholder=\"Output\"></textarea>\n"
"<br />\n"
"<span id=\"prompt\"></span><input type=\"text\" id=\"input\" readonly><button id=\"sendinput\" disabled onclick=\"sendinput();\">Send Input</button>\n"
"<input type=\"checkbox\" id=\"autoscroll\"> Autoscroll Output\n"
"</div>\n"
"\n"
"</body>\n"
"</html>";

}	// namespace MyBasicESP

#endif	// _mybasicesp_
