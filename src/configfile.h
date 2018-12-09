#ifndef _configfile_
#define _configfile_

#include <SPIFFS.h>
#include <WString.h>

#include <map>

class ConfigFile
{
public:
	ConfigFile();
	~ConfigFile();
	
	const bool LoadFromSPIFFS(const String &path);
	const bool SaveToSPIFFS(const String &path);
	
	const bool HasOption(const String &option) const;
	const String GetOption(const String &option) const;
	void SetOption(const String &option, const String &value);
	void ClearOption(const String &option);
	void ClearOptions();
	
	String &operator[](const String &option);

private:
	std::map<String,String> m_options;
};

#endif	// _configfile_
