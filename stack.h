#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <comp421/filesystem.h>
//    定义一个节点的结构
typedef struct node
{
    int member;            //数据域
    struct node * pNext;//指针域
}Node,*pNode;

//    定义一个栈结构
typedef struct stack
{
    pNode Top;            //栈顶
    pNode Bottom;        //栈底
}Stack,* pStack;

void initStack(pStack );        //    初始化栈的函数
int push(pStack ,int);            //    进行压栈操作的函数
int peek(pStack); 
void traverseStack(pStack );    //    遍历栈函数
int empty(pStack );            //    判断栈是否为空的函数
int pop(pStack );                //    进行出栈操作的函数
void clear(pStack );            //    清空栈的函数