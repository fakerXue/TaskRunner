
#include "SysUtil.h"

using namespace x2lib;


/*************************************************************************
** Desc		: º¯Êý
** Author	: faker@2016-11-18 20:51:07
**************************************************************************/


unsigned long long SysUtil::GetCurrentTick(bool isMill)
{
	unsigned long long tick = 0;
#ifdef __WIN32__
	struct __timeb64 tp;
	_ftime64(&tp);
	tick = ((unsigned long long)time(NULL) * 1000 + tp.millitm);
#elif defined __LINUX__
	struct timeval tmv;
	gettimeofday(&tmv, NULL);
	tick = (unsigned long long)time(NULL) * 1000 + tmv.tv_usec / 1000;
#endif 
	return (isMill ? tick : (tick/1000));
}
