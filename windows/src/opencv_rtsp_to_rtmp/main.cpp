
#include <opencv2/highgui.hpp>
#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using namespace std;
using namespace cv;

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "opencv_world330d.lib")

int main(int argc, char *argv[])
{

	//char *inUrl = "rtsp:://test:test123456@192.168.1.64"; //hikon
	char *inUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov"; //big bunny
	//char *inUrl = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	//nginx-rtmp ֱ��������
	char *outUrl = "rtmp://192.168.1.104/live";
	
	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	// pixel convert context
	SwsContext *vsc = NULL;
	AVFrame *yuv = NULL;

	//������������
	AVCodecContext   *vc = NULL;

	try {
		av_register_all();

		/// 1. �����(opencv)
		cam.open(inUrl);

		if (!cam.isOpened()) {
			throw exception("cam open failed");
		}
		cout << inUrl << "cam open sucess" << endl;

		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);


		/// 2. init convert context
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, // src width, height
			AV_PIX_FMT_BGR24,
			inWidth, inHeight,// dst width, height, dst format
			AV_PIX_FMT_YUV420P,
			SWS_BICUBIC, // �ߴ�仯ʹ���㷨
			0,0,0);

		if (!vsc)
		{
			throw exception("sws_getCacheContext faied");
		}

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

			/// 3 rgb to yuv
			// ��������ݽṹ
			yuv = av_frame_alloc();
			yuv->format = AV_PIX_FMT_YUV420P;
			yuv->width = inWidth;
			yuv->height = inHeight;
			yuv->pts = 0;

			// ����yuv���������ݿռ�
			int ret = av_frame_get_buffer(yuv, 32);
			if (ret != 0)
			{
				char buf[1024] = {0};
				av_strerror(ret, buf, sizeof(buf));
				throw exception("buf");
			}

			//��������ݽṹ
			uint8_t  *indata[AV_NUM_DATA_POINTERS] = {0};
			// interlaced:BGR BGR BGR BGR
			// planed: indata[0]: BBBB  indata[1]: GGGG indata[2]:RRRR
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//һ�е����ݵ��ֽ�����width��
			insize[0] = frame.cols * frame.elemSize();
			int h = sws_scale(vsc, indata, insize, 0, frame.rows, // Դ����
				              yuv->data, yuv->linesize);

			if (h <= 0)
			{
				continue;
			}
			cout << h << " " << flush;

			/// 4 ��ʼ��������������
			// a �ҵ�������
			AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (!codec)
			{
				throw exception("can not finde h264 encoder!");
			}

			// b ����������������
			vc = avcodec_alloc_context3(codec);
			if (!vc)
			{
				throw exception("avcodec_alloc_context3 failed!");
			}

			// c �򿪱�����������

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

		if (vc)
		{
			avcodec_free_context(&vc);
		}
		cerr << ex.what() << endl;
	}
	
	
	getchar();
	return 0;
}