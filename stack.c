#include "stack.h"
//    进行栈的初始化的函数
void initStack(pStack ps)
{
    ps->Top = (pNode)malloc(sizeof(Node));        //    分配内存空间给栈顶
    if (NULL == ps->Top)
    {
        printf("动态分配内存失败\n");
        exit(-1);
    }
    else
    {
        ps->Bottom = ps->Top;                    //    使栈底也指向栈顶空间
        ps->Top->pNext = NULL;                    //    栈顶指针置为NULL；
    }
    return ;
}

//    进行进栈操作的函数
int push(pStack ps,int data)
{
    pNode pNew = (pNode)malloc(sizeof(Node));    //    定义一个新节点，并分配内存空间
    if (NULL == pNew)
    {
        return 0;
    }
    pNew->member = data;                        //    把要进栈的数据赋给新节点的member成员
    pNew->pNext = ps->Top;                        //    使新节点的指针指向栈顶
    ps->Top = pNew;                                //    把新节点作为新栈顶
    TracePrintf(0, "push:%d\n", data);
    TracePrintf(0, "push:%d\n", (ps->Top)->member);
    return 1;
}

int peek(pStack ps) 
{
   if (empty(ps))                //判断栈是否为空，为空就不能进行出栈操作
    {
        exit(-1);
    }
    // TracePrintf(0, "%s", (ps->Top)->member);
   return (ps->Top)->member;
}


//    遍历栈的函数
void traverseStack(pStack ps)
{
    pNode pNew = ps->Top;
    while(pNew!= ps->Bottom)                //    只要栈顶不等于栈底，循环
    {
        TracePrintf(0, "traverseStack:%d ",pNew->member);            //    打印栈顶的成员member
        pNew = pNew->pNext;                //    栈顶指针向下移动一次
    }
    return ;
}

//    判断栈是否为空
int empty(pStack ps)
{
    if(ps->Top == ps->Bottom)    //    栈顶等于栈底，不就是栈中没数据么
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//    进行出栈操作函数
int pop(pStack ps)
{
    pNode pSwap = NULL;            
    int return_val;
    if (empty(ps))                //判断栈是否为空，为空就不能进行出栈操作
    {
        exit(-1);
    }
    else
    {
        return_val = ps->Top->member;    //    把栈顶的成员member的值赋给return_val做为函数返回值
        pSwap = ps->Top;                //    使pSwap指向栈顶
        ps->Top = ps->Top->pNext;        //    使栈顶指向栈顶下一个节点
        free(pSwap);                    //    释放以前的栈顶空间
        return return_val;
    }
}

//    清空栈的函数
void clear(pStack ps)
{
    pNode pNew = NULL;
    
    while (ps->Top != ps->Bottom)        //    栈顶和栈底不等，循环
    {
        pNew = ps->Top;                    //    使一个新节点和栈顶指向同一空间
        ps->Top = ps->Top->pNext;        //    使栈顶指向栈顶的下一个节点
        free(pNew);                        //    释放掉以前的栈顶空间
    }
    return ;
}