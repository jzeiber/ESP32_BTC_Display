#ifndef _simplejson_
#define _simplejson_

/*

	A simple quick and dirty json parser / encoder.  It may not catch all edge cases, but for json documents with simple structure, it should parse and encode.

*/

#include <WString.h>
#include <vector>

namespace SimpleJSON
{

enum JSONType
{
	JSON_CONTAINER_OBJ=1,
	JSON_CONTAINER_ARRAY=2,
	JSON_CONTAINER_NAMEVALUE=3,
	JSON_VALUE_OBJ=4,
	JSON_VALUE_STRING=5,
	JSON_VALUE_INT=6,
	JSON_VALUE_FLOAT=7,
	JSON_VALUE_BOOL=8,
	JSON_VALUE_NULL=9
};

class JSONContainer
{
public:
	JSONContainer();
	JSONContainer(const std::vector<JSONContainer> &val);			// array container
	JSONContainer(const std::vector<String> &val);					// array container (string)
	JSONContainer(const std::vector<int> &val);						// array container (int)
	JSONContainer(const std::vector<float> &val);					// array container (float)
	//JSONContainer(const std::vector<bool> &val);					// array container (bool)
	JSONContainer(const String &name, const JSONContainer &val);	// name/value container
	JSONContainer(const String &name, const String &val);			// name/string value
	JSONContainer(const String &name, const int &val);				// name/int value
	JSONContainer(const String &name, const float &val, const int &decimals=2);		// name/float value
	JSONContainer(const JSONContainer &val);						// copy constructor - don't confuse with setting object value - that must be done with Set method
	JSONContainer(const String &val);								// string value
	JSONContainer(const int &val);									// int value
	JSONContainer(const float &val, const int &decimals);			// float value
	//JSONContainer(const bool &val);								// bool value
	~JSONContainer();

	const uint8_t Type() const;
	const String &Name() const;

	const bool Parse(const String &json);

	void Set();													// object container
	void Set(const std::vector<JSONContainer> &val);			// array container
	void Set(const std::vector<String> &val);					// array container (string)
	void Set(const std::vector<int> &val);						// array container (int)
	void Set(const std::vector<float> &val);					// array container (float)
	//void Set(const std::vector<bool> &val);						// array container (bool)
	void SetBool(const std::vector<bool> &val);
	void Set(const String &name, const JSONContainer &val);		// name/value container
	void Set(const String &name, const String &val);			// name/string value
	void Set(const String &name, const int &val);				// name/int value
	void Set(const String &name, const float &val, const int &decimals=2);		// name/float value
	void Set(const JSONContainer &val);							// object value - don't confuse with copy constructor or equal operator - use Set to set object value , use = for object copy
	void Set(const String &val);								// string value
	void Set(const int &val);									// int value
	void Set(const float &val, const int &decimals=2);			// float value
	//void Set(const bool &val);								// bool value
	void SetBool(const bool &val);
	void SetNull();

	const String &StringValue() const;
	const int &IntValue() const;
	const float &FloatValue() const;
	const bool &BoolValue() const;
	const bool IsNull() const;

	const String Encode(const bool prettyprint=false, const int &nestlevel=0, const bool commaafter=false) const;

	JSONContainer &operator=(const JSONContainer &rhs);			// assignment operator
	const bool operator==(const JSONContainer &rhs) const;
	const bool operator==(const String &name) const;
	const bool operator!=(const JSONContainer &rhs) const;
	const bool operator<(const JSONContainer &rhs) const;

	JSONContainer &operator=(const String &val);
	JSONContainer &operator=(const int &val);
	JSONContainer &operator=(const float &val);
	//JSONContainer &operator=(const bool &val);
	JSONContainer &operator=(const std::vector<JSONContainer> &val);
	JSONContainer &operator=(const std::vector<String> &val);
	JSONContainer &operator=(const std::vector<int> &val);
	JSONContainer &operator=(const std::vector<float> &val);
	//JSONContainer &operator=(const std::vector<bool> &val);

	const bool HasProperty(const String &name) const;
	const int ChildCount() const;
	//JSONContainer &Child(const int index);

	JSONContainer &operator[](const String &name);		// forces this to object container with name/value pairs
	JSONContainer &operator[](const int index);			// forces this to array container

private:
	uint8_t m_type;
	String m_str;
	std::vector<JSONContainer> m_children;

	enum ParseTokenType
	{
		TOKEN_STARTOBJ=1,
		TOKEN_ENDOBJ=2,
		TOKEN_STARTARRAY=3,
		TOKEN_ENDARRAY=4,
		TOKEN_STARTSTRING=5,
		TOKEN_ENDSTRING=6,
		TOKEN_COMMA=7,
		TOKEN_COLON=8,
		TOKEN_STARTBOOL=9,
		TOKEN_ENDBOOL=10,
		TOKEN_STARTNUMBER=11,
		TOKEN_ENDNUMBER=12,
		TOKEN_STARTNULL=13,
		TOKEN_ENDNULL=14
	};

	struct Token
	{
		Token(const ParseTokenType &tokentype, const int &pos, const int &objlevel, const int &arraylevel):m_type(tokentype),m_pos(pos),m_objlevel(objlevel),m_arraylevel(arraylevel)	{}
		ParseTokenType m_type;
		int m_pos;
		int m_objlevel;
		int m_arraylevel;
	};

	const bool ExtractTokens(const String &json, const int &start, const int &end, std::vector<Token> &tokens) const;
	const bool Parse(const String &json, const std::vector<Token> &tokens, const int &starttoken, const int &endtoken);

	int m_floatdecimals;
	union
	{
	float m_floatvalue;
	int m_intvalue;
	bool m_boolvalue;
	};

	const String EncodeString(const String &val) const;
	const String EncodeChildren(const bool prettyprint, const int &nestlevel) const;
	const String DecodeString(const String &val) const;

	const String PrettyPrintPrefix(const bool prettyprint, const int &nestlevel) const;
	const String PrettyPrintSuffix(const bool prettyprint, const int &nestlevel, const bool commaafter) const;
	
	const bool IsWhitespace(const char c) const;

};

}	// namespace SimpleJSON

#endif	// _simplejson_
