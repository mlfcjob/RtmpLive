#pragma once 
class XData
{
public:
	XData();

	// �����ռ䲢����data����
	XData(char *data, int size);
	~XData();

	char *data = 0;
	int  size = 0;
	void Drop();
};

