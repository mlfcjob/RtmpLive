#include "XVideoCapture.h"
#include <opencv2/highgui.hpp>
#include <iostream>
#include <QMutex>
#include <list>

using namespace cv;
using namespace std;

#pragma comment(lib, "opencv_world330d.lib")

class CXVideoCapture : public XVideoCapture
{
public:
	void run()
	{
		cout << "视频抓取线程" << endl;
		
		Mat frame;
		while (!isExit)
		{
			if (!cam.read(frame))
			{
				QThread::msleep(1);
				continue;
			}

			if (frame.empty())
			{
				QThread::msleep(1);
				continue;
			}

			//确保数据是连续的
			fmutex.lock();
			for (int i = 0; i < filters.size(); i++)
			{
				Mat des;
				filters[i]->Filter(&frame, &des);
				frame = des;
			}
			fmutex.unlock();

			XData d((char*)frame.data, frame.cols * frame.rows * frame.elemSize(), GetCurTime());
			Push(d);	
		}
	}

	//开始录制视频
	bool Init(const char* inUrl)
	{
		Stop();
		cam.open(inUrl);
		if (!cam.isOpened())
		{
			cout << "cam open failed" << endl;
			return false;
		}
		
		width = cam.get(CAP_PROP_FRAME_WIDTH);
		height = cam.get(CAP_PROP_FRAME_HEIGHT);
		fps = cam.get(CAP_PROP_FPS);

		QThread::start();
		isExit = false;

		return true;
	}

	bool Init(int camIndex = 0)
	{
		Stop();
		cam.open(camIndex);
		if (!cam.isOpened())
		{
			cout << "cam open failed!" << endl;
			return false;
		}

		cout << camIndex << "open success" << endl;
		width = cam.get(CAP_PROP_FRAME_WIDTH);
		height = cam.get(CAP_PROP_FRAME_HEIGHT);
		fps = cam.get(CAP_PROP_FPS);

		if (fps == 0) {
			fps = 25;
		}

		return true;
	}

	// 关闭视频
	void Stop()
	{
		XDataThread::Stop();
		if(cam.isOpened())
		{ 
			cam.release();
		}

		if (frame.data) {
			frame.release();
		}
	}

	VideoCapture cam;
	Mat  frame;
private:

};

XVideoCapture::XVideoCapture()
{
}


XVideoCapture::~XVideoCapture()
{
}

XVideoCapture * XVideoCapture::Get(XVIDEO_TYPE type, unsigned char index)
{
	static CXVideoCapture xc[255];
	return &xc[index];
}



