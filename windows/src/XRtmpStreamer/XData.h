#pragma once 
class XData
{
public:
	XData();

	// �����ռ䲢����data����
	XData(char *data, int size, long long p = 0);
	~XData();

	char *data = 0;
	int  size = 0;
	long long pts = 0;
	void Drop();
};

//��ȡ��ǰʱ���  us
long long GetCurTime();

