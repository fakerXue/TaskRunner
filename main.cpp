#include "TaskRunner.h"

using namespace x2lib;


// ģ����ͨ����
int func01(int a, int b)
{
    return a + b;
}

// ģ���ʱ����
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

    // ͨ�� Listener �̳�
    virtual void Notify(const TaskRunner::TaskInfo * pTaskInfo, void * vCache, int nCache) override
    {
        if (!pTaskInfo)
        {
            printf("--------------------------����ʼ-----------------------\n");
            return;
        }
        printf("Tick=%d, Notify[task_id=%d]: iRet=%d\n", (int)SysUtil::GetCurrentTick(), pTaskInfo->id, pTaskInfo->iRet);
        if (pTaskInfo->vData != nullptr)
        { // ����6
            // ��ת����
            ((TaskRunner*)pTaskInfo->vData)->Seek(1, true); //��ʹ�ô���6��Զ�޷�ִ��ti04
            printf("Seek->1\n");
        }
    }
};

int main()
{
    Notifier notifier;
    TaskRunner tr(1024, &notifier);

    do
    {
        int iItem = 0;
        printf("\n\n��������ţ�");
        scanf("%d", &iItem);

        switch (iItem)
        {
        case 1:// 1.��ʱ������ͨ������ѭ����2������
        {
            tr.Start(true);
            TaskRunner::TaskInfo ti(func01, 1, 2);
            tr.Load(ti);
        }break;
        case 2:// 2.��ʱ��-��Ա������ѭ����1������
        {
            tr.Start(true);
            A a;
            TaskRunner::TaskInfo ti(&A::memfunc, &a, 10);
            tr.Load(ti);
        }break;
        case 3:// 3.���������
        {
            tr.Start(false);
            TaskRunner::TaskInfo ti01(func01, 1, 2);
            TaskRunner::TaskInfo ti02(func02, 3000);
            tr.Load(ti01);
            tr.Load(ti02);
        }break;
        case 4:// 4.���������
        {
            tr.Start(false);
            TaskRunner::TaskInfo ti(func01, 1, 2);
            ti.Put(func02, 3000);
            tr.Load(ti);
        }break;
        case 5:// 2.���в��������
        {
            tr.Start(false);
            TaskRunner::TaskInfo ti01(func01, 1, 2);
            TaskRunner::TaskInfo ti02(func02, 3000);
            ti02.Put(func02, 1000);
            ti02.Put(func02, 4000);
            TaskRunner::TaskInfo ti03(func01, 1, 2);
            tr.Load(ti01);
            tr.Load(ti02);
            tr.Load(ti03);
        }break;
        case 6:// 6.��תִ��
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
            tr.Load(ti04);
        }break;
        default: break;
        }
        getchar();
        system("pause");
        tr.Stop(true, 3000);

    } while (true);

    return 0;
}

