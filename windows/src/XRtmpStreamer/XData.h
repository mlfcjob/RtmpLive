#pragma once 
class XData
{
public:
	XData();

	// 创建空间并复制data内容
	XData(char *data, int size, long long p = 0);
	~XData();

	char *data = 0;
	int  size = 0;
	long long pts = 0;
	void Drop();
};

//获取当前时间戳  us
long long GetCurTime();

