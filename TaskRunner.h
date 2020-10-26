/*************************************************************************
** Copyright(c) 2016-2025  faker
** All rights reserved.
** Name		: TaskRunner.h
** Desc		: һ���๦���������֧�ִ��С����С���ʱ��ѭ������ת������ж�ء��¼�֪ͨ
** Author	: faker@2020-10-26 11:40:58
*************************************************************************/

#ifndef _C056CD96_EE58_46B2_B6B7_E6164F91BE60
#define _C056CD96_EE58_46B2_B6B7_E6164F91BE60

#define _CRT_SECURE_NO_WARNINGS

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

    /*************************************������************************************/
#pragma region ������

    class Mutex
    {
    public:
        Mutex()
        {
            m_mutex = (int)::CreateMutex(NULL, FALSE, NULL);
        }

        virtual ~Mutex()
        {
            CloseHandle((HANDLE)m_mutex);
        }

        bool Lock() const
        {
            return WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)m_mutex, INFINITE);
        }

        bool Unlock() const
        {
            return 0 != ReleaseMutex((HANDLE)m_mutex);
        }

    private:
        int m_mutex;
    };

    class Signal
    {
    public:
        Signal(int iInit, int iMax, char pName[128] = (char*)"")
        {
            char szName[128] = { 0 };
            if (pName[0] == 0)
                sprintf(szName, "xCores::Signal_%08d_%08d", (int)this, (int)&m_signal);
            else
                sprintf(szName, pName);

            m_signal = (int)::CreateSemaphoreA(NULL, iInit, iMax, szName);
        }

        virtual ~Signal()
        {
            CloseHandle((HANDLE)m_signal);
        }

        virtual bool Wait(unsigned long msTimeout = -1) const
        {
            return WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_signal, msTimeout);
        }

        virtual bool Notify(int nCnt) const
        {
            return 0 != ::ReleaseSemaphore((HANDLE)m_signal, nCnt, NULL);
        }

    private:
        int m_signal; // linux sem_id
    };

    class SysUtil
    {
    public:
        static unsigned long long GetCurrentTick(bool isMill = true)
        {
            struct __timeb64 tp;
            _ftime64(&tp);
            unsigned long long tick = ((unsigned long long)time(NULL) * 1000 + tp.millitm);
            return (isMill ? tick : (tick / 1000));
        }
    };

#pragma endregion

    class TaskRunner
    {
    public:
        struct TaskInfo
        {
            /*************************************************************************
            ** Desc		: ����һ���������
            ** Param	: [in] f ������̺���ָ�룬�������������ƣ������ٱ�֤����ֵΪint
            **			  [in] args ����������
            ** Return	:
            ** Author	: faker@2020-10-19 09:58:07
            *************************************************************************/
            template<class F, class...Args>
            TaskInfo(F f, Args...args)
            {
                this->id = 0;
                this->vData = nullptr;
                this->iState = 0;
                this->ulWait = 0;
                this->ulTick = 0;
                this->iRet = 0;
                this->vFuncs.push_back(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            }
            virtual ~TaskInfo()
            {
                this->vFuncs.clear();
            };

            // ���һ��ִ���壬���ص�ǰִ��������
            template<class F, class...Args>
            int Put(F f, Args...args)
            {
                this->vFuncs.push_back(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
                return this->vFuncs.size() - 1;
            }

            // ɾ��ָ��������ִ���壬����ʣ��ִ�������
            int Del(int iFunc)
            {
                if (iFunc >= this->vFuncs.size())
                    return -1;
                this->vFuncs.erase(this->vFuncs.begin() + iFunc);
                return this->vFuncs.size();
            }

            // �õ�ִ�������
            int Size()
            {
                return this->vFuncs.size();
            }

            // ���ڰ󶨶�����û����������纯��args������Listener::Notify��ʹ��
            const TaskInfo& Bind(void* vData)
            {
                this->vData = vData;
                return *this;
            }

            int id; // ����id����1��ʼ˳�����ӣ������ʾ�û�����id��Load����ֵ��
            void* vData; // �ṩһ���ɹ��û�ʹ�õ�ͨ�ò���
            int iRet; // ִ�н�����û��Զ���
            unsigned long ulWait; // ��ʱ�����룩

        private:
            int iState; // 0δִ�У�1����ִ�У�2��ִ��
            unsigned long ulTick; // �ڲ�ʹ�õ�ʱ���
            std::vector<std::function<int(void)>> vFuncs; // std::function<int(void)>Ҫ������ӵĺ����������㷵��ֵΪint�����������������Ϳ��Բ���

            friend class TaskRunner;
        };

        class Listener
        {
        public:
            /*************************************************************************
            ** Desc		: ��������֪ͨ
            ** Param	: [in] pTaskInfo �����壬ͨ��iState��Ա�ж���ִ��ǰ����ִ�к�
            **			  [in,out] vCache ���������������
            **			  [in] nCache �����������������С
            ** Return	:
            ** Author	: faker@2016-10-26 19:58:35
            *************************************************************************/
            virtual void Notify(const TaskInfo* pTaskInfo, void* vCache, int nCache) = 0;

            /*************************************************************************
            ** Desc		: ���������ִ����֪ͨ
            ** Param	: [in] pTaskInfo �����壬ͨ��iState��Ա�ж���ִ��ǰ����ִ�к�
            **			  [in] iFuncId ��ǰ�����ִ����id
            **			  [in,out] vCache ���������������
            **			  [in] nCache �����������������С
            ** Return	:
            ** Author	: faker@2016-10-26 19:58:35
            *************************************************************************/
            virtual void Notify(const TaskInfo* pTaskInfo, int iFuncId, void* vCache, int nCache) = 0;
        };

    public:
        /*************************************************************************
        ** Desc		: ���캯��
        ** Param	: [in] nCache ����������С
        **			  [in] pListener ������
        ** Return	:
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        TaskRunner(Listener *pListener = nullptr, int nCache = 1024)
        {
            _iIDCounter_ = 0;
            m_nCache = nCache;
            if (m_nCache > 0)
            {
                m_vCache = calloc(1, m_nCache);
            }
            m_pListener = pListener;
            m_pTaskInfo = nullptr;
            m_isRunning = false;
            m_isLoop = false;
            m_vecpThread.resize(1 + POOL_THREAD_COUNT);
            char szSigName[128] = { 0 };
            sprintf(szSigName, "TaskRunner_m_pSignal_%08x", &nCache);
            m_pSignal = new Signal(0, 1, szSigName);
            sprintf(szSigName, "TaskRunner_m_pSigExit_%08x", &nCache);
            m_pSigExit = new Signal(0, 1 + POOL_THREAD_COUNT, szSigName);
            m_pMutex = new Mutex();
            m_pMtxPool = new Mutex();
            sprintf(szSigName, "TaskRunner_m_pSigPool_%08x", &nCache);
            m_pSigPool = new Signal(0, POOL_THREAD_COUNT, szSigName);
            sprintf(szSigName, "TaskRunner_m_pSigPoolEnd_%08x", &nCache);
            m_pSigPoolEnd = new Signal(0, 1, szSigName);
        };
        virtual ~TaskRunner()
        {
            if (m_vCache)
            {
                free(m_vCache);
            }
            delete m_pSignal;
            delete m_pMutex;
            delete m_pMtxPool;
            delete m_pSigPool; 
            delete m_pSigExit;
            delete m_pSigPoolEnd;
        };

        /*************************************************************************
        ** Desc		: ����
        ** Param	: [in] isLoop �Ƿ�ѭ��ִ��
        ** Return	: �Ƿ�����ɹ�
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        bool Start(bool isLoop)
        {
            if (m_isRunning)
            {
                return false;
            }
            m_isRunning = true;
            m_isLoop = isLoop;

            m_vecpThread[0] = new std::thread(__SerialExecer__, this, 0);
            for (size_t i = 1; i < m_vecpThread.size(); i++)
            {
                m_vecpThread[i] = new std::thread(__ParallelExecer__, this, i);
            }

            return true;
        }

        /*************************************************************************
        ** Desc		: ֹͣ
        ** Param	: [in] isClear �Ƿ�����������
        **            [in] nWaitMillSeconds ��ȴ�ʱ��(��0��50msһ��λ����-1��ʾ
        **              ���޵ȴ���ֱ����ǰ����ִ�н���
        ** Return	: �Ƿ��ǳ�ʱ����
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        bool Stop(bool isClear, int nWaitMillSeconds)
        {
            if (!m_isRunning)
            {
                return false;
            }
            m_isRunning = false;

            // ����кͲ����߳�
            m_pSignal->Notify(1);
            m_pSigPool->Notify(POOL_THREAD_COUNT);

            // �ȴ��������߳�
            int iSigCount = 0;
            for (int i = 0; i < nWaitMillSeconds; i += 50)
            {
                for (int i = 0; i < 1 + POOL_THREAD_COUNT; ++i)
                {
                    if (!m_pSigExit->Wait(1))
                    {
                        iSigCount++;
                    }
                }
                if (iSigCount >= 1 + POOL_THREAD_COUNT)
                { // �����������ȴ��ɹ�
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            for (auto& it : m_vecpThread)
            {
                if (it->joinable())
                {
                    it->detach();
                }
                delete it;
            }

            // ����������
            while (isClear && m_lsTaskInfo.size() > 0)
            {
                delete m_lsTaskInfo.back();
                m_lsTaskInfo.pop_back();
            }

            return (iSigCount < 1 + POOL_THREAD_COUNT);
        }

        /*************************************************************************
        ** Desc		: ����һ�����񣬱�����Start֮��ʹ��
        ** Param	: [in] ti �Ƿ�ѭ��ִ��
        **            [in] ms ��ʱʱ�������ڵ���0
        ** Return	: ��ǰ����id
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        int Load(const TaskInfo& ti, unsigned long ms = 0)
        {
            if (ti.vFuncs.size() < 1 || !m_isRunning/*������Start*/)
                return -1;

            m_pMutex->Lock();
            TaskInfo* pTaskInfo = new TaskInfo(ti);
            pTaskInfo->id = ++TaskRunner::_iIDCounter_;
            pTaskInfo->ulWait = ms;
            pTaskInfo->ulTick = (unsigned long)SysUtil::GetCurrentTick() + ms;
            // ������ѭ����ĺ�ִ��
            for (std::list<TaskInfo*>::iterator it = m_lsTaskInfo.begin(); ; ++it)
            {
                if (it == m_lsTaskInfo.end())
                {
                    m_lsTaskInfo.push_back(pTaskInfo);
                    break;
                }
                else if ((*it)->ulTick > pTaskInfo->ulTick)
                {
                    m_lsTaskInfo.insert(it, pTaskInfo);
                    break;
                }
            }
            m_pMutex->Unlock();
            m_pSignal->Notify(1);
            return TaskRunner::_iIDCounter_;
        };

        /*************************************************************************
        ** Desc		: ж������
        ** Param	: [in] id id=-1��ʾ�Ƴ��������񣬷����Ƴ���Ӧid�����񣬷��س�
        **              ���Ƴ����������
        ** Return	: ж�ص�����ĸ���
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        int Unload(int id = -1)
        {
            m_pMutex->Lock();
            int iCount = m_lsTaskInfo.size();
            for (std::list<TaskInfo*>::iterator it = m_lsTaskInfo.begin(); it != m_lsTaskInfo.end();)
            {
                if (id == -1 || id == (*it)->id)
                {
                    delete (*it);
                    it = m_lsTaskInfo.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            iCount -= m_lsTaskInfo.size();
            m_pMutex->Unlock();
            return iCount;
        };

        /*************************************************************************
        ** Desc		: �α���ƣ���ת��
        ** Param	: [in] id ָ�������id
        **            [in] isResetFollow �Ƿ����ú��������״̬��iState��
        ** Return	: �Ƿ�����ɹ�
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        bool Seek(int id, bool isResetFollow)
        {
            bool isFind = false;
            m_pMutex->Lock();
            for (auto& it : m_lsTaskInfo)
            {
                if (id == it->id)
                {
                    it->iState = 0;
                    isFind = true;
                }
                if (isResetFollow && isFind)
                {
                    it->iState = 0;
                }
            }
            m_pMutex->Unlock();
            return isFind;
        };

        /*************************************************************************
        ** Desc		: ��ȡ��������
        ** Param	: [out] pnData ���ػ�������С
        ** Return	: ���ػ�������ַ
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        void* GetCache(int* pnData = NULL)
        {
            if (pnData)
            {
                *pnData = m_nCache;
            }
            return m_vCache;
        };

    protected:
            static int __SerialExecer__(TaskRunner* pThis, int index)
            {
                std::thread* pThread = pThis->m_vecpThread[index];

                // ����ʼǰ�ġ�α����
                if (pThis->m_pListener) pThis->m_pListener->Notify(NULL, pThis->m_vCache, pThis->m_nCache);

                do
                {
                    pThis->m_pMutex->Lock();
                    if (pThis->m_lsTaskInfo.size() < 1)
                    {
                        pThis->m_pMutex->Unlock();
                        pThis->m_pSignal->Wait();
                        continue;
                    }

                // ��ȡ��һ��δִ�е�����
                pThis->m_pTaskInfo = nullptr;
                for (std::list<TaskRunner::TaskInfo*>::iterator it = pThis->m_lsTaskInfo.begin(); it != pThis->m_lsTaskInfo.end(); ++it)
                {
                    if ((*it)->iState == 0)
                    {
                        pThis->m_pTaskInfo = (*it);
                        pThis->m_pTaskInfo->iState = 1;
                        break;
                    }
                }
                pThis->m_pMutex->Unlock();

                if (pThis->m_pTaskInfo != NULL)
                {
                    long ulWaitms = (long)pThis->m_pTaskInfo->ulTick - (long)SysUtil::GetCurrentTick();
                    pThis->m_pSignal->Wait(ulWaitms > 0 ? ulWaitms : 0);
                    if (pThis->m_pTaskInfo->vFuncs.size() < 2)
                    { // ֻ��һ��ִ����ʱ�ڵ�ǰ�߳�ִ��
                        pThis->m_pTaskInfo->iRet = pThis->m_pTaskInfo->vFuncs[0]();
                    }
                    else
                    { // ���ִ����ʱ���̳߳�ִ��
                        // �������
                        pThis->m_pvFuncs = &pThis->m_pTaskInfo->vFuncs;
                        pThis->m_vFuncId.clear();
                        for (int i = 0; i < pThis->m_pTaskInfo->vFuncs.size(); ++i)
                        {
                            pThis->m_vFuncId.push_back(i);
                        }
                        pThis->m_pSigPool->Notify(pThis->POOL_THREAD_COUNT);
                        for (int i = 0; i < pThis->m_pvFuncs->size(); ++i)
                        {
                            pThis->m_pSigPoolEnd->Wait();
                        }
                        pThis->m_pTaskInfo->iRet = 0;
                    }
                    pThis->m_pTaskInfo->iState = 2;
                    if (pThis->m_pListener) pThis->m_pListener->Notify(pThis->m_pTaskInfo, pThis->m_vCache, pThis->m_nCache);

                    if (!pThis->m_isLoop)
                    { // ������ִ�е�����
                        pThis->m_pMutex->Lock();
                        std::list<TaskRunner::TaskInfo*>::iterator it = std::find(pThis->m_lsTaskInfo.begin(), pThis->m_lsTaskInfo.end(), pThis->m_pTaskInfo);
                        if (it != pThis->m_lsTaskInfo.end())
                        {
                            delete pThis->m_pTaskInfo;
                            pThis->m_lsTaskInfo.erase(it);
                        }
                        pThis->m_pMutex->Unlock();
                    }
                    pThis->m_pTaskInfo = nullptr;
                }
                else if (pThis->m_isLoop)
                { // ������������Ϊδִ��
                    pThis->m_pMutex->Lock();
                    for (auto& it : pThis->m_lsTaskInfo)
                    {
                        it->iState = 0;
                        it->ulTick = (unsigned long)SysUtil::GetCurrentTick() + it->ulWait;
                        it->iRet = 0;
                    }
                    pThis->m_pMutex->Unlock();
                }

                } while (pThis->m_isRunning);
                pThis->m_pSigExit->Notify(1);

                return 0;
            }

            /** �̳߳�
                @param [in] pThis ��ǰ����ָ��
                @return
                @note
            */
            static int __ParallelExecer__(TaskRunner* pThis, int index)
            {
                std::thread* pThread = pThis->m_vecpThread[index];

                do
                {
                    pThis->m_pMtxPool->Lock();
                    if (pThis->m_pvFuncs == nullptr/*����̳߳ظ�����*/ || pThis->m_vFuncId.size() < 1/*��Բ�������ִ�����*/) // ��ʱû�в�������
                    {
                        pThis->m_pSigPool->Wait();
                        pThis->m_pMtxPool->Unlock();
                        continue;
                    }

                    int iFuncId = pThis->m_vFuncId.back();
                    pThis->m_vFuncId.pop_back();
                    pThis->m_pMtxPool->Unlock();

                pThis->m_pvFuncs->at(iFuncId)();
                if (pThis->m_pListener) pThis->m_pListener->Notify(pThis->m_pTaskInfo, iFuncId,pThis->m_vCache, pThis->m_nCache);
                pThis->m_pSigPoolEnd->Notify(1); // ֪ͨ1�Ρ��ܹ���m_pvFuncs->size()�Ρ����������Ѿ�ִ�н���

                } while (pThis->m_isRunning); // �����ԣ�continue����ִ�е�������
                pThis->m_pSigExit->Notify(1);

                return 0;
            };

    protected:
        bool m_isRunning;
        bool m_isLoop;
        int m_nCache;
        void* m_vCache;
        int _iIDCounter_;

    private:
        Mutex* m_pMutex;
        Signal* m_pSignal;
        Signal* m_pSigExit;
        Mutex* m_pMtxPool;
        Signal* m_pSigPool;
        Signal* m_pSigPoolEnd;
        const int POOL_THREAD_COUNT = 4;

        std::vector<std::thread*> m_vecpThread; // 0��Ϊ���������̣߳�����Ϊ���������̳߳�
        std::list<TaskInfo*> m_lsTaskInfo;
        TaskRunner::TaskInfo* m_pTaskInfo; // ��ǰ����ִ�е�����
        std::vector<std::function<int(void)>>* m_pvFuncs; // ��ǰ���������funcer
        std::vector<int> m_vFuncId; // ��ǰ���������funcer������
        Listener *m_pListener;

    };

}

#endif

