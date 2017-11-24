#pragma once 
class XData
{
public:
	XData();

	// 创建空间并复制data内容
	XData(char *data, int size);
	~XData();

	char *data = 0;
	int  size = 0;
	void Drop();
};

