
#include <opencv2/highgui.hpp>
#include <iostream>
#include "XMediaEncode.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


using namespace std;
using namespace cv;

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "opencv_world330d.lib")


void XError(int ret)
{
	char buf[1024] = { 0 };
	av_strerror(ret, buf, sizeof(buf));
	throw exception("buf");
}

int main(int argc, char *argv[])
{

	//char *inUrl = "rtsp:://test:test123456@192.168.1.64"; //hikon
	//char *inUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov"; //big bunny
	char *inUrl = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	//char *inUrl = "F:\\FFOutput\\2504.mkv";
	//nginx-rtmp ֱ��������
	char *outUrl = "rtmp://192.168.103.139/live";

	XMediaEncode *me = XMediaEncode::Get();
	 
	//ע�����з�װ��
	av_register_all();

	//ע����������Э��
	avformat_network_init();

	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	//������������
	AVCodecContext   *vc = NULL;

	// rtmp flv ��װ��
	AVFormatContext  *ic = NULL;

	AVPacket pack;
	memset(&pack, 0, sizeof(pack));

	int vpts = 0;
	int ret;

	try {
		/// 1. �����(opencv)
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

		// ��ʼ����ʽת��������
		me->inWidth = inWidth;
		me->inHeight = inHeight;
		me->outWidth = inWidth;
		me->outHeight = inHeight;
		me->InitScale();


		/// 4 ��ʼ��������������
		// a �ҵ�������
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			throw exception("can not find h264 encoder!");
		}

		// b ����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			throw exception("avcodec_alloc_context3 failed!");
		}

		// c ���ñ���������
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;  //ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = 8;

		// only for video
		vc->bit_rate = 50 * 1024 * 8;      // bits size per sec after compressed : 50kB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = { 1, fps };  // pts * 1 / fps = second 
		vc->framerate = { fps, 1 };

		// ������Ĵ�С������֡һ���ؼ�֡��
		//���Խ��ѹ����Խ�ߣ�ͬʱ����Ҳ���������ʧ����������Ҳ�����
		vc->gop_size = 5;
		vc->max_b_frames = 0; // B ֡��Ϊ0�� ���������pts��dts�ͻ�һ��
		vc->pix_fmt = AV_PIX_FMT_YUV420P;

		//d �򿪱�����������
		ret = avcodec_open2(vc, NULL, NULL);
		if (ret != 0)
		{
			XError(ret);
		}

		cout << "avcodec Open 2 encoder success." << endl;

		/// 5 �����װ������Ƶ������
		// a �����װ��������
		ret = avformat_alloc_output_context2(&ic, 0, "flv", outUrl);
		if (ret != 0)
		{
			XError(ret);
		}
		
		// b �����Ƶ��
		AVStream *vs = avformat_new_stream(ic, NULL);
		if (!vs)
		{
			throw exception("avformat_new_stream failed");
		}
		vs->codecpar->codec_tag = 0;

		//c �ӱ��������Ʋ���
		avcodec_parameters_from_context(vs->codecpar, vc);
		av_dump_format(ic, 0, outUrl, 1);

		//6 ��rtmp  �������IO
		ret = avio_open(&ic->pb, outUrl, AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			XError(ret);
		}

		//  д���װͷ��Ϣ
		ret = avformat_write_header(ic, 0);
		if (ret < 0)
		{
			XError(ret);
		}

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
				continue;
			}

			/// h264 encode
			yuv->pts = vpts++;

			ret = avcodec_send_frame(vc, yuv);
			if (ret != 0)
			{
				continue;
			}

			ret = avcodec_receive_packet(vc, &pack);
			if (ret != 0 || pack.size > 0)
			{
				cout << "*" << pack.size << flush;
			} 
			else 
			{
				continue;
			}

			/// push media
			pack.pts = av_rescale_q(pack.pts, vc->time_base, vs->time_base);
			pack.dts = av_rescale_q(pack.dts, vc->time_base, vs->time_base);
			//av_write_frame();
			ret = av_interleaved_write_frame(ic, &pack);
			if (ret == 0)
			{
				cout << " #" << fflush;
			}
		}
	}
	catch(exception &ex) {
		if (cam.isOpened())
		{
			cam.release();
		}		

		if (vc)
		{	
			avio_closep(&ic->pb); //ic->pb = NULL;
			avcodec_free_context(&vc);
		}

		cerr << ex.what() << endl;
	}
	
	getchar();
	return 0;
}