#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <iostream>


#include "TL_OSUtil.h"
#include "TL_Log.h"
#include "TL_Exception.h"


namespace ThreeLight
{


string
TL_OSUtil::GetErrorString(int errno_)
{
	char buf[MAXERRORSIZE];
	string description;
	
	description =
		strerror_r(errno_, buf, MAXERRORSIZE);
	return description;
}


void
TL_OSUtil::StrTok(const char* str_,
	vector<string> &pieces_,
	const char *delimiter_)
{
	if(str_==NULL)
	{
		pieces_.clear();
		return;
	}
	static const char *DefaultDelimiter=" ";
	
	char *delim = const_cast<char*>(delimiter_);
	if(delim==NULL)
	{
		delim = const_cast<char*>(DefaultDelimiter);	
	}
	
	size_t size = strlen(str_);
	char path[size+1];
	
	memcpy(path, str_, size+1);
	
	char *ptr = path, *savePtr, *token;
	
	for(;;ptr=NULL)
	{
		token = strtok_r(ptr, delim, &savePtr);
		if(token==NULL)
			break;
		pieces_.push_back( string(token) );
			
	}
	
}


string
TL_OSUtil::GetWorkingDir()
{
	char buf[PATH_MAX];
	char *result=NULL;
	
	result = getcwd(buf, PATH_MAX);
	if(result==NULL)
	{
		throw TL_Exception<TL_OSUtil>("TL_OSUtil::getWorkingDir",
			GetErrorString(errno));
	}
	
	string dir(buf);
	return dir;
}


string
TL_OSUtil::readLink(const char* path_)
{
	char buf[PATH_MAX];
	memset(buf, 0, PATH_MAX);
	ssize_t result = readlink(path_, buf, PATH_MAX);

	if(result == -1)
	{
		throw TL_Exception<TL_OSUtil>("TL_OSUtil::readLink",
			GetErrorString(errno) );
	}

	return string(buf);
}


TL_OSUtil::FileType
TL_OSUtil::FileExists(const char* path_)
{
	if(path_==NULL)
	{
		throw TL_Exception<TL_OSUtil>("TL_OSUtil::fileExists",
			"NULL pointer passed.");
	}
	
	string path;
	
	if(path_[0] != '/')
	{
		// relative path
		path = GetWorkingDir();
		path.append("/");
		path.append(path_);
	}
	else
	{
		path = path_;
	}
	
	FileType ret = NONEXISTENT;
	
	struct stat info;
	int result = stat(path.c_str(), &info);
	
	if(result==-1)
	{
		if(errno==ENOENT)
		{
			ret = NONEXISTENT;
		}
		else
		{
			throw TL_Exception<TL_OSUtil>("TL_OSUtil::fileExists",
				GetErrorString(errno) );
		}
	}
	else
	{
		if(S_ISREG(info.st_mode))
		{
			ret = REGULAR;
		}
		else if(S_ISDIR(info.st_mode))
		{
			ret = DIRECTORY;
		}
		else if(S_ISLNK(info.st_mode))
		{
			ret = LINK;
		}
		else if(S_ISBLK(info.st_mode))
		{
			ret = BLOCK;
		}
		else if(S_ISFIFO(info.st_mode))
		{
			ret = FIFO;
		}
		else if(S_ISCHR(info.st_mode))
		{
			ret = CHAR;
		}
		else if(S_ISSOCK(info.st_mode))
		{
			ret = SOCKET;
		}
	}

	return ret;
}


void
TL_OSUtil::CreateDir(const char* path_, mode_t mode_)
{
	if(path_==NULL)
	{
		throw TL_Exception<TL_OSUtil>("TL_OSUtil::CreateDir",
			"NULL pointer passed.");
	}
	
	string path;
	if(path_[0] != '/')
	{
		path = GetWorkingDir();
	}
	path.append("/");

	vector<string> pathPieces;
	TL_OSUtil::FileType type=NONEXISTENT;
	StrTok(path_, pathPieces, "/");

	vector<string>::const_iterator iter = pathPieces.begin();
	for(; iter != pathPieces.end(); ++iter)
	{
		path.append( (*iter) );
		try
		{
			type = FileExists(path.c_str());
		}
		catch (TL_Exception<TL_OSUtil>& e)
		{
			throw TL_Exception<TL_OSUtil>("TL_OSUtil::CreateDir",
				_TLS( e.who() << "->" << e.what() ) );
		}

		if(type == NONEXISTENT ||
			type != DIRECTORY /*In order to produce error*/)
		{
			if( ::mkdir(path.c_str(), mode_) == -1)
			{
				throw TL_Exception<TL_OSUtil>("TL_OSUtil::CreateDir",
					GetErrorString(errno) + ": " + path);
			}
		}
		path.append("/");
	}
}


bool
TL_OSUtil::DeleteDir(const char* path_)
{
	// Report failure if path exists but we can't open it
	// (permissions problem, not a dir, etc)
	::DIR *pdir = ::opendir(path_);
	if (!pdir)
	{
		if(errno==ENOENT) return true;
		throw TL_Exception<TL_OSUtil>("TL_OSUtil::DeleteDir",
			GetErrorString(errno) );
	}

	struct dirent * pent;
	bool keepGoing(true);
	string deeper, path(path_);

	while (keepGoing && (pent = ::readdir(pdir)))
	{
		if (!strcmp(pent->d_name,".") || !strcmp(pent->d_name,".."))
			continue;

		deeper = path + "/" + pent->d_name;
		FileType type = FileExists( deeper.c_str() );

		if(type == DIRECTORY)
		{
			if ( !DeleteDir(deeper.c_str()) )
			{
				keepGoing = false;
				continue;
			}
		} else {
			if (::unlink(deeper.c_str()) == -1)
			{
				keepGoing = false;
				continue;
			}
		}
        }

        ::closedir(pdir);

        // If any files or dirs beneath have failed to be removed, this will fail
        if(::rmdir( path.c_str() ) == -1)
	{
		if(errno==ENOENT) return true;
		throw TL_Exception<TL_OSUtil>("TL_OSUtil::DeleteDir",
			GetErrorString(errno) );
	}

	return true;
}


} // end of ThreeLight namespace
