#include "XDataThread.h"
#include <string.h>

XDataThread::XDataThread()
{
}


XDataThread::~XDataThread()
{
}

//在列表结尾插入
void XDataThread::Push(XData d)
{
	mutex.lock();
	if (datas.size() > maxList)
	{
		datas.front().Drop();
		datas.pop_front();
	}
	datas.push_back(d);
	mutex.unlock();

}
//读取列表中最早的数据
XData XDataThread::Pop()
{
	mutex.lock();
	if (datas.empty())
	{
		mutex.unlock();
		return XData();
	}

	XData d = datas.front();
	datas.pop_front();
	mutex.unlock();

	return d;
}

//启动线程
bool XDataThread::Start()
{
	isExit = false;
	QThread::start();
	return true;
}

//退出线程并等待线程退出（阻塞函数）
void XDataThread::Stop()
{
	isExit = true;
	wait();
}

void XDataThread::Clear()
{
	mutex.lock();
	while (!datas.empty())
	{
		datas.front().Drop();
		datas.pop_front();
	}
	mutex.unlock();
}
