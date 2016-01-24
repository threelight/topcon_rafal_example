#ifndef __TL_NOCOPY__
#define __TL_NOCOPY__


namespace ThreeLight
{


class TL_NoCopy
{
protected:
	TL_NoCopy(){};
	
	virtual ~TL_NoCopy(){};

private:
	TL_NoCopy(const TL_NoCopy&);
	
	TL_NoCopy& operator=(const TL_NoCopy&);
};


} // end of ThreeLine namespace


#endif
