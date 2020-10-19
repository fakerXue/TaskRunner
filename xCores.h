#ifndef _80AF3CDF_5146_4BD2_89B3_B6E7D9CECEC2
#define _80AF3CDF_5146_4BD2_89B3_B6E7D9CECEC2

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <ctype.h>  
#include <time.h>  
#include <sys/types.h>
#include <sys/timeb.h> 

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <list>
#include <set>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace x2lib
{

namespace xCores
{
	/*************************************************************************
	** Name		: ��java�ս���
	** Desc		: ��java��final�ؼ��֣�ʹ�÷�������̳�֮
	** Author	: faker@2017-5-27 10:27:25
	*************************************************************************/
	template <class T>
	class Final
	{
		friend T;
	private:
		Final() {}
		Final(const Final&) {}
	};

	template<class F>
	int GetFuncAddr(F f)
	{
		//void(Listener::*pfunc)(void) = (void(Listener::*)(void))&Listener::OnIsLogined;
		//return *(int*)&pfunc;
		return *(int*)(&f);
	}

	/*************************************************************************
	** Desc		: ��ƽ̨��3�ֶ��̹߳��ߣ�Engine��Mutex��Signal
	** Author	: faker@2017-4-17 21:23:25
	*************************************************************************/
	class Mutex
	{
	public:
		Mutex();
		virtual ~Mutex();

        bool Lock() const;
        bool Unlock() const;

	private:
		int m_mutex;
	};

	class Signal
	{
	public:
        Signal(int iInit, int iMax, char pName[128]=(char*)"");
		virtual ~Signal();

		/*************************************************************************
		** Desc		: �ȴ�
		** Param	: [in] msTimeout, -1��ʾ���޵ȴ�
		** Return	: ����ʱ�Ƿ񴥷��˳�ʱ
		** Author	: faker@2017-2-26 13:03:10
		*************************************************************************/
		virtual bool Wait(unsigned long msTimeout=-1) const;
		virtual bool Notify(int nCnt) const;

	private:
		int m_signal; // linux sem_id
		union semun{
			int val;
			struct semid_ds *buf;
			unsigned short *arry;
		};
	};
}

}

#endif

