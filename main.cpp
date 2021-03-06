#include "TaskRunner.h"

using namespace x2lib;


// 模拟普通操作
int func01(int a, int b)
{
    return a + b;
}

// 模拟耗时操作
int func02(int ms)
{
    Sleep(ms);
    return ms;
}

class A
{
public:
    int memfunc(int i)
    {
        return i*2;
    }

private:
    int sum;
};

class Notifier : public TaskRunner::Listener
{
public:

    // 通过 Listener 继承
    virtual void Notify(const TaskRunner::TaskInfo * pTaskInfo, void * vCache, int nCache) override
    {
        if (!pTaskInfo)
        {
            printf("--------------------------任务开始-----------------------\n");
            return;
        }
        printf("Tick=%d, Notify[task_id=%d]: iRet=%d\n", (int)SysUtil::GetCurrentTick(), pTaskInfo->id, pTaskInfo->iRet);
        if (pTaskInfo->vData != nullptr)
        { // 代号6
            // 跳转测试
            ((TaskRunner*)pTaskInfo->vData)->Seek(1, true); //将使得代号6永远无法执行ti04
            printf("Seek->1\n");
        }
    }

    // 通过 Listener 继承
    virtual void Notify(const TaskRunner::TaskInfo * pTaskInfo, int iFuncId, void * vCache, int nCache) override
    {
    }
};

int main()
{
    Notifier notifier;
    TaskRunner tr(&notifier); // 使用Notifier监听任务机执行

    do
    {
        int iItem = 0;
        printf("\n\n请输入代号：");
        scanf("%d", &iItem);

        switch (iItem)
        {
        case 1:// 1.定时器：普通函数、循环、2个参数
        {
            tr.Start(true); // 循环执行
            TaskRunner::TaskInfo ti(func01, 1, 2);
            tr.Load(ti);
        }break;
        case 2:// 2.定时器-成员函数、循环、1个参数
        {
            tr.Start(true); // 循环执行
            A a;
            TaskRunner::TaskInfo ti(&A::memfunc, &a, 10);
            tr.Load(ti);
        }break;
        case 3:// 3.串行任务机
        {
            tr.Start(true); // 循环执行
            TaskRunner::TaskInfo ti01(func01, 1, 2);
            TaskRunner::TaskInfo ti02(func02, 3000);
            tr.Load(ti01);
            tr.Load(ti02);
        }break;
        case 4:// 4.并行任务机
        {
            tr.Start(false);
            TaskRunner::TaskInfo ti(func01, 1, 2);
            ti.Add(func02, 3000); // 此时ti有两个执行体，func01和func02并行执行
            tr.Load(ti);
        }break;
        case 5:// 5.串行并行任务机
        {
            tr.Start(false);
            TaskRunner::TaskInfo ti01(func01, 1, 2);
            TaskRunner::TaskInfo ti02(func02, 3000);
            ti02.Add(func02, 1000);
            ti02.Add(func02, 4000);
            TaskRunner::TaskInfo ti03(func01, 1, 2);
			// ti01，ti02，ti03串行执行，而每个ti里的执行体为并行执行
            tr.Load(ti01);
            tr.Load(ti02);
            tr.Load(ti03);
        }break;
        case 6:// 6.跳转执行
        {
            tr.Start(true);
            TaskRunner::TaskInfo ti01(func01, 1, 2);
            TaskRunner::TaskInfo ti02(func01, 3, 4);
            TaskRunner::TaskInfo ti03([&](int a)->int{
                return a;
            },555);
            TaskRunner::TaskInfo ti04(func02, 1000);
            tr.Load(ti01);
            tr.Load(ti02);
            tr.Load(ti03.Bind(&tr));
            tr.Load(ti04); // 由于在Notifier::Notify里调用了Seek，因而ti04永远无法执行
        }break;
        default: break;
        }
        getchar();
		system("pause");
        tr.Stop(true, 3000);
		printf("\n=======================\n");

    } while (true);

    return 0;
}

