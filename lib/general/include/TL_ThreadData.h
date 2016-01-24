#ifndef __TL_THREADDATA__
#define __TL_THREADDATA__


#include <sstream>
#include <fstream>


using namespace std;


namespace ThreeLight
{


struct  TL_ThreadData
{
	TL_ThreadData()
	{
		_file = NULL;
	}

	~TL_ThreadData()
	{
		if(_file != NULL)
		{
			_file->flush();
			delete _file;
			_file = NULL;
		}
	}
	
	 
	stringstream _ss;

	/*! \brief log outstream
	*/
	ofstream *_file;

	/*! \brief name of the log file
	*/
	string _logFileName;
};

	
}


#endif
