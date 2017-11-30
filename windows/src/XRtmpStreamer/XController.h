#pragma once

#include <QThread>
#include <string>
#include "XDataThread.h"

class XController : public XDataThread
{
public:

	std::string outUrl;
	int camIndex = -1;
	std::string inUrl = "";
	std::string err = "";

	static XController *Get()
	{
		static XController xc;
		return &xc;
	}

	//设定美颜参数
	virtual bool Set(std::string key, double val);

	virtual bool Start();
	virtual void Stop();
	virtual ~XController();
	void run();

protected:
	int vindex = 0;  //视频流索引
	int aindex = 0;
	XController();
};

