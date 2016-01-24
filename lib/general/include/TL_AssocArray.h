#ifndef TL_ASSOCARRAY
#define TL_ASSOCARRAY


#include <string>
#include <list>
#include <map>
#include <vector>


namespace ThreeLight
{


class TL_AssocArray 
{
public:
	TL_AssocArray();
	TL_AssocArray(const std::string&);
	
	virtual ~TL_AssocArray();

	operator const char*(void) const {return _value.c_str();}
	
	bool operator==(const char *) const;
	bool operator==(const std::string&) const;
	
	// If you have the same indexes in the table
	// on the same level the only way to retrieve
	// individual ones is to use numbers
	// This function will not create new indexes
	TL_AssocArray& operator[](unsigned long);
	
	// normal retrieval way by string index
	// if index does not exists it will be created
	TL_AssocArray& operator[](const std::string&);
	
	TL_AssocArray& operator()(const std::string&);
	
	const TL_AssocArray& operator=(const std::string &);
	const TL_AssocArray& operator=(const char*);

	const TL_AssocArray& operator=(const int &);
	const TL_AssocArray& operator=(const unsigned int &);
	const TL_AssocArray& operator=(const long &);
	const TL_AssocArray& operator=(const unsigned long &);
	const TL_AssocArray& operator=(const char &);
	const TL_AssocArray& operator=(const float &);
	const TL_AssocArray& operator=(const double &);
		
	const std::string& str() const {return _value;}
	
	bool isAttribute(const std::string&) const;
	bool isIndex(const std::string&) const;
	
	unsigned long count(const std::string& = "") const;

	TL_AssocArray* getUpTable() const;
	TL_AssocArray* getTopTable();
	void print();
	

	// special filed representing attributes in xml structure
	std::map<std::string, std::string> attr;
	
	typedef std::multimap<std::string, TL_AssocArray*> AssocArrayType;
	typedef AssocArrayType::const_iterator ConstIteratorType;
	typedef AssocArrayType::iterator IteratorType;
	
private:
	AssocArrayType _innerTable;
	
	std::string _index;
	std::string _value;
	
	TL_AssocArray* _upTableLink;
	unsigned long _level;
};


} // end of threelight namespace


#endif

