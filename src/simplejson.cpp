#include "simplejson.h"

namespace SimpleJSON
{

JSONContainer::JSONContainer()
{
	Set();
}

JSONContainer::JSONContainer(const std::vector<JSONContainer> &val)
{
	Set(val);
}

JSONContainer::JSONContainer(const std::vector<String> &val)
{
	Set(val);
}

JSONContainer::JSONContainer(const std::vector<int> &val)
{
	Set(val);
}

JSONContainer::JSONContainer(const std::vector<float> &val)
{
	Set(val);
}
/*
JSONContainer::JSONContainer(const std::vector<bool> &val)
{
	Set(val);
}*/

JSONContainer::JSONContainer(const String &name, const JSONContainer &val)
{
	Set(name,val);
}

JSONContainer::JSONContainer(const String &name, const String &val)
{
	Set(name,val);
}

JSONContainer::JSONContainer(const String &name, const int &val)
{
	Set(name,val);
}

JSONContainer::JSONContainer(const String &name, const float &val, const int &decimals)
{
	Set(name,val,decimals);
}

JSONContainer::JSONContainer(const JSONContainer &val)
{
	//Set(val);
	*this=val;
}

JSONContainer::JSONContainer(const String &val)
{
	Set(val);
}

JSONContainer::JSONContainer(const int &val)
{
	Set(val);
}

JSONContainer::JSONContainer(const float &val, const int &decimals)
{
	Set(val,decimals);
}
/*
JSONContainer::JSONContainer(const bool &val)
{
	Set(val);
}*/

JSONContainer::~JSONContainer()
{

}

const uint8_t JSONContainer::Type() const
{
	return m_type;
}

const String &JSONContainer::Name() const
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		return m_str;
	}
	return "";
}

const bool JSONContainer::Parse(const String &json)
{
	std::vector<Token> tokens;
	if(ExtractTokens(json,0,json.length(),tokens)==false)
	{
		return false;
	}

	return Parse(json,tokens,0,tokens.size());

}

const bool JSONContainer::Parse(const String &json, const std::vector<Token> &tokens, const int &starttoken, const int &endtoken)
{

	if(starttoken<0 || starttoken>=tokens.size() || starttoken>=endtoken)
	{
		return false;
	}

	const int starttokentype=tokens[starttoken].m_type;
	if(starttokentype==TOKEN_STARTOBJ || starttokentype==TOKEN_STARTARRAY)
	{
		bool foundendtoken=false;
		int nestlevel=0;
		int endtokenpos=starttoken+1;
		for(int i=starttoken+1; i<endtoken && foundendtoken==false; ++i)
		{
			if(starttokentype==TOKEN_STARTOBJ && tokens[i].m_type==TOKEN_STARTOBJ)
			{
				nestlevel++;
			}
			else if(starttokentype==TOKEN_STARTOBJ && tokens[i].m_type==TOKEN_ENDOBJ)
			{
				if(nestlevel>0)
				{
					nestlevel--;
				}
				else
				{
					foundendtoken=true;
					endtokenpos=i+1;
				}
			}
			else if(starttokentype==TOKEN_STARTARRAY && tokens[i].m_type==TOKEN_STARTARRAY)
			{
				nestlevel++;
			}
			else if(starttokentype==TOKEN_STARTARRAY && tokens[i].m_type==TOKEN_ENDARRAY)
			{
				if(nestlevel>0)
				{
					nestlevel--;
				}
				else
				{
					foundendtoken=true;
					endtokenpos=i+1;
				}
			}
		}
		if(foundendtoken==false)
		{
			return false;
		}

		if(starttokentype==TOKEN_STARTOBJ)
		{
			Set();
			m_type=JSON_CONTAINER_OBJ;
		}
		else if(starttokentype==TOKEN_STARTARRAY)
		{
			Set();
			m_type=JSON_CONTAINER_ARRAY;
		}
		{
			//find and parse all comma delimited items at this nesting level
			int laststarttoken=starttoken+1;
			for(int i=starttoken+1; i<endtokenpos; ++i)
			{
				if(tokens[i].m_type==TOKEN_COMMA && tokens[starttoken].m_objlevel==tokens[i].m_objlevel && tokens[starttoken].m_arraylevel==tokens[i].m_arraylevel)
				{
					JSONContainer item;
					if(item.Parse(json,tokens,laststarttoken,i+1)==false)
					{
						return false;
					}
					m_children.push_back(item);
					laststarttoken=i+1;
				}
			}
			if(laststarttoken!=endtokenpos)
			{
				JSONContainer item;
				if(item.Parse(json,tokens,laststarttoken,endtokenpos)==false)
				{
					return false;
				}
				m_children.push_back(item);
			}
		}
		return true;
	}
	else if(starttokentype==TOKEN_STARTSTRING)
	{
		if(starttoken+1>=endtoken || tokens[starttoken+1].m_type!=TOKEN_ENDSTRING)
		{
			return false;
		}
		if(starttoken+2<endtoken && tokens[starttoken+2].m_type==TOKEN_COLON)
		{
			JSONContainer child;
			child.Parse(json,tokens,starttoken+3,endtoken);
			Set(json.substring(tokens[starttoken].m_pos+1,tokens[starttoken+1].m_pos),child);
		}
		else
		{
			Set(json.substring(tokens[starttoken].m_pos+1,tokens[starttoken+1].m_pos));
		}
		return true;
	}
	else if(starttokentype==TOKEN_STARTNUMBER)
	{
		if(starttoken+1>=endtoken || tokens[starttoken+1].m_type!=TOKEN_ENDNUMBER)
		{
			return false;
		}
		String val(json.substring(tokens[starttoken].m_pos,tokens[starttoken+1].m_pos));
		if(val.indexOf('.')>-1 || val.indexOf('e')>-1 || val.indexOf("E")>-1)
		{
			int decimals=2;

			int decpos=val.indexOf('.');
			if(decpos>=0)
			{
				decimals=val.length()-(decpos+1);
				if(decimals>16 || decimals<0)
				{
					decimals=2;
				}
			}
			Set(val.toFloat(),decimals);
		}
		else
		{
			Set((int)val.toInt());
		}
		return true;
	}
	else if(starttokentype==TOKEN_STARTBOOL)
	{
		if(starttoken+1>=endtoken || tokens[starttoken+1].m_type!=TOKEN_ENDBOOL)
		{
			return false;
		}
		if(json[tokens[starttoken].m_pos]=='t')
		{
			SetBool(true);
		}
		else
		{
			SetBool(false);
		}
		return true;
	}
	else if(starttokentype==TOKEN_STARTNULL)
	{
		if(starttoken+1>=endtoken || tokens[starttoken+1].m_type!=TOKEN_ENDNULL)
		{
			return false;
		}
		SetNull();
		return true;
	}
	else
	{
		return false;
	}
	return false;
}

const bool JSONContainer::ExtractTokens(const String &json, const int &start, const int &end, std::vector<Token> &tokens) const
{
	if(start>=0 && end<=json.length())
	{
		tokens.clear();

		int objlevel=0;
		int arraylevel=0;
		bool instring=false;
		for(int pos=start; pos<end; ++pos)
		{
			const char c=json[pos];
			if(instring==true)
			{
				if(c=='"')
				{
					tokens.push_back(Token(TOKEN_ENDSTRING,pos,objlevel,arraylevel));
					instring=false;
				}
				if(c=='\\')
				{
					if(!(pos<end-1))
					{
						return false;
					}
					pos++;
					if(json[pos]=='u')
					{
						if(!(pos+4<=end))
						{
							return false;
						}
						pos+=3;
					}
				}
			}
			else
			{
				if(c=='{')
				{
					objlevel++;
					tokens.push_back(Token(TOKEN_STARTOBJ,pos,objlevel,arraylevel));
				}
				else if(c=='}')
				{
					tokens.push_back(Token(TOKEN_ENDOBJ,pos,objlevel,arraylevel));
					objlevel--;
				}
				else if(c=='[')
				{
					arraylevel++;
					tokens.push_back(Token(TOKEN_STARTARRAY,pos,objlevel,arraylevel));
				}
				else if(c==']')
				{
					tokens.push_back(Token(TOKEN_ENDARRAY,pos,objlevel,arraylevel));
					arraylevel--;
				}
				else if(c==',')
				{
					tokens.push_back(Token(TOKEN_COMMA,pos,objlevel,arraylevel));
				}
				else if(c==':')
				{
					tokens.push_back(Token(TOKEN_COLON,pos,objlevel,arraylevel));
				}
				else if(c=='\"')
				{
					tokens.push_back(Token(TOKEN_STARTSTRING,pos,objlevel,arraylevel));
					instring=true;
				}
				else if(c=='t' && pos+4<=end && json.substring(pos,pos+4)=="true")
				{
					tokens.push_back(Token(TOKEN_STARTBOOL,pos,objlevel,arraylevel));
					tokens.push_back(Token(TOKEN_ENDBOOL,pos+4,objlevel,arraylevel));
					pos+=3;
				}
				else if(c=='f' && pos+5<=end && json.substring(pos,pos+5)=="false")
				{
					tokens.push_back(Token(TOKEN_STARTBOOL,pos,objlevel,arraylevel));
					tokens.push_back(Token(TOKEN_ENDBOOL,pos+5,objlevel,arraylevel));
					pos+=4;
				}
				else if(c=='n' && pos+4<=end && json.substring(pos,pos+4)=="null")
				{
					tokens.push_back(Token(TOKEN_STARTNULL,pos,objlevel,arraylevel));
					tokens.push_back(Token(TOKEN_ENDNULL,pos+4,objlevel,arraylevel));
					pos+=3;
				}
				else if(c=='-' || (c>='0' && c<='9'))
				{
					tokens.push_back(Token(TOKEN_STARTNUMBER,pos,objlevel,arraylevel));
					while((pos+1)<end && ((json[pos+1]>='0' && json[pos+1]<='9') || json[pos+1]=='+' || json[pos+1]=='-' || json[pos+1]=='.' || json[pos+1]=='e' || json[pos+1]=='E'))
					{
						pos++;
					}
					tokens.push_back(Token(TOKEN_ENDNUMBER,pos+1,objlevel,arraylevel));
				}
			}
		}

		return true;
	}
	return false;
}

const bool JSONContainer::IsWhitespace(const char c) const
{
	return (c==' ' || c=='\b' || c=='\f' || c=='\n' || c=='\r' || c=='\t');
}

void JSONContainer::Set()
{
	m_type=JSON_CONTAINER_OBJ;
	m_str="";
	m_children.clear();
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::Set(const std::vector<JSONContainer> &val)
{
	m_type=JSON_CONTAINER_ARRAY;
	m_str="";
	m_children=val;
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::Set(const std::vector<String> &val)
{
	m_type=JSON_CONTAINER_ARRAY;
	m_str="";
	m_children.clear();
	for(std::vector<String>::const_iterator i=val.begin(); i!=val.end(); ++i)
	{
		m_children.push_back(JSONContainer((*i)));
	}
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::Set(const std::vector<int> &val)
{
	m_type=JSON_CONTAINER_ARRAY;
	m_str="";
	m_children.clear();
	for(std::vector<int>::const_iterator i=val.begin(); i!=val.end(); ++i)
	{
		m_children.push_back(JSONContainer((*i)));
	}
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::Set(const std::vector<float> &val)
{
	m_type=JSON_CONTAINER_ARRAY;
	m_str="";
	m_children.clear();
	for(std::vector<float>::const_iterator i=val.begin(); i!=val.end(); ++i)
	{
		m_children.push_back(JSONContainer((*i)));
	}
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}
/*
void JSONContainer::Set(const std::vector<bool> &val)
{
	m_type=JSON_CONTAINER_ARRAY;
	m_name="";
	m_children.clear();
	for(std::vector<bool>::const_iterator i=val.begin(); i!=val.end(); ++i)
	{
		m_children.push_back(JSONContainer((*i)));
	}
	m_stringvalue="";
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}*/

void JSONContainer::Set(const String &name, const JSONContainer &val)
{
	m_type=JSON_CONTAINER_NAMEVALUE;
	m_str=name;
	m_children.clear();
	m_children.push_back(val);
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::Set(const String &name, const String &val)
{
	Set(name,JSONContainer(val));
}

void JSONContainer::Set(const String &name, const int &val)
{
	Set(name,JSONContainer(val));
}

void JSONContainer::Set(const String &name, const float &val, const int &decimals)
{
	Set(name,JSONContainer(val,decimals));
}

void JSONContainer::Set(const JSONContainer &val)
{
	m_type=JSON_VALUE_OBJ;
	m_str="";
	m_children.clear();
	m_children.push_back(val);
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::Set(const String &val)
{
	m_type=JSON_VALUE_STRING;
	m_str=val;
	m_children.clear();
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::Set(const int &val)
{
	m_type=JSON_VALUE_INT;
	m_str="";
	m_children.clear();
	//m_floatvalue=0;
	m_intvalue=val;
	//m_boolvalue=false;
}

void JSONContainer::Set(const float &val, const int &decimals)
{
	m_type=JSON_VALUE_FLOAT;
	m_str="";
	m_children.clear();
	m_floatvalue=val;
	m_floatdecimals=decimals;
	//m_intvalue=0;
	//m_boolvalue=false;
}

void JSONContainer::SetBool(const bool &val)
{
	m_type=JSON_VALUE_BOOL;
	m_str="";
	m_children.clear();
	//m_floatvalue=0;
	//m_intvalue=0;
	m_boolvalue=val;
}

void JSONContainer::SetNull()
{
	m_type=JSON_VALUE_NULL;
	m_str="";
	m_children.clear();
	m_floatvalue=0;
	//m_intvalue=0;
	//m_boolvalue=false;
}

JSONContainer &JSONContainer::operator=(const JSONContainer &rhs)
{
	if(this!=&rhs)
	{
		m_type=rhs.m_type;
		m_str=rhs.m_str;
		m_children=rhs.m_children;
		m_floatvalue=rhs.m_floatvalue;
		m_floatdecimals=rhs.m_floatdecimals;
		m_intvalue=rhs.m_intvalue;
		m_boolvalue=rhs.m_boolvalue;
	}
	return *this;
}

const bool JSONContainer::operator==(const JSONContainer &rhs) const
{
	return (m_type==rhs.m_type && m_str==rhs.m_str && m_children==rhs.m_children && m_floatvalue==rhs.m_floatvalue && m_intvalue==rhs.m_intvalue);
}

const bool JSONContainer::operator==(const String &name) const
{
	return(m_type==JSON_CONTAINER_NAMEVALUE && m_str==name);
}

const bool JSONContainer::operator!=(const JSONContainer &rhs) const
{
	return ((*this)!=rhs);
}

const bool JSONContainer::operator<(const JSONContainer &rhs) const
{
	if(m_type<rhs.m_type)
	{
		return true;
	}
	else if(m_type==rhs.m_type && m_str<rhs.m_str)
	{
		return true;
	}
	else if(m_type==rhs.m_type && m_str==rhs.m_str && m_children<rhs.m_children)
	{
		return true;
	}
	else if(m_type==rhs.m_type && m_str==rhs.m_str && m_children==rhs.m_children && m_intvalue<rhs.m_intvalue)
	{
		return true;
	}
	else if(m_type==rhs.m_type && m_str==rhs.m_str && m_children==rhs.m_children && m_intvalue==rhs.m_intvalue && m_floatvalue<rhs.m_floatvalue)
	{
		return true;
	}
	else if(m_type==rhs.m_type && m_str==rhs.m_str && m_children==rhs.m_children && m_intvalue==rhs.m_intvalue && m_floatvalue==rhs.m_floatvalue && m_boolvalue==rhs.m_boolvalue)
	{
		return true;
	}

	return false;
}

JSONContainer &JSONContainer::operator=(const String &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_str,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}

JSONContainer &JSONContainer::operator=(const int &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_str,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}

JSONContainer &JSONContainer::operator=(const float &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_str,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}
/*
JSONContainer &JSONContainer::operator=(const bool &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_name,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}*/

JSONContainer &JSONContainer::operator=(const std::vector<JSONContainer> &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_str,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}

JSONContainer &JSONContainer::operator=(const std::vector<String> &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_str,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}

JSONContainer &JSONContainer::operator=(const std::vector<int> &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_str,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}

JSONContainer &JSONContainer::operator=(const std::vector<float> &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_str,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}
/*
JSONContainer &JSONContainer::operator=(const std::vector<bool> &val)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		Set(m_name,val);
	}
	else
	{
		Set(val);
	}
	return *this;
}*/

JSONContainer &JSONContainer::operator[](const String &name)
{
	if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		if(m_children.size()>0)
		{
			if(m_children[0].m_type==JSON_CONTAINER_NAMEVALUE && m_children[0].m_str==name)
			{
				if(m_children[0].m_children.size()==0)
				{
					m_children[0].m_children.push_back(JSONContainer());
				}
				else
				{
					
				}
				// return the VALUE object, not the name container object
				return m_children[0].m_children[0];
			}
			else if(m_children[0].m_type==JSON_CONTAINER_OBJ)
			{
				return m_children[0][name];
			}
		}
		if(m_children.size()==0)
		{
			m_children.push_back(JSONContainer());
			return m_children[0];
		}
	}

	if(m_type!=JSON_CONTAINER_OBJ)
	{
		Set();
	}
	for(std::vector<JSONContainer>::iterator i=m_children.begin(); i!=m_children.end(); ++i)
	{
		if((*i)==name)
		{
			if((*i).m_children.size()==0)
			{
				(*i).m_children.push_back(JSONContainer());
			}
			// return VALUE object, not name object
			return (*i).m_children[0];
		}
	}
	m_children.push_back(JSONContainer(name,JSONContainer()));
	if(m_children[m_children.size()-1].m_children.size()==0)
	{
		m_children[m_children.size()-1].m_children.push_back(JSONContainer());
	}
	return m_children[m_children.size()-1].m_children[0];
}

JSONContainer &JSONContainer::operator[](const int index)
{
	if(m_type!=JSON_CONTAINER_ARRAY)
	{
		std::vector<JSONContainer> arr(index+1);
		Set(arr);
	}
	else
	{
		if((m_children.size()-1)<index)
		{
			m_children.resize(index+1);
		}
	}
	return m_children[index];
}

const String &JSONContainer::StringValue() const
{
	if(m_type==JSON_VALUE_STRING)
	{
		return m_str;
	}
	else if(m_type==JSON_CONTAINER_NAMEVALUE && m_children.size()>0)
	{
		return m_children[0].StringValue();
	}
	return "";
}

const int &JSONContainer::IntValue() const
{
	if(m_type==JSON_VALUE_INT)
	{
		return m_intvalue;
	}
	else if(m_type==JSON_CONTAINER_NAMEVALUE && m_children.size()>0)
	{
		return m_children[0].IntValue();
	}
	return 0;
}

const float &JSONContainer::FloatValue() const
{
	if(m_type==JSON_VALUE_FLOAT)
	{
		return m_floatvalue;
	}
	else if(m_type==JSON_CONTAINER_NAMEVALUE && m_children.size()>0)
	{
		return m_children[0].FloatValue();
	}
	return 0;
}

const bool &JSONContainer::BoolValue() const
{
	if(m_type==JSON_VALUE_BOOL)
	{
		return m_boolvalue;
	}
	else if(m_type==JSON_CONTAINER_NAMEVALUE && m_children.size()>0)
	{
		return m_children[0].m_boolvalue;
	}
	return 0;
}

const bool JSONContainer::IsNull() const
{
	if(m_type==JSON_VALUE_NULL)
	{
		return true;
	}
	else if(m_type==JSON_CONTAINER_NAMEVALUE && m_children.size()>0)
	{
		return m_children[0].IsNull();
	}
	return false;
}

const bool JSONContainer::HasProperty(const String &name) const
{
	if(m_type==JSON_CONTAINER_NAMEVALUE && m_str==name)
	{
		return true;
	}
	else if(m_type==JSON_CONTAINER_OBJ || m_type==JSON_VALUE_OBJ)
	{
		for(std::vector<JSONContainer>::const_iterator i=m_children.begin(); i!=m_children.end(); ++i)
		{
			if((*i).HasProperty(name)==true)
			{
				return true;
			}
		}
	}
	return false;
}

const int JSONContainer::ChildCount() const
{
	if(m_type==JSON_CONTAINER_OBJ || m_type==JSON_CONTAINER_ARRAY)
	{
		return m_children.size();
	}
	else if(m_type==JSON_CONTAINER_NAMEVALUE)
	{
		if(m_children.size()>0 && m_children[0].Type()==JSON_CONTAINER_ARRAY)
		{
			return m_children[0].m_children.size();
		}
		return 1;
	}
	return 0;
}

const String JSONContainer::Encode(const bool prettyprint, const int &nestlevel, const bool commaafter) const
{
	String out("");
	switch(m_type)
	{
	case JSON_CONTAINER_OBJ:
		out+="{"+PrettyPrintSuffix(prettyprint,nestlevel,false);
		out+=EncodeChildren(prettyprint,nestlevel+1);
		out+=PrettyPrintPrefix(prettyprint,nestlevel)+"}"+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		break;
	case JSON_CONTAINER_ARRAY:
		out+="["+PrettyPrintSuffix(prettyprint,nestlevel,false);
		out+=EncodeChildren(prettyprint,nestlevel+1);
		out+=PrettyPrintPrefix(prettyprint,nestlevel)+"]"+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		break;
	case JSON_CONTAINER_NAMEVALUE:
		out+=EncodeString(m_str);
		out+=":";
		if(prettyprint==true)
		{
			out+=" ";
		}
		if(m_children.size()>0)
		{
			out+=m_children.at(0).Encode(prettyprint,nestlevel,commaafter);
		}
		else
		{
			out+="null"+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		}
		break;
	case JSON_VALUE_OBJ:
		out+=EncodeChildren(prettyprint,nestlevel);
		break;
	case JSON_VALUE_STRING:
		out+=EncodeString(m_str)+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		break;
	case JSON_VALUE_INT:
		out+=String(m_intvalue)+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		break;
	case JSON_VALUE_FLOAT:
		out+=String(m_floatvalue,m_floatdecimals)+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		break;
	case JSON_VALUE_BOOL:
		out+=String(m_boolvalue ? "true" : "false")+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		break;
	case JSON_VALUE_NULL:
		out+=String("null")+PrettyPrintSuffix(prettyprint,nestlevel,commaafter);
		break;
	}
	return out;
}

const String JSONContainer::EncodeString(const String &val) const
{
	String out=val;
	out.replace("\\","\\\\");	// \\ needs to be first because everything else has \\ in it
	out.replace("\"","\\\"");
	out.replace("/","\\/");
	out.replace("\b","\\b");
	out.replace("\f","\\f");
	out.replace("\n","\\n");
	out.replace("\r","\\r");
	out.replace("\t","\\t");
	return "\""+out+"\"";
}

const String JSONContainer::EncodeChildren(const bool prettyprint, const int &nestlevel) const
{
	String out("");
	for(std::vector<JSONContainer>::const_iterator i=m_children.begin(); i!=m_children.end(); ++i)
	{
		out+=PrettyPrintPrefix(prettyprint,nestlevel);
		out+=(*i).Encode(prettyprint,nestlevel,((i!=m_children.end()-1) ? true : false));
	}
	return out;
}

const String JSONContainer::PrettyPrintPrefix(const bool prettyprint, const int &nestlevel) const
{
	String out("");
	if(prettyprint==true && nestlevel>0)
	{
		for(int i=0; i<nestlevel; ++i)
		{
			out+="    ";
		}
	}
	return out;
}

const String JSONContainer::PrettyPrintSuffix(const bool prettyprint, const int &nestlevel, const bool commaafter) const
{
	String out("");
	if(prettyprint==true)
	{
		if(commaafter==true)
		{
			out+=",";
		}
		out+="\r\n";
	}
	else if(prettyprint==false && commaafter==true)
	{
		out+=",";
	}
	return out;
}

const String JSONContainer::DecodeString(const String &val) const
{
	String out=val;
	out.replace("\\\"","\"");
	out.replace("\\/","/");
	out.replace("\\b","\b");
	out.replace("\\f","\f");
	out.replace("\\n","\n");
	out.replace("\\r","\r");
	out.replace("\\t","\t");
	out.replace("\\\\","\\");	// \\ needs to be done last
	return out;
}

}	// namespace SimpleJSON
