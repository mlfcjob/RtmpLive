#pragma once


#include <QMutex>
#include "XData.h"
#include <list>
#include <QThread>

class XDataThread:public QThread
{
public:
	// �������б��б����ֵ������ɾ����ɵ����ݣ���ǰ�棩
	int  maxList = 100;

	//���б��β����
	virtual void Push(XData d);
	//��ȡ�б������������, ����������Ҫ����XData.Drop()����
	virtual XData Pop();

	//�����߳�
	virtual bool Start();
	//�˳��̲߳��ȴ��߳��˳�������������
	virtual void Stop();

	virtual void Clear();

	XDataThread();
	~XDataThread();

protected:
	//��Ž������� ������� �Ƚ��ȳ�
	std::list<XData> datas;

	//���������б��С
	int dataCount = 0;
	//������� datas
	QMutex mutex;

	//�����߳��˳�
	bool isExit = false;
};

