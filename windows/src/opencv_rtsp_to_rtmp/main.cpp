
#include <opencv2/highgui.hpp>
#include <iostream>
extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using namespace std;
using namespace cv;

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "opencv_world330d.lib")

int main(int argc, char *argv[])
{

	//char *inUrl = "rtsp:://test:test123456@192.168.1.64"; //hikon
	char *inUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov"; //big bunny
	//char *inUrl = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	//nginx-rtmp 直播服务器
	char *outUrl = "rtmp://192.168.1.104/live";
	
	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	// pixel convert context
	SwsContext *vsc = NULL;


	try {
		// 1. 打开像机(opencv)
		cam.open(inUrl);

		if (!cam.isOpened()) {
			throw exception("cam open failed");
		}
		cout << inUrl << "cam open sucess" << endl;

		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);


		// 2. init convert context
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, // src width, height
			AV_PIX_FMT_BGR24,
			inWidth, inHeight,// dst width, height, dst format
			AV_PIX_FMT_YUV420P,
			SWS_BICUBIC, // 尺寸变化使用算法
			0,0,0);

		if (!vsc)
		{
			throw exception("sws_getCacheContext faied");
		}

		//3. 读取帧
		for (;;)
		{
			// read rtsp frame ---> decode
			if (!cam.grab())
			{
				continue;
			}

			// yuv to rgb
			if (!cam.retrieve(frame))
			{
				continue;
			}
			imshow("video", frame);
			waitKey(1);

			// rgb to yuv

			// h264 encode

			// push media
		}
	}
	catch(exception &ex) {
		if (cam.isOpened())
		{
			cam.release();
		}		

		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = NULL;
		}

		cerr << ex.what() << endl;
	}
	
	

	
	getchar();
	return 0;
}