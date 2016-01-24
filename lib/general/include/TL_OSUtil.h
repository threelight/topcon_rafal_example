#ifndef __TL_OSUTIL__
#define __TL_OSUTIL__

#include <sys/types.h>
#include <vector>

using namespace std;

namespace ThreeLight
{

class TL_OSUtil
{
public:
	enum FileType
	{
		NONEXISTENT = 0,
		
		REGULAR = 1,	// Regular file
		DIRECTORY = 2,	// Dir
		CHAR = 3,	// Character device
		BLOCK = 4,	// Block device
		FIFO = 5,	// FIFO named pipe
		LINK = 6,	// Symbolik link
		SOCKET = 7	// Socket
	};
	
	enum
	{
		MAXERRORSIZE = 2048
	};
	

	/*! \brief Default is space when delimiter NULL.
	 */	
	static string GetErrorString(int errno_);

	/*! \brief Default is space when delimiter NULL.
	 */
	static void StrTok(const char* str_,
			vector<string> &pieces_,
			const char* delimiter_=NULL);
	
	static string GetWorkingDir();
	
	/*! \brief Standard readlink function
	 */
	static string readLink(const char* path_);
	
	/*! \brief Check if the object exits.
	 *         If it is a link then it checks the target
	 */
	static FileType FileExists(const char* path_);

	/*! \brief Recursively creates directory
	 *         if directory exists no error produced 
	 */
	static void CreateDir(const char* path_, mode_t mode_=0777);
	
	/*! \brief Recursively deletes directories or files
	 *         if object dosn't exist no error produced 
	 */
	static bool DeleteDir(const char* path_);
};


} // end of ThreeLight namespace


#endif

