#include "configfile.h"

ConfigFile::ConfigFile()
{

}

ConfigFile::~ConfigFile()
{

}

const bool ConfigFile::LoadFromSPIFFS(const String &path)
{
	m_options.clear();
	
	File f=SPIFFS.open(path,"r");
	if(!f)
	{
		return false;
	}
	
	size_t size=f.size();
	String filedata("");
	filedata.reserve(size);
	filedata=f.readString();
	
	int startpos=0;
	int epos=filedata.indexOf("=");
	int npos=0;
	while(epos>-1)
	{
		npos=filedata.indexOf("\n",epos);
		if(npos==-1)
		{
			npos=size;
		}
		if(npos>epos)
		{
			m_options[filedata.substring(startpos,epos)]=filedata.substring(epos+1,npos);
			
			//Serial.println(String(FPSTR("ConfigFile::LoadFromSPIFFS found "))+filedata.substring(startpos,epos)+"="+filedata.substring(epos+1,npos));
			
			epos=filedata.indexOf("=",npos);
			startpos=npos+1;
		}
		else
		{
			epos=-1;
		}
	}
	
	f.close();
	return true;
} 

const bool ConfigFile::SaveToSPIFFS(const String &path)
{
	File f=SPIFFS.open(path,"w");
	if(!f)
	{
		return false;
	}
	
	for(std::map<String,String>::const_iterator i=m_options.begin(); i!=m_options.end(); ++i)
	{
		f.write((const uint8_t *)(*i).first.c_str(),(*i).first.length());
		f.write((const uint8_t *)"=",1);
		f.write((const uint8_t *)(*i).second.c_str(),(*i).second.length());
		f.write((const uint8_t *)"\n",1);
	}
	
	f.close();
	return true;
}

const bool ConfigFile::HasOption(const String &option) const
{
	return(m_options.find(option)!=m_options.end());
}

const String ConfigFile::GetOption(const String &option) const
{
	std::map<String,String>::const_iterator pos=m_options.find(option);
	if(pos!=m_options.end())
	{
		return (*pos).second;
	}
	else
	{
		return String("");
	}
}

void ConfigFile::SetOption(const String &option, const String &value)
{
	m_options[option]=value;
}

void ConfigFile::ClearOption(const String &option)
{
	std::map<String,String>::iterator pos=m_options.find(option);
	if(pos!=m_options.end())
	{
		m_options.erase(pos);
	}
}

void ConfigFile::ClearOptions()
{
	m_options.clear();
}

String &ConfigFile::operator[](const String &option)
{
	return m_options[option];
}
