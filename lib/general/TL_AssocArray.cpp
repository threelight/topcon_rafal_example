#include <sstream>
#include <assert.h>
#include <iostream>


#include "TL_AssocArray.h"


using namespace std;


namespace ThreeLight
{


TL_AssocArray::TL_AssocArray():
	_upTableLink(NULL),
	_level(0){}


TL_AssocArray::TL_AssocArray(const string& index_):
	_index(index_),
	_upTableLink(NULL),
	_level(0){}


TL_AssocArray::~TL_AssocArray()
{
	IteratorType iter = _innerTable.begin();
	
	for(; iter!=_innerTable.end(); ++iter)
	{
		delete (*iter).second;
	}
}


bool
TL_AssocArray::operator==(const char* value_) const
{
	return _value == value_;
};


bool
TL_AssocArray::operator==(const string& value_) const
{
	return _value == value_;
};


TL_AssocArray&
TL_AssocArray::operator[](unsigned long index_)
{
	if(_upTableLink==NULL) return *this;

	pair<IteratorType, IteratorType> range =
		_upTableLink->_innerTable.equal_range(_index);
	unsigned long cnt=0;

	IteratorType iter = range.first;
	for(;iter!=range.second; ++iter,++cnt)
	{
		if(cnt==index_) return *((*iter).second);
	}

	// if index could not be found 
	return *((*--iter).second);
}


TL_AssocArray&
TL_AssocArray::operator[](const string& index_)
{
	IteratorType iter = _innerTable.find(index_);
	if(iter != _innerTable.end())
	{
		return *((*iter).second);
	} else {
		// if I have the under key I cannot have a value
		_value = "";
		TL_AssocArray *table = new TL_AssocArray(index_);

		_innerTable.insert(make_pair(index_, table));
		table->_upTableLink = this;
		table->_level = _level+1;

		return *table;
	}
};


TL_AssocArray&
TL_AssocArray::operator()(const string& index_)
{
	_value = "";
	TL_AssocArray *table = new TL_AssocArray(index_);

	_innerTable.insert(make_pair(index_, table));
	table->_upTableLink = this;
	table->_level = _level+1;

	return *table;
}


const TL_AssocArray&
TL_AssocArray::operator=(const string& value_)
{
	return operator=( value_.c_str() );
};


const TL_AssocArray&
TL_AssocArray::operator=(const char* value_)
{
	IteratorType iter = _innerTable.begin();

	for(; iter!=_innerTable.end(); ++iter)
	{
		delete (*iter).second;
	}
	_innerTable.clear();
	
	if(value_ != NULL)
	{
		_value = value_;
	}
	else 
	{
		_value = "";
	}

	return *this;
}


const TL_AssocArray&
TL_AssocArray::operator=(const int &num)
{
	stringstream convert;
	
	convert << num;
	
	return operator=( convert.str().c_str() );
}


const TL_AssocArray&
TL_AssocArray::operator=(const unsigned int &num)
{
	stringstream convert;
	
	convert << num;
	
	return operator=( convert.str().c_str() );
}

const TL_AssocArray&
TL_AssocArray::operator=(const long & num)
{
	stringstream convert;
	
	convert << num;
	
	return operator=( convert.str().c_str() );
}

const TL_AssocArray&
TL_AssocArray::operator=(const unsigned long &num)
{
	stringstream convert;
	
	convert << num;
	
	return operator=( convert.str().c_str() );
}
	
const TL_AssocArray&
TL_AssocArray::operator=(const char &num)
{
	stringstream convert;
	
	convert << num;
	
	return operator=( convert.str().c_str() );
}


const TL_AssocArray&
TL_AssocArray::operator=(const float &num)
{
	stringstream convert;
	
	convert << num;
	
	return operator=( convert.str().c_str() );
}


const TL_AssocArray&
TL_AssocArray::operator=(const double &num)
{
	stringstream convert;
	
	convert << num;
	
	return operator=( convert.str().c_str() );
}


bool TL_AssocArray::isIndex(const string& index_) const
{
	return _innerTable.find(index_) != _innerTable.end();
}


unsigned long TL_AssocArray::count(const string& index_) const
{
	return index_.empty() ? _innerTable.size() : _innerTable.count(index_);
}


TL_AssocArray* TL_AssocArray::getUpTable() const
{
	return this->_upTableLink;
}


TL_AssocArray* TL_AssocArray::getTopTable()
{
	TL_AssocArray *table=this,*prev=this;
	while( NULL!= (table = table->getUpTable()) ) prev = table;
	return prev;
}


bool TL_AssocArray::isAttribute(const string& name_) const
{
	return attr.find(name_) != attr.end();
}


void TL_AssocArray::print()
{
	if(_level)
	{
		for(unsigned int i=0; i<_level; i++)
		{
			cout << ("\t");
		}
		cout << "NODE=" << _index << "    ";
		
		map<string, string>::const_iterator iter = attr.begin();
		for(cout << "ATTR=("; iter!=attr.end(); ++iter)
		{
			cout << (*iter).first << "=" << "'" <<(*iter).second << "' ";
		}
		cout << ")    VALUE=" << _value << endl;
	}
	
	ConstIteratorType iter = _innerTable.begin();
	for(; iter!=_innerTable.end(); ++iter)
	{
		(*iter).second->print();
	}
}


} // and of ThreeLight namespace
