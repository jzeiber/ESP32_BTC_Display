#include "mybasicesp.h"
#include "simplejson.h"

#include <cstdlib>

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <SSD1306.h>

namespace MyBasicESP
{

//static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

MyBasic *mybasic_ptr=nullptr;
int mybasic_maxstringlength=10240;

int MyBasicPrintWrapper(const char *fmt, ...)
{
	if(mybasic_ptr)
	{
		int rval=0;
		va_list l;
		va_start(l,fmt);
		
		// vsnprintf was very unstable when running as a thread
		if(mybasic_ptr->RunThreaded()==false)
		{
			va_list args2;
			va_copy(args2,l);
			int len=vsnprintf(NULL,0,fmt,args2)+1;
			va_end(args2);
			
			if(len>0 && mybasic_maxstringlength>-1 && len>mybasic_maxstringlength)
			{
				len=mybasic_maxstringlength;
			}
		
			if(len>0)
			{
				char *buff=new char[len];
				if(buff)
				{
					int outlen=vsnprintf(buff,len,fmt,l);
					buff[outlen]='\0';
					String text(buff);
					rval=mybasic_ptr->InternalPrintWrapper(text);
					delete [] buff;
				}
			}
		}
		else
		{
			String fmtstr("");
			if(fmt)
			{
				String fmtstr(fmt);
				if(fmtstr==String(MB_INT_FMT))
				{
					// get int arg and print
					int arg=va_arg(l,int);
					rval=mybasic_ptr->InternalPrintWrapper(String(arg));
				}
				else if(fmtstr==String(MB_REAL_FMT))
				{
					// get double/float arg and print
					real_t arg=va_arg(l,real_t);
					rval=mybasic_ptr->InternalPrintWrapper(String(arg));
				}
				else if(fmtstr=="%s")
				{
					// get string arg and print
					char *arg=va_arg(l,char *);
					if(arg && arg[0]!='\0')
					{
						rval=mybasic_ptr->InternalPrintWrapper(String(arg));
					}
				}
				else if(fmtstr=="%ls")	// wide char - convert to multibyte
				{
					wchar_t *arg=va_arg(l,wchar_t *);
					if(arg && arg[0]!='\0')
					{
						const size_t csize=wcstombs(NULL,arg,0);
						if(csize>0)
						{
							char *cp=nullptr;
							cp=new char[csize+1];
							wcstombs(cp,arg,csize);
							cp[csize]='\0';
							mybasic_ptr->InternalPrintWrapper(String((char *)arg));
							rval=csize;
							delete [] cp;
						}
					}
				}
				else
				{
					rval=mybasic_ptr->InternalPrintWrapper(fmtstr);
				}
			}
		}
		va_end(l);
		return rval;
	}
	return 0;
}

int MyBasicInputWrapper(const char *prompt, char *inp, int len)
{
	if(mybasic_ptr)
	{
		return mybasic_ptr->InternalInputWrapper(prompt,inp,len);
	}
	return 0;
}

SPIFFSImportHandler::SPIFFSImportHandler()
{

}

SPIFFSImportHandler::~SPIFFSImportHandler()
{

}

const bool SPIFFSImportHandler::Import(const String &name, String &code)
{

	String fname=name;
	if(fname.length()>0 && fname.charAt(0)!='/')
	{
		fname="/"+fname;
	}

	File f=SPIFFS.open(fname,"r");
	if(!f)
	{
		return false;
	}

	size_t size=f.size();
	code="";
	code.reserve(size);
	code=f.readString();
	
	f.close();

	return true;
}

MyBasic::MyBasic():
m_basic(nullptr),
m_printer(nullptr),
m_debugprinter(nullptr),
m_importhandler(nullptr),
m_baseuri(""),
m_web(nullptr),
m_status(MYBASIC_STOPPED),
m_wantstop(false),
m_inputprompt(""),
m_error(""),
m_runthreaded(false),
m_debugprint(false),
m_maximumprintcache(-1)/*,
m_usingoled(false),
m_display(nullptr),
m_oledaddress(0),
m_oledsda(0),
m_oledscl(0)*/
{
	mb_init();
	mybasic_ptr=this;
}

MyBasic::~MyBasic()
{
	Stop();
	if(m_basic)
	{
		mb_close(&m_basic);
	}
	mb_dispose();
}

void MyBasic::SetPrinter(Print &printer)
{
	m_printer=&printer;
}

void MyBasic::SetDebugPrinter(Print &debugprinter)
{
	m_debugprinter=&debugprinter;
}

void MyBasic::DebugPrint(const bool debugprint)
{
	m_debugprint=debugprint;
}

void MyBasic::SetImportHandler(ImportHandler &importhandler)
{
	m_importhandler=&importhandler;
}

/*
void MyBasic::SetOLEDDisplay(OLEDDisplay *display)
{
	m_display=display;
}

void MyBasic::SetOLEDParameters(uint8_t address, uint8_t sda, uint8_t scl)
{
	m_oledaddress=address;
	m_oledsda=sda;
	m_oledscl=scl;
}
*/

void MyBasic::SetWebHandler(WebServer &web, const String &baseuri)
{
	m_web=&web;
	m_baseuri=baseuri;
	if(m_web)
	{
		m_web->addHandler(this);
	}
}

bool MyBasic::canHandle(HTTPMethod method, String uri)
{
	return uri.startsWith(m_baseuri);
}

bool MyBasic::canUpload(String uri)
{
	return false;
}

bool MyBasic::handle(WebServer& server, HTTPMethod requestMethod, String requestUri)
{

	if(!canHandle(requestMethod,requestUri))
	{
		return false;
	}
	
	ExternalDebugPrintLn("MyBasic::handle handling requestUri="+requestUri);
	
	if(requestUri==m_baseuri)
	{
		// send html for basic input
		String html(FPSTR(my_basic_html));
		html.replace("<!--{ROOTPATH}-->",m_baseuri);
		server.send(200,"text/html",html);
		return true;
	}
	else if(requestUri==m_baseuri+String(F("/help")))
	{
		server.send(200,"text/html",my_basic_help);
		return true;
	}
	else if(requestUri==m_baseuri+String(F("/start")))
	{
		// start interpreter
		if(server.hasArg("code"))
		{
			bool threaded=false;
			if(server.hasArg("threaded") && server.arg("threaded")=="Y")
			{
				threaded=true;
			}
			ExternalDebugPrintLn("MyBasic::handle getting code");
			String code(server.arg("code"));
			ExternalDebugPrintLn("MyBasic::handle running");
			//Run(code,threaded);
			RunTask(code);
			ExternalDebugPrintLn("MyBasic::handle sending back 200");
			server.send(200,"text/plain","started");
			return true;
		}
	}
	else if(requestUri==m_baseuri+String(F("/stop")))
	{
		// stop interpreter
		Stop();
		server.send(200,"text/plain","stopping");
		return true;
	}
	else if(requestUri==m_baseuri+String(F("/status")))
	{
		// send status json
		SimpleJSON::JSONContainer statobj;

		statobj["prompt"]="";
		switch(GetStatus())
		{
		case MYBASIC_STOPPED:
			statobj["status"]="STOPPED";
			break;
		case MYBASIC_RUNNING:
			statobj["status"]="RUNNING";
			break;
		case MYBASIC_REQUIREINPUT:
			statobj["status"]="NEED INPUT";
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				statobj["prompt"]=m_inputprompt;
			}
			break;
		default:
			statobj["status"]="UNKNOWN";
			break;
		}

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			statobj["output"]=m_output;
			m_output.clear();
			statobj["error"]=m_error;
		}

		server.send(200,F("application/json"),statobj.Encode());
	}
	else if(requestUri==m_baseuri+String(F("/input")))
	{
		// handle new input
		if(server.hasArg("val"))
		{
			ExternalDebugPrintLn("MyBasic::handle got input "+server.arg("val"));
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_input.push_back(server.arg("val"));
			}
			server.send(200,"text/plain","got input");
			return true;
		}
	}
	return false;
}

void MyBasic::upload(WebServer& server, String requestUri, HTTPUpload& upload)
{
	return;
}

void MyBasic::ExternalDebugPrint(const String &str)
{
	if(m_debugprint && m_debugprinter)
	{
		m_debugprinter->print(str);
	}
}

void MyBasic::ExternalDebugPrintLn(const String &str)
{
	if(m_debugprint && m_debugprinter)
	{
		m_debugprinter->println(str);
	}
}

void MyBasic::ExternalPrint(const String &str)
{
	if(m_printer)
	{
		m_printer->print(str);
	}
}

void MyBasic::ExternalPrintLn(const String &str)
{
	if(m_printer)
	{
		m_printer->println(str);
	}
}

int MyBasic::InternalPrintWrapper(const String &text)
{
	int outputlength=0;
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_output.push_back(text);
		outputlength=m_output.size();
	}
	
	while(m_maximumprintcache>-1 && WantStop()==false && outputlength>m_maximumprintcache)
	{
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			outputlength=m_output.size();
		}
		if(outputlength>m_maximumprintcache)
		{
			delay(5);
		}
	}
	
	ExternalPrint(text);
	//std::this_thread::yield();
	//std::this_thread::sleep_for(std::chrono::milliseconds(50));
	//taskYIELD();
	return text.length();
}

int MyBasic::InternalInputWrapper(const char *prompt, char *input, int len)
{
	ExternalDebugPrintLn("MyBasic::InternalInputWrapper top");
	
	// can't get input in single threaded mode
	if(RunThreaded()==false && RunAsTask()==false)
	{
		if(input && len>0)
		{
			input[0]='\0';
			return 0;
		}
		return 0;
	}
	
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_status=MYBASIC_REQUIREINPUT;
		if(prompt && prompt[0]!='\0')
		{
			// m_inputprompt=String(prompt) was crashing
			String inp(prompt);
			m_inputprompt=inp;
		}
		else
		{
			m_inputprompt="";
		}
	}
	int val=-1;
	try
	{
		while(val==-1 && WantStop()==false)
		{
			if(m_mutex.try_lock()==true)
			{
				if(m_input.size()>0)
				{
					const int sz=(len<m_input[0].length()) ? len : m_input[0].length();
					strncpy(input,m_input[0].c_str(),sz);
					if(sz<len)
					{
						input[sz]='\0';
					}
					val=sz;
					m_input.erase(m_input.begin());
					m_status=MYBASIC_RUNNING;
				}
				m_mutex.unlock();
			}
			if(val==-1)
			{
				//std::this_thread::yield();
				//std::this_thread::sleep_for(std::chrono::milliseconds(50));
				//taskYIELD();
				vTaskDelay(50);
			}
		}
		if(WantStop()==true)
		{
			val=0;
			if(len>0)
			{
				input[0]='\0';
			}
		}
	}
	catch(std::exception &e)
	{
		ExternalDebugPrintLn("MyBasic::InternalInputWrapper caught exception "+String(e.what()));
		val=0;
	}
	return val;
}

const uint8_t MyBasic::GetStatus()
{
	std::lock_guard<std::mutex> guard(m_mutex);
	return m_status;
}

const bool MyBasic::WantStop()
{
	std::lock_guard<std::mutex> guard(m_mutex);
	return m_wantstop;
}

void MyBasic::Run(String &code, const bool newthread)
{
	Stop();
	m_code=code;
	if(newthread==true)
	{
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_runthreaded=true;
		}
		ExternalDebugPrintLn("MyBasic::Run starting thread");
		std::thread t=std::thread(&MyBasic::RunInternal,this);
		ExternalDebugPrintLn("MyBasic::Run detaching thread");
		t.detach();
		ExternalDebugPrintLn(String(F("MyBasic::Run started code\n"))+code);
	}
	else
	{
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_runthreaded=false;
		}
		ExternalDebugPrintLn("MyBasic::Run starting non-threaded");
		RunInternal();
		ExternalDebugPrintLn("MyBasic::Run completed non-threaded");
	}
}

void MyBasic::RunTask(String &code)
{
	Stop();
	m_code=code;
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_runtask=true;
	}
	TaskHandle_t handle = NULL;

	xTaskCreate(MyBasic::RunThreadWrapper,"my_basic",10240,this,tskIDLE_PRIORITY,&handle);
	
}

void MyBasic::Update()
{
	if(RunThreaded()==true)
	{
		if(WaitingFor()==WAITING_IMPORT)
		{
			struct mb_interpreter_t *mb=nullptr;
			const char *name=nullptr;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				mb=m_paramdata.m_basic;
				name=m_paramdata.m_pcchar;
			}
			
			int rval=InternalImportHandler(mb,name);
			
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_paramdata.m_result=rval;
				m_waitingfor=WAITING_NONE;
			}
		}
		else if(WaitingFor()==WAITING_GETHTTP)
		{
			struct mb_interpreter_t *mb=nullptr;
			void **l=nullptr;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				mb=m_paramdata.m_basic;
				l=m_paramdata.m_ppvoid;
			}
			
			int rval=GetHttp(mb,l);
			
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_paramdata.m_result=rval;
				m_waitingfor=WAITING_NONE;
			}
		}
		else if(WaitingFor()==WAITING_POSTHTTP)
		{
			struct mb_interpreter_t *mb=nullptr;
			void **l=nullptr;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				mb=m_paramdata.m_basic;
				l=m_paramdata.m_ppvoid;
			}
			
			int rval=PostHttp(mb,l);
			
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_paramdata.m_result=rval;
				m_waitingfor=WAITING_NONE;
			}
		}
		else if(WaitingFor()==WAITING_HEAPMEM)
		{
			struct mb_interpreter_t *mb=nullptr;
			void **l=nullptr;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				mb=m_paramdata.m_basic;
				l=m_paramdata.m_ppvoid;
			}
			
			int rval=HeapMem(mb,l);
			
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_paramdata.m_result=rval;
				m_waitingfor=WAITING_NONE;
			}
		}
		else if(WaitingFor()==WAITING_TEMPERATURE)
		{
			struct mb_interpreter_t *mb=nullptr;
			void **l=nullptr;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				mb=m_paramdata.m_basic;
				l=m_paramdata.m_ppvoid;
			}
			
			int rval=Temperature(mb,l);
			
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_paramdata.m_result=rval;
				m_waitingfor=WAITING_NONE;
			}
		}
		else if(WaitingFor()==WAITING_HALL)
		{
			struct mb_interpreter_t *mb=nullptr;
			void **l=nullptr;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				mb=m_paramdata.m_basic;
				l=m_paramdata.m_ppvoid;
			}
			
			int rval=Hall(mb,l);
			
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_paramdata.m_result=rval;
				m_waitingfor=WAITING_NONE;
			}
		}
		else if(WaitingFor()==WAITING_DELAY)
		{
			struct mb_interpreter_t *mb=nullptr;
			void **l=nullptr;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				mb=m_paramdata.m_basic;
				l=m_paramdata.m_ppvoid;
			}
			
			int rval=Delay(mb,l);
			
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_paramdata.m_result=rval;
				m_waitingfor=WAITING_NONE;
			}
		}
	}
}

void MyBasic::Stop()
{
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_wantstop=true;
	}
	while(GetStatus()!=MYBASIC_STOPPED)
	{
		//std::this_thread::yield();
		//std::this_thread::sleep_for(std::chrono::milliseconds(50));
		//taskYIELD();
		vTaskDelay(50);
	}
}

void MyBasic::RunInternal()
{
	try
	{
		ExternalDebugPrintLn("MyBasic::RunInternal starting");
		ExternalDebugPrintLn("MyBasic::RunInternal free memory "+String(ESP.getFreeHeap()));
		mb_open(&m_basic);
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_input.clear();
			m_output.clear();
			m_error="";
			//m_usingoled=true;
		}
		//OLEDInit();
		ExternalDebugPrintLn("MyBasic::RunInternal setting mb data");
		m_waitingfor=WAITING_NONE;
		mb_set_userdata(m_basic,this);
		mb_set_printer(m_basic,MyBasicPrintWrapper);
		mb_set_inputer(m_basic,MyBasicInputWrapper);
		mb_debug_set_stepped_handler(m_basic,MyBasic::SteppedHandlerWrapper);
		mb_set_error_handler(m_basic,MyBasic::ErrorHandlerWrapper);
		mb_set_import_handler(m_basic,MyBasic::ImportHandlerWrapper);
		mb_register_func(m_basic,"GETHTTP",MyBasic::GetHttpWrapper);
		mb_register_func(m_basic,"POSTHTTP",MyBasic::PostHttpWrapper);
		mb_register_func(m_basic,"HEAPMEM",MyBasic::HeapMemWrapper);
		mb_register_func(m_basic,"TEMPERATURE",MyBasic::TemperatureWrapper);
		mb_register_func(m_basic,"HALL",MyBasic::HallWrapper);
		mb_register_func(m_basic,"DELAY",MyBasic::DelayWrapper);
		mb_register_func(m_basic,"MAXSTRINGLENGTH",MyBasic::MaxStringLengthWrapper);
		//mb_register_func(m_basic,"OLEDCLEAR",MyBasic::OLEDClearWrapper);
		//mb_register_func(m_basic,"OLEDDISPLAY",MyBasic::OLEDDisplayWrapper);
		//mb_register_func(m_basic,"OLEDPUTPIXEL",MyBasic::OLEDPutPixelWrapper);
		//mb_register_func(m_basic,"OLEDPRINT",MyBasic::OLEDPrintWrapper);
		
		ExternalDebugPrintLn("MyBasic::RunInternal loading code");
		mb_load_string(m_basic,m_code.c_str(),false);
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_wantstop=false;
			m_status=MYBASIC_RUNNING;
		}
		ExternalDebugPrintLn("MyBasic::RunInternal running code");
		mb_run(m_basic,false);
		
		ExternalDebugPrintLn("MyBasic::RunInternal closing mb");
		mb_close(&m_basic);
		
		//OLEDDestroy();
		
		m_basic=nullptr;
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_wantstop=false;
			m_status=MYBASIC_STOPPED;
			//m_usingoled=false;
		}
	}
	catch(std::exception &e)
	{
		ExternalDebugPrintLn("MyBasic::RunInternal caught exception "+String(e.what()));
		m_status=MYBASIC_STOPPED;
	}

}

void MyBasic::RunThreadWrapper(void *mb)
{
	if(mb)
	{
		((MyBasic *)mb)->RunInternal();
		if(((MyBasic *)mb)->RunAsTask()==true)
		{
			((MyBasic *)mb)->ExternalDebugPrintLn("MyBasic::RunThreadWrapper stack high water mark "+String(uxTaskGetStackHighWaterMark(NULL)));
			vTaskDelete(NULL);
		}
	}

}

const bool MyBasic::RunThreaded()
{
	std::lock_guard<std::mutex> guard(m_mutex);
	return m_runthreaded;
}

const bool MyBasic::RunAsTask()
{
	std::lock_guard<std::mutex> guard(m_mutex);
	return m_runtask;
}

const int MyBasic::WaitingFor()
{
	std::lock_guard<std::mutex> guard(m_mutex);
	return m_waitingfor;
}

/*
const bool MyBasic::UsingOLED()
{
	//std::lock_guard<std::mutex> guard(m_mutex);
	return m_usingoled;
}
*/

void MyBasic::SetMaximumPrintCache(const int lines)
{
	std::lock_guard<std::mutex> guard(m_mutex);
	m_maximumprintcache=lines;
}

void MyBasic::SetMaxStringLength(const int maxlength)
{
	mybasic_maxstringlength=maxlength;
}

int MyBasic::SteppedHandlerWrapper(struct mb_interpreter_t *mb, void **ast, const char *sourcefile, int sourcepos, unsigned short sourcerow, unsigned short sourcecol)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			return ((MyBasic *)ud)->SteppedHandler(mb,ast,sourcefile,sourcepos,sourcerow,sourcecol);
		}
	}
	return MB_FUNC_SUSPEND;
}

int MyBasic::SteppedHandler(struct mb_interpreter_t *mb, void **ast, const char *sourcefile, int sourcepos, unsigned short sourcerow, unsigned short sourcecol)
{
	ExternalDebugPrintLn("MyBasic::SteppedHandler stepped "+String(sourcepos)+" ("+String(sourcerow)+","+String(sourcecol)+")   Free Heap "+String(ESP.getFreeHeap()));
	//std::this_thread::yield();
	//std::this_thread::sleep_for(std::chrono::milliseconds(0));
	//taskYIELD();
	vTaskDelay(1);
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_status=MYBASIC_RUNNING;
		if(m_wantstop)
		{
			return MB_FUNC_SUSPEND;
		}
	}
	return MB_FUNC_OK;
}

void MyBasic::ErrorHandlerWrapper(struct mb_interpreter_t *mb, mb_error_e e, const char *m, const char *f, int p, unsigned short row, unsigned short col, int abort_code)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			((MyBasic *)ud)->ErrorHandler(mb,e,m,f,p,row,col,abort_code);
		}
	}
}

void MyBasic::ErrorHandler(struct mb_interpreter_t *mb, mb_error_e e, const char *m, const char *f, int p, unsigned short row, unsigned short col, int abort_code)
{
	String err("");
	if(e!=SE_NO_ERR)
	{
		String message("");
		if(m && m[0]!='\0')
		{
			message=String(m);
		}
		
		if(f)
		{
			String fstr("");
			if(f && f[0]!='\0')
			{
				fstr=String(f);
			}
			if(e==SE_RN_WRONG_FUNCTION_REACHED)
			{

				err+="Error: Ln "+String(row)+", Col "+String(col)+" in Func "+fstr+" Code "+String(e)+", Abort Code "+String((e==SE_EA_EXTENDED_ABORT ? abort_code-MB_EXTENDED_ABORT : abort_code))+" Message: "+message;
			}
			else
			{
				err+="Error: Ln "+String(row)+", Col "+String(col)+" in File "+fstr+" Code "+String(e)+", Abort Code "+String((e==SE_EA_EXTENDED_ABORT ? abort_code-MB_EXTENDED_ABORT : abort_code))+" Message: "+message;
			}
		}
		else
		{
			err+="Error: Ln "+String(row)+", Col "+String(col)+" Code "+String(e)+", Abort Code "+String((e==SE_EA_EXTENDED_ABORT ? abort_code-MB_EXTENDED_ABORT : abort_code))+" Message: "+message;
		}
	}
	ExternalDebugPrintLn("MyBasic::ErrorHandler err "+err);
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_error=err;
	}
}

int MyBasic::ImportHandlerWrapper(struct mb_interpreter_t *mb, const char *name)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				MyBasic *c=((MyBasic *)ud);
				c->ExternalDebugPrintLn("MyBasic::ImportHandlerWrapper import wrapper in threaded mode");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					c->m_paramdata.m_basic=mb;
					c->m_paramdata.m_pcchar=name;
					c->m_waitingfor=WAITING_IMPORT;
				}
				
				c->ExternalDebugPrintLn("MyBasic::ImportHandlerWrapper import wrapper waiting for import");
				while(c->WaitingFor()==WAITING_IMPORT)
				{
					//std::this_thread::yield();
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					//taskYIELD();
					vTaskDelay(10);
				}
				
				c->ExternalDebugPrintLn("MyBasic::ImportHandlerWrapper import wrapper got result");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					return c->m_paramdata.m_result;
				}
			}
			else
			{
				return ((MyBasic *)ud)->InternalImportHandler(mb,name);
			}
		}
	}
	return MB_FUNC_ERR;
}

int MyBasic::InternalImportHandler(struct mb_interpreter_t *mb, const char *name)
{
	if(name && name[0]!='\0' && m_importhandler)
	{
		String code("");
		String fname(name);
		ExternalDebugPrintLn("MyBasic::ImportHandler importing from "+fname);
		bool imported=m_importhandler->Import(fname,code);
		if(imported)
		{
			ExternalDebugPrintLn("MyBasic::ImportHandler successfully read code from "+fname);
			int rval=mb_load_string(mb,code.c_str(),true);
			if(rval==MB_FUNC_OK)
			{
				return MB_FUNC_OK;
			}
		}
	}
	return MB_FUNC_ERR;
}

const bool MyBasic::GetHttpInfo(const String &url, String &method, bool &secure, String &host, String &port, String &path)
{
	if(url.indexOf("https://")==0 || url.indexOf("HTTPS://")==0)
	{
		method="https://";
		secure=true;
		port=443;
	}
	else if(url.indexOf("http://")==0 || url.indexOf("HTTP://")==0)
	{
		method="http://";
		secure=false;
		port=80;
	}
	else
	{
		return false;
	}
	
	int dspos=url.indexOf("://");
	int spos=0;
	if(dspos>0)
	{
		spos=url.indexOf("/",dspos+3);
		if(spos>dspos)
		{
			host=url.substring(dspos+3,spos);
			path=url.substring(spos);
		}
		else
		{
			host=url.substring(dspos+3);
		}
		int cpos=host.indexOf(":");
		if(cpos>-1)
		{
			port=host.substring(cpos+1);
			host=host.substring(0,cpos);
		}
	}
	
	return (host.length()>0 && path.length()>0);

}

int MyBasic::GetHttpWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				MyBasic *c=((MyBasic *)ud);
				c->ExternalDebugPrintLn("MyBasic::GetHttpWrapper wrapper in threaded mode");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					c->m_paramdata.m_basic=mb;
					c->m_paramdata.m_ppvoid=l;
					c->m_waitingfor=WAITING_GETHTTP;
				}
				
				c->ExternalDebugPrintLn("MyBasic::GetHttpWrapper wrapper waiting gethttp");
				while(c->WaitingFor()==WAITING_GETHTTP)
				{
					//std::this_thread::yield();
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					//taskYIELD();
					vTaskDelay(10);
				}
				
				c->ExternalDebugPrintLn("MyBasic::GetHttpWrapper wrapper got result");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					return c->m_paramdata.m_result;
				}
			}
			else
			{
				return ((MyBasic *)ud)->GetHttp(mb,l);
			}
			return MB_FUNC_OK;
		}
	}
	return MB_FUNC_ERR;
}

int MyBasic::GetHttp(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	char *arg=0;
	int maxlength=-1;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_open_bracket(mb,l));
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_string(mb,l,&arg));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&maxlength));
	}
	mb_check(mb_attempt_close_bracket(mb,l));
	
	if(arg && arg[0]!='\0')
	{
		bool secure=false;
		String url(arg);
		String host("");
		String method("");
		String port("80");
		String path("/");
		String answer("");

		GetHttpInfo(url,method,secure,host,port,path);
		
		ExternalDebugPrintLn("MyBasic::GetHttp getting url '"+url+"'   host='"+host+"'  port="+port+"  path='"+path+"'");
		
		WiFiClient *client=nullptr;
		if(secure==true)
		{
			client=new WiFiClientSecure;
		}
		else
		{
			client=new WiFiClient;
		}

		if(client)
		{
			if(!client->connect(host.c_str(),port.toInt()))
			{
				ExternalDebugPrintLn("MyBasic::GetHttp couldn't connect to host");
				delete client;
				return MB_FUNC_OK;
			}
			
			ExternalDebugPrintLn("MyBasic::GetHttp connected and sending GET request max length="+String(maxlength));
			client->print("GET "+path+" HTTP/1.1\r\nHost: "+host+"\r\nConnection: close\r\n\r\n");
			ExternalDebugPrintLn("MyBasic::GetHttp waiting for response");
			delay(10);
			while(client->connected() && (maxlength==-1 || answer.length()<maxlength))
			{
				//std::this_thread::yield();
				//std::this_thread::sleep_for(std::chrono::milliseconds(0));
				//taskYIELD();
				vTaskDelay(1);
				answer+=client->readStringUntil('\r');
				if(client->connected())
				{
					answer+='\r';
				}
			}
			client->stop();
			delete client;
			client=nullptr;
			if(maxlength>-1 && answer.length()>maxlength)
			{
				answer.remove(maxlength+1);
			}
		}
		
		ExternalDebugPrintLn("MyBasic::GetHttp waiting got response ("+String(answer.length())+") "+answer);
		
		mb_check(mb_push_string(mb, l, mb_memdup(answer.c_str(), (unsigned)(answer.length() + 1))));
	}
	else
	{
		result=MB_FUNC_ERR;
	}
	
	return result;
	
}

int MyBasic::PostHttpWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				MyBasic *c=((MyBasic *)ud);
				c->ExternalDebugPrintLn("MyBasic::PostHttpWrapper wrapper in threaded mode");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					c->m_paramdata.m_basic=mb;
					c->m_paramdata.m_ppvoid=l;
					c->m_waitingfor=WAITING_POSTHTTP;
				}
				
				c->ExternalDebugPrintLn("MyBasic::PostHttpWrapper wrapper waiting posthttp");
				while(c->WaitingFor()==WAITING_POSTHTTP)
				{
					//std::this_thread::yield();
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					//taskYIELD();
					vTaskDelay(10);
				}
				
				c->ExternalDebugPrintLn("MyBasic::PostHttpWrapper wrapper got result");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					return c->m_paramdata.m_result;
				}
			}
			else
			{
				return ((MyBasic *)ud)->PostHttp(mb,l);
			}
			return MB_FUNC_OK;
		}
	}
	return MB_FUNC_ERR;
}


int MyBasic::PostHttp(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	char *arg0=0;
	char *arg1=0;
	int maxlength=-1;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_open_bracket(mb,l));
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_string(mb,l,&arg0));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_string(mb,l,&arg1));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&maxlength));
	}
	mb_check(mb_attempt_close_bracket(mb,l));
	
	if(arg0 && arg0[0]!='\0' && arg1 && arg1[0]!='\0')
	{
		String postargs(arg1);
		bool secure=false;
		String url(arg0);
		String method("");
		String host("");
		String port("80");
		String path("/");
		String answer("");
		
		GetHttpInfo(url,method,secure,host,port,path);
		
		ExternalDebugPrintLn("MyBasic::PostHttp getting url '"+url+"'   host='"+host+"'  port="+port+"  path='"+path+"'");
		
		WiFiClient *client=nullptr;
		if(secure==true)
		{
			client=new WiFiClientSecure;
		}
		else
		{
			client=new WiFiClient;
		}

		if(client)
		{
			if(!client->connect(host.c_str(),port.toInt()))
			{
				ExternalDebugPrintLn("MyBasic::PostHttp couldn't connect to host");
				delete client;
				return MB_FUNC_ERR;
			}
			
			ExternalDebugPrintLn("MyBasic::PostHttp connected and sending POST request max length="+String(maxlength));
			client->print("POST "+path+" HTTP/1.1\r\nHost: "+host+"\r\nConnection: close\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: "+postargs.length()+"\r\n\r\n");
			client->println(postargs);
			ExternalDebugPrintLn("MyBasic::PostHttp waiting for response");
			delay(10);
			while(client->connected() && (maxlength==-1 || answer.length()<maxlength))
			{
				//std::this_thread::yield();
				//std::this_thread::sleep_for(std::chrono::milliseconds(0));
				//taskYIELD();
				vTaskDelay(1);
				answer+=client->readStringUntil('\r');
				if(client->connected())
				{
					answer+='\r';
				}
			}
			client->stop();
			delete client;
			client=nullptr;
			if(maxlength>-1 && answer.length()>maxlength)
			{
				answer.remove(maxlength+1);
			}
		}
		
		ExternalDebugPrintLn("MyBasic::PostHttp waiting got response ("+String(answer.length())+") "+answer);
		
		mb_check(mb_push_string(mb, l, mb_memdup(answer.c_str(), (unsigned)(answer.length() + 1))));
	}
	else
	{
		result=MB_FUNC_ERR;
	}
	
	return result;
	
}

int MyBasic::HeapMemWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				MyBasic *c=((MyBasic *)ud);
				c->ExternalDebugPrintLn("MyBasic::HeapMemWrapper wrapper in threaded mode");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					c->m_paramdata.m_basic=mb;
					c->m_paramdata.m_ppvoid=l;
					c->m_waitingfor=WAITING_HEAPMEM;
				}
				
				c->ExternalDebugPrintLn("MyBasic::HeapMemWrapper wrapper waiting MEM");
				while(c->WaitingFor()==WAITING_HEAPMEM)
				{
					//std::this_thread::yield();
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					//taskYIELD();
					vTaskDelay(10);
				}
				
				c->ExternalDebugPrintLn("MyBasic::HeapMemWrapper wrapper got result");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					return c->m_paramdata.m_result;
				}
			}
			else
			{
				return ((MyBasic *)ud)->HeapMem(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;
}

int MyBasic::HeapMem(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_func_begin(mb,l));
	mb_check(mb_attempt_func_end(mb,l));

	mb_check(mb_push_int(mb, l, ESP.getFreeHeap()));
	
	return result;
}

int MyBasic::TemperatureWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				MyBasic *c=((MyBasic *)ud);
				c->ExternalDebugPrintLn("MyBasic::TemperatureWrapper wrapper in threaded mode");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					c->m_paramdata.m_basic=mb;
					c->m_paramdata.m_ppvoid=l;
					c->m_waitingfor=WAITING_TEMPERATURE;
				}
				
				c->ExternalDebugPrintLn("MyBasic::TemperatureWrapper wrapper waiting temp");
				while(c->WaitingFor()==WAITING_TEMPERATURE)
				{
					//std::this_thread::yield();
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					//taskYIELD();
					vTaskDelay(10);
				}
				
				c->ExternalDebugPrintLn("MyBasic::TemperatureWrapper wrapper got result");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					return c->m_paramdata.m_result;
				}
			}
			else
			{
				return ((MyBasic *)ud)->Temperature(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;

}

int MyBasic::Temperature(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_func_begin(mb,l));
	mb_check(mb_attempt_func_end(mb,l));

	mb_check(mb_push_real(mb, l, temperatureRead()));
	
	return result;
}

int MyBasic::HallWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				MyBasic *c=((MyBasic *)ud);
				c->ExternalDebugPrintLn("MyBasic::HallWrapper wrapper in threaded mode");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					c->m_paramdata.m_basic=mb;
					c->m_paramdata.m_ppvoid=l;
					c->m_waitingfor=WAITING_HALL;
				}
				
				c->ExternalDebugPrintLn("MyBasic::HallWrapper wrapper waiting hall");
				while(c->WaitingFor()==WAITING_HALL)
				{
					//std::this_thread::yield();
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					//taskYIELD();
					vTaskDelay(10);
				}
				
				c->ExternalDebugPrintLn("MyBasic::HallWrapper wrapper got result");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					return c->m_paramdata.m_result;
				}
			}
			else
			{
				return ((MyBasic *)ud)->Hall(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;

}

int MyBasic::Hall(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_func_begin(mb,l));
	mb_check(mb_attempt_func_end(mb,l));

	mb_check(mb_push_int(mb, l, hallRead()));
	
	return result;
}

int MyBasic::DelayWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				MyBasic *c=((MyBasic *)ud);
				c->ExternalDebugPrintLn("MyBasic::DelayWrapper wrapper in threaded mode");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					c->m_paramdata.m_basic=mb;
					c->m_paramdata.m_ppvoid=l;
					c->m_waitingfor=WAITING_HALL;
				}
				
				c->ExternalDebugPrintLn("MyBasic::DelayWrapper wrapper waiting delay");
				while(c->WaitingFor()==WAITING_HALL)
				{
					//std::this_thread::yield();
					//std::this_thread::sleep_for(std::chrono::milliseconds(10));
					//taskYIELD();
					vTaskDelay(10);
				}
				
				c->ExternalDebugPrintLn("MyBasic::DelayWrapper wrapper got result");
				{
					std::lock_guard<std::mutex> guard(c->m_mutex);
					return c->m_paramdata.m_result;
				}
			}
			else
			{
				return ((MyBasic *)ud)->Delay(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;

}

int MyBasic::Delay(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	int ms=0;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_open_bracket(mb,l));
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&ms));
	}
	else
	{
		return MB_FUNC_ERR;
	}	
	mb_check(mb_attempt_close_bracket(mb,l));
	
	delay(ms);
	
	return result;
}

int MyBasic::MaxStringLengthWrapper(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_func_begin(mb,l));
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&mybasic_maxstringlength));
	}
	mb_check(mb_attempt_func_end(mb,l));

	mb_check(mb_push_int(mb, l, mybasic_maxstringlength));
	
	return result;
}
/*
void MyBasic::OLEDInit()
{
	if(m_display==nullptr)
	{
		m_display=new SSD1306(m_oledaddress,m_oledsda,m_oledscl);
		m_display->init();
		m_display->flipScreenVertically();
	}
}

void MyBasic::OLEDDestroy()
{
	if(m_display)
	{
		delete m_display;
		m_display=nullptr;
	}
}

int MyBasic::OLEDClearWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				return MB_FUNC_ERR;
			}
			else
			{
				return ((MyBasic *)ud)->OLEDClear(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;
}

int MyBasic::OLEDClear(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_func_begin(mb,l));
	mb_check(mb_attempt_func_end(mb,l));

	{
		//std::lock_guard<std::mutex> guard(m_mutex);
		//OLEDInit();
		if(m_display)
		{
			m_usingoled=true;
			//portENTER_CRITICAL(&spinlock);
			m_display->clear();
			//portEXIT_CRITICAL(&spinlock);
		}
	}
	
	return result;
}

int MyBasic::OLEDDisplayWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				return MB_FUNC_ERR;
			}
			else
			{
				return ((MyBasic *)ud)->OLEDDisplayInternal(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;
}

int MyBasic::OLEDDisplayInternal(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	mb_assert(mb && l);
	mb_check(mb_attempt_func_begin(mb,l));
	mb_check(mb_attempt_func_end(mb,l));

	{
		//std::lock_guard<std::mutex> guard(m_mutex);
		//OLEDInit();
		if(m_display)
		{
			m_usingoled=true;
			//portENTER_CRITICAL(&spinlock);
			m_display->display();
			//portEXIT_CRITICAL(&spinlock);
		}
	}
	
	return result;
}

int MyBasic::OLEDPutPixelWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				return MB_FUNC_ERR;
			}
			else
			{
				return ((MyBasic *)ud)->OLEDPutPixel(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;
}

int MyBasic::OLEDPutPixel(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	int x=0;
	int y=0;
	mb_assert(mb && l);
	mb_check(mb_attempt_open_bracket(mb,l));
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&x));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&y));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	mb_check(mb_attempt_close_bracket(mb,l));
	
	{
		//std::lock_guard<std::mutex> guard(m_mutex);
		//OLEDInit();
		if(m_display)
		{
			m_usingoled=true;
			//portENTER_CRITICAL(&spinlock);
			m_display->setPixel(x,y);
			//portEXIT_CRITICAL(&spinlock);
		}
	}
	
	return result;
}

int MyBasic::OLEDPrintWrapper(struct mb_interpreter_t *mb, void **l)
{
	void *ud;
	if(mb_get_userdata(mb,&ud)==MB_FUNC_OK)
	{
		if(ud)
		{
			if(((MyBasic *)ud)->RunThreaded()==true)
			{
				return MB_FUNC_ERR;
			}
			else
			{
				return ((MyBasic *)ud)->OLEDPrint(mb,l);
			}
		}
	}
	return MB_FUNC_ERR;
}

int MyBasic::OLEDPrint(struct mb_interpreter_t *mb, void **l)
{
	int result=MB_FUNC_OK;
	
	char *str;
	int x=0;
	int y=0;
	int size=10;
	mb_assert(mb && l);
	mb_check(mb_attempt_open_bracket(mb,l));
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_string(mb,l,&str));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&x));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&y));
	}
	else
	{
		return MB_FUNC_ERR;
	}
	if(mb_has_arg(mb,l)>0)
	{
		mb_check(mb_pop_int(mb,l,&size));
	}
	mb_check(mb_attempt_close_bracket(mb,l));
	
	{
		//std::lock_guard<std::mutex> guard(m_mutex);
		//OLEDInit();
		if(m_display)
		{
			m_usingoled=true;
			//portENTER_CRITICAL(&spinlock);
			switch(size)
			{
			case 24:
				m_display->setFont(ArialMT_Plain_24);
				break;
			case 16:
				m_display->setFont(ArialMT_Plain_16);
				break;
			default:
				m_display->setFont(ArialMT_Plain_10);
				break;
			}
			//portEXIT_CRITICAL(&spinlock);
			m_display->drawString(x,y,str);
		}
	}
	
	return result;
}
*/
}	// namespace MyBasicESP
