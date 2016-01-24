#include <iostream>
#include <assert.h>


#include "TL_Xml2Array.h"
#include "TL_AssocArray.h"
#include "TL_Exception.h"


using namespace std;


namespace ThreeLight
{


TL_Xml2Array::TL_Xml2Array(TL_AssocArray* table_):
xmlpp::SaxParser(),
_startElement(false),
_table(table_),
_dataValid(true)
{}

void 
TL_Xml2Array::parseMemory(const string& memory_)
{
	try 
	{
		parse_memory(memory_);
	}
	catch(const xmlpp::exception& ex)
	{
		throw TL_Exception<TL_Xml2Array>("TL_Xml2Array::parseMemory",
			ex.what());
		_dataValid = false;
	}
}

void 
TL_Xml2Array::setTable(TL_AssocArray* table_)
{
	// clear the previous error message;
	_errorInfo.str("");
	_table = table_;
	_dataValid = true;
}

const string
TL_Xml2Array::getErrorInfo() const
{
	return _errorInfo.str();
}

void 
TL_Xml2Array::on_start_element(const string& name_,
	const AttributeList& attributes_)
{
	_startElement=true;
	TL_AssocArray &newKey = (*_table)(name_);

	xmlpp::SaxParser::AttributeList::const_iterator iter;
	for(iter = attributes_.begin(); iter != attributes_.end(); ++iter)
	{
		newKey.attr[iter->name] = iter->value;
	}
	_table = &newKey;
}

void 
TL_Xml2Array::on_end_element(const string& name_)
{
	_startElement=false;
	if(_table->getUpTable() != NULL)
	{
		_table = _table->getUpTable();
	}
}

void 
TL_Xml2Array::on_cdata_block(const string& text_)
{
	if(_startElement)
		*_table = text_;
	else
		(*_table)("__CDATA__") = text_;
}

void 
TL_Xml2Array::on_error(const string& text_)
{
	throw TL_Exception<TL_Xml2Array>("TL_Xml2Array::on_error",
		text_);
	_dataValid = false;
}

void 
TL_Xml2Array::on_fatal_error(const string& text_)
{
	throw TL_Exception<TL_Xml2Array>("TL_Xml2Array::on_fatal_error",
		text_);
	_dataValid = false;
}

} // end of ThreeLight namespace

