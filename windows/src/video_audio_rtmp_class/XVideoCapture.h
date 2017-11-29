#pragma once

#include "XDataThread.h"
#include "XFilter.h"
#include <vector>

enum XVIDEO_TYPE {
	XVIDEO_OPENCV,
	XVIDEO_D3D,
};

class XVideoCapture:public XDataThread
{
public:
	int  width = 0;
	int  height = 0;
	int  fps = 0;

	static XVideoCapture *Get(XVIDEO_TYPE type = XVIDEO_OPENCV, unsigned char index = 0);
	virtual ~XVideoCapture();

	//��ʼ¼����Ƶ
	virtual bool Init(const char* inUrl) = 0;
	virtual bool Init(int camIndex = 0) = 0;
	
	// �ر���Ƶ
	virtual void Stop() = 0;

	void AddFilter(XFilter *f)
	{
		fmutex.lock();
		filters.push_back(f);
		fmutex.unlock();
	}

protected:
	QMutex fmutex;
	std::vector <XFilter*> filters;
	XVideoCapture();
};

