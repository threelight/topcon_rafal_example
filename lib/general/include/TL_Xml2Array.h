#ifndef TL_XML2ARRAY
#define TL_XML2ARRAY


#include <libxml++/libxml++.h>
#include <sstream>


using namespace xmlpp;
using namespace std;


namespace ThreeLight
{


class TL_AssocArray;


class TL_Xml2Array:
	public xmlpp::SaxParser
{
public:
	TL_Xml2Array(TL_AssocArray*);
	virtual ~TL_Xml2Array(){};
	
	void parseMemory(const std::string&);
	void setTable(TL_AssocArray*);
	const std::string getErrorInfo() const;
	bool getDataValid() const {return _dataValid;}
	
protected:
	//overrides: xmlpp::SaxParser
	virtual void on_start_element(const string& name_,
				const AttributeList& properties_);
	virtual void on_end_element(const string& name_);
	virtual void on_cdata_block(const string& text_);
	virtual void on_error(const string& text_);
	virtual void on_fatal_error(const string& text_);

private:
	bool _startElement;
	TL_AssocArray* _table;
	std::stringstream _errorInfo;
	bool _dataValid;
};


} // end of ThreeLight namespace


#endif

