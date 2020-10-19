/*************************************************************************
** Copyright(c) 2016-2020  faker
** All rights reserved.
** Name		: TaskRunner.h
** Desc		: 一个多功能任务机，支持串行、并行、延时、循环、跳转、任务卸载、事件通知
** Author	: faker@2016-10-26 19:58:35
*************************************************************************/

#ifndef _C056CD96_EE58_46B2_B6B7_E6164F91BE60
#define _C056CD96_EE58_46B2_B6B7_E6164F91BE60

#include "xCores.h"
#include "SysUtil.h"

namespace x2lib
{
    class TaskRunner
    {
    public:
        struct TaskInfo
        {
            /*************************************************************************
            ** Desc		: 构造一个任务对象
            ** Param	: [in] f 任务过程函数指针，参数个数不限制，但至少保证返回值为int
            **			  [in] args 任务函数参数
            ** Return	:
            ** Author	: faker@2020-10-19 09:58:07
            *************************************************************************/
            template<class F, class...Args>
            TaskInfo(F f, Args...args)
            {
                this->id = 0;
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

            // 添加一个执行体，返回当前执行体索引
            template<class F, class...Args>
            int Put(F f, Args...args)
            {
                this->vFuncs.push_back(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
                return this->vFuncs.size() - 1;
            }

            // 删除指定索引的执行体，返回剩余执行体个数
            int Del(int iFunc)
            {
                if (iFunc >= this->vFuncs.size())
                    return -1;
                this->vFuncs.erase(this->vFuncs.begin() + iFunc);
                return this->vFuncs.size();
            }

            // 得到执行体个数
            int Size()
            {
                return this->vFuncs.size();
            }

            int id; // 任务id（从1开始顺序增加）其余表示用户任务id【Load返回值】
            int iRet; // 执行结果，用户自定义
            unsigned long ulWait; // 延时（毫秒）

        private:
            int iState; // 0未执行，1正在执行，2已执行
            unsigned long ulTick; // 内部使用的时间戳
            std::vector<std::function<int(void)>> vFuncs; // std::function<int(void)>要求了添加的函数至少满足返回值为int，但参数个数和类型可以不限

            friend class TaskRunner;
        };

        class Listener
        {
        public:
            /*************************************************************************
            ** Desc		: 通知
            ** Param	: [in] pTaskInfo 任务体，通过iState成员判断是执行前还是执行后
            **			  [in,out] vCache 任务机共享数据区
            **			  [in] nCache 任务机共享数据区大小
            ** Return	:
            ** Author	: faker@2016-10-26 19:58:35
            *************************************************************************/
            virtual void Notify(const TaskInfo* pTaskInfo, void* vCache, int nCache) = 0;
        };

    public:
        /*************************************************************************
        ** Desc		: 构造函数
        ** Param	: [in] nCache 共享缓冲区大小
        **			  [in] pListener 监听器
        ** Return	:
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        TaskRunner(int nCache = 1024, Listener *pListener = NULL)
        {
            _iIDCounter_ = 0;
            m_nCache = nCache;
            if (m_nCache > 0)
            {
                m_vCache = calloc(1, m_nCache);
            }
            m_pListener = pListener;
            m_isRunning = false;
            m_isLoop = false;
            m_vecpThread.resize(1 + POOL_THREAD_COUNT);
            char szSigName[128] = { 0 };
            sprintf(szSigName, "TaskRunner_m_pSignal_%08x", &nCache);
            m_pSignal = new xCores::Signal(0, 1, szSigName);
            sprintf(szSigName, "TaskRunner_m_pSigExit_%08x", &nCache);
            m_pSigExit = new xCores::Signal(0, 1 + POOL_THREAD_COUNT, szSigName);
            m_pMutex = new xCores::Mutex();
            m_pMtxPool = new xCores::Mutex();
            sprintf(szSigName, "TaskRunner_m_pSigPool_%08x", &nCache);
            m_pSigPool = new xCores::Signal(0, POOL_THREAD_COUNT, szSigName);
            sprintf(szSigName, "TaskRunner_m_pSigPoolEnd_%08x", &nCache);
            m_pSigPoolEnd = new xCores::Signal(0, 1, szSigName);
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
        ** Desc		: 启动
        ** Param	: [in] isLoop 是否循环执行
        ** Return	: 是否操作成功
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
        ** Desc		: 停止
        ** Param	: [in] isClear 是否清除任务队列
        **            [in] nWaitMillSeconds 最长等待时长(≥0，50ms一单位），-1表示
        **              无限等待，直到当前任务执行结束
        ** Return	: 是否是超时返回
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        bool Stop(bool isClear, int nWaitMillSeconds)
        {
            if (!m_isRunning)
            {
                return false;
            }
            m_isRunning = false;

            // 激活串行和并行线程
            m_pSignal->Notify(1);
            m_pSigPool->Notify(POOL_THREAD_COUNT);

            // 等待，清理线程
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
                { // 完美结束，等待成功
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            //bool isTimeout = false;

            //bool isTimeout = m_pSigExit->Wait();
            for (auto& it : m_vecpThread)
            {
                if (it->joinable())
                {
                    it->detach();
                }
                delete it;
            }

            // 清除任务队列
            while (isClear && m_lsTaskInfo.size() > 0)
            {
                delete m_lsTaskInfo.back();
                m_lsTaskInfo.pop_back();
            }

            return (iSigCount < 1 + POOL_THREAD_COUNT);
        }

        /*************************************************************************
        ** Desc		: 加载一个任务，必须在Start之后使用
        ** Param	: [in] ti 是否循环执行
        **            [in] ms 延时时长，大于等于0
        ** Return	: 当前任务id
        ** Author	: faker@2016-10-26 19:58:35
        *************************************************************************/
        int Load(const TaskInfo& ti, unsigned long ms)
        {
            if (ti.vFuncs.size() < 1 || !m_isRunning/*必须先Start*/)
                return -1;

            m_pMutex->Lock();
            TaskInfo* pTaskInfo = new TaskInfo(ti);
            pTaskInfo->id = ++TaskRunner::_iIDCounter_;
            pTaskInfo->ulWait = ms;
            pTaskInfo->ulTick = (unsigned long)SysUtil::GetCurrentTick() + ms;
            // 排序，遵循靠后的后执行
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
        ** Desc		: 卸载任务
        ** Param	: [in] id id=-1表示移除所有任务，否则移除对应id的任务，返回成
        **              功移除的任务个数
        ** Return	: 卸载的任务的个数
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
        ** Desc		: 游标控制（跳转）
        ** Param	: [in] id 指定任务的id
        **            [in] isResetFollow 是否重置后续任务的状态（iState）
        ** Return	: 是否操作成功
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
        ** Desc		: 获取共享缓冲区
        ** Param	: [out] pnData 返回缓冲区大小
        ** Return	: 返回缓冲区地址
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

                // 任务开始前的“伪任务”
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

                    // 获取下一个未执行的任务
                    TaskRunner::TaskInfo* pFind = NULL;
                    for (std::list<TaskRunner::TaskInfo*>::iterator it = pThis->m_lsTaskInfo.begin(); it != pThis->m_lsTaskInfo.end(); ++it)
                    {
                        if ((*it)->iState == 0)
                        {
                            pFind = (*it);
                            pFind->iState = 1;
                            break;
                        }
                    }
                    pThis->m_pMutex->Unlock();

                    if (pFind != NULL)
                    {
                        long ulWaitms = (long)pFind->ulTick - (long)SysUtil::GetCurrentTick();
                        pThis->m_pSignal->Wait(ulWaitms > 0 ? ulWaitms : 0);
                        if (pFind->vFuncs.size() < 2)
                        { // 只有一个执行体时在当前线程执行
                            pFind->iRet = pFind->vFuncs[0]();
                        }
                        else
                        { // 多个执行体时在线程池执行
                            // 无需加锁
                            pThis->m_pvFuncs = &pFind->vFuncs;
                            pThis->m_vFuncId.clear();
                            for (int i = 0; i < pFind->vFuncs.size(); ++i)
                            {
                                pThis->m_vFuncId.push_back(i);
                            }
                            pThis->m_pSigPool->Notify(pThis->POOL_THREAD_COUNT);
                            for (int i = 0; i < pThis->m_pvFuncs->size(); ++i)
                            {
                                pThis->m_pSigPoolEnd->Wait();
                            }
                            pFind->iRet = 0;
                        }
                        pFind->iState = 2;
                        if (pThis->m_pListener) pThis->m_pListener->Notify(pFind, pThis->m_vCache, pThis->m_nCache);

                        if (!pThis->m_isLoop)
                        { // 清理已执行的任务
                            pThis->m_pMutex->Lock();
                            std::list<TaskRunner::TaskInfo*>::iterator it = std::find(pThis->m_lsTaskInfo.begin(), pThis->m_lsTaskInfo.end(), pFind);
                            if (it != pThis->m_lsTaskInfo.end())
                            {
                                delete pFind;
                                pThis->m_lsTaskInfo.erase(it);
                            }
                            pThis->m_pMutex->Unlock();
                        }
                    }
                    else if (pThis->m_isLoop)
                    { // 将所有任务置为未执行
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

            /** 线程池
                @param [in] pThis 当前对象指针
                @return
                @note
            */
            static int __ParallelExecer__(TaskRunner* pThis, int index)
            {
                std::thread* pThread = pThis->m_vecpThread[index];

                do
                {
                    pThis->m_pMtxPool->Lock();
                    if (pThis->m_pvFuncs == nullptr/*针对线程池刚启动*/ || pThis->m_vFuncId.size() < 1/*针对并行任务执行完成*/) // 暂时没有并行任务
                    {
                        pThis->m_pSigPool->Wait();
                        pThis->m_pMtxPool->Unlock();
                        continue;
                    }

                    int iFuncId = pThis->m_vFuncId.back();
                    pThis->m_vFuncId.pop_back();
                    pThis->m_pMtxPool->Unlock();

                    pThis->m_pvFuncs->at(iFuncId)();
                    pThis->m_pSigPoolEnd->Notify(1); // 通知1次【总共需m_pvFuncs->size()次】并行任务已经执行结束

                } while (pThis->m_isRunning); // 经测试，continue可以执行到此条件
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
        xCores::Mutex* m_pMutex;
        xCores::Signal* m_pSignal;
        xCores::Signal* m_pSigExit;
        xCores::Mutex* m_pMtxPool;
        xCores::Signal* m_pSigPool;
        xCores::Signal* m_pSigPoolEnd;
        const int POOL_THREAD_COUNT = 2;
        //const int FUNCS_MAX_COUNT = 4096;

        std::vector<std::thread*> m_vecpThread; // 0号为监视线程，1号为串行任务线程，余下为并行任务线程池
        std::list<TaskInfo*> m_lsTaskInfo;
        std::vector<std::function<int(void)>>* m_pvFuncs;
        std::vector<int> m_vFuncId;
        Listener *m_pListener;

    };

}

#endif

