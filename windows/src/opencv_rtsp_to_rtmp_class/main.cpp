
#include <opencv2/highgui.hpp>
#include <iostream>
#include "XMediaEncode.h"
#include "XRtmp.h"

using namespace std;
using namespace cv;

#pragma comment(lib, "opencv_world330d.lib")


int main(int argc, char *argv[])
{

	//char *inUrl = "rtsp:://test:test123456@192.168.1.64"; //hikon
	//char *inUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov"; //big bunny
	char *inUrl = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	//char *inUrl = "F:\\FFOutput\\2504.mkv";
	//nginx-rtmp 直播服务器
	char *outUrl = "rtmp://192.168.103.139/live";

	// 初始化编码器和像素格式转换对象
	XMediaEncode *me = XMediaEncode::Get(0);

	//封装和推流对象
	XRtmp *xr = XRtmp::Get(0);

	VideoCapture cam;
	Mat frame;
	namedWindow("video");


	try {
		/// 1. 打开像机(opencv)
		//cam.open(inUrl);
		cam.open(0);

		if (!cam.isOpened()) {
			throw exception("cam open failed");
		}
		cout << inUrl << "cam open sucess" << endl;

		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
		int fps = cam.get(CAP_PROP_FPS);

		if (fps <= 0) 
		{
			fps = 25;
		}

		// 3 初始化格式转换上下文
		me->inWidth = inWidth;
		me->inHeight = inHeight;
		me->outWidth = inWidth;
		me->outHeight = inHeight;
		me->fps = fps;
		me->InitScale();


		///4 初始化编码器上下文
		if (!me->InitVideoCodec())
		{
			throw exception("InitVieoCodec failed");
		}
		cout << "InitVideoCodec success " << endl;


		/// 5 输出封装器和视频流配置
		xr->Init(outUrl);

		//6 添加视频流
		xr->AddStream(me->GetCodecContext());

		//7 打开rtmp  网络输出IO	
		xr->SendHead();

		for (;;)
		{
			/// read rtsp frame ---> decode
			if (!cam.grab())
			{
				continue;
			}

			/// yuv to rgb
			if (!cam.retrieve(frame))
			{
				continue;
			}
			imshow("video", frame);
			waitKey(1);

			/// rgb to yuv
			me->inPixSize = frame.elemSize();
			AVFrame *yuv = me->RGBToYUV((char*)frame.data);

			if (!yuv)
			{
				cout << "RGBToYuv failed" << endl;
				continue;
			}

			/// h264 encode
			AVPacket *pack = me->EncodeVideo(yuv);
			if (!pack)
			{
				continue;
			}

			if (!xr->SendFrame(pack))
			{
				cout << "SendFrame failed" << endl;
				continue;
			}
		}
	}
	catch(exception &ex) {
		if (cam.isOpened())
		{
			cam.release();
		}		
		cerr << ex.what() << endl;
	}
	
	getchar();
	return 0;
}