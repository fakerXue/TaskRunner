// ConsoleApplication6.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "TaskRunner.h"
using namespace x2lib;


int func(int a)
{
    int sum = 0;
    //while (true)
    {
        //sum += a;
        //Sleep(3000);
        printf("%d, %d\n", (int)SysUtil::GetCurrentTick(), a);
    }
    return 0;
}

int func2(int a)
{
    int sum = 0;
    //while (true)
    {
        //sum += a;
        //Sleep(6000);
        printf("%d, %d\n", (int)SysUtil::GetCurrentTick(), a);
    }
    return 0;
}


int main()
{

    TaskRunner tr;
    TaskRunner::TaskInfo ti(func, 0000);
    ti.Put(func2, 1111);
    ti.Put(func, 2222);
    ti.Put(func, 3333);
    tr.Start(true);
    tr.Load(ti, 0);
    TaskRunner::TaskInfo ti1(func, 4444);
    tr.Load(ti1, 0);
    TaskRunner::TaskInfo ti2(func, 555);
    tr.Load(ti2, 0);
    TaskRunner::TaskInfo ti3(func, 66666);
    tr.Load(ti3, 0);
    TaskRunner::TaskInfo ti4(func, 888);
    tr.Load(ti4, 0);

    while (true)
        Sleep(1000);
    tr.Stop(true, 30000);

    system("pause");

    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
