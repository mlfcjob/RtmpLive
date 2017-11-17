#include "XMediaEncode.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <iostream>

using namespace std;

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")

#if defined WIN32 || defined _WIN32
#include <Windows.h>
#endif

//��ȡcpu����
static int XGetCpuNum()
{
#if defined WIN32 || defined _WIN32  // 64λ
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	return (int)sysinfo.dwNumberOfProcessors;
#elif defined __linux__
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined __APPLE__
	int numCPU = 0;
	int mib[4];
	size_t len = sizeof(numCPU);

	//set the mib for hw.ncpu
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;  // alternatively, try HW_CPU

	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if (numCPU < 1) {
		mib[1] = HW_NCPU;
		sysctl(mib, 2, &numCPU, &len, NULL, 0);

		if (numCPU < 1)
			numCPU = 1;
	}

	return (int)numCPU;
#else
	return 1;
#endif
}

class CXMediaEncode :public XMediaEncode
{
public:

	void XError(int ret)
	{
		char buf[1024] = {0};
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << buf << endl;
	}

	void Close()
	{
		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = NULL;
		}

		if (yuv)
		{
			av_frame_free(&yuv);  //����Ҫ yuv = NULL
		}

		if (vc)
		{
			avio_closep(&ic->pb); //ic->pb = NULL;
			avcodec_free_context(&vc);
		}

		vpts = 0;
		av_packet_unref(&pack);

	}

	bool InitScale()
	{
		vsc = sws_getCachedContext(vsc, inWidth, inHeight, AV_PIX_FMT_BGR24,
			outWidth, outHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

		if (!vsc)
		{
			cout<<"sws_getCachedContext failed"<<endl;
			return false;
		}
		
		//��ʼ����������ݽṹ
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;

		//����yuvָ������ݿռ�
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}
		return true;
	}

	AVFrame  *RGBToYUV(char *rgb) 
	{
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		int insize[AV_NUM_DATA_POINTERS] = { 0 };

		indata[0] = (unsigned char*)rgb;
		insize[0] = inWidth * inPixSize;

		int h = sws_scale(vsc, indata, insize, 0, inHeight, yuv->data, yuv->linesize);
		cout << "sws_scale: " << h << endl;

		if (h < 0)
		{
			return NULL;
		}

		return yuv;
	}

	// �������ĳ�ʼ��
	bool InitVideoCodec()
	{
		//a �ҵ�������
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);

		if (!codec)
		{
			cout << "avcodec_find_encoder failed. " << endl;
			return false;
		}

		// b ����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			cout << "avcodec_alloc_context3 failed" << endl;
			return false;
		}

		// c ���ñ���������
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;  //ȫ��ͷ����
		vc->codec_id = codec->id;
		vc->thread_count = XGetCpuNum();
		//only for video
		vc->bit_rate = bitrate;  // 50kB
		vc->width = outWidth;
		vc->height = outHeight;
		vc->time_base = {1, fps};
		vc->framerate = {fps, 1};
		vc->gop_size = 50;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;

		// d �򿪱�����������
		ret = avcodec_open2(vc, NULL, NULL);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}

		cout << "avcodec open 2 encoder success ." << endl;
		return true;
	}

	AVPacket *EncodeVideo(AVFrame *frame)
	{
		av_packet_unref(&pack);
		yuv->pts = vpts++;

		if (!yuv || !vc)
		{
			cout << "yuv or ic is NULL" << endl;
			return NULL;
		}

		ret = avcodec_send_frame(vc, frame);
		if (ret != 0)
		{
			XError(ret);
			return NULL;
		}

		ret = avcodec_receive_packet(vc, &pack);

		if (ret != 0 || pack.size <= 0)
		{
			return NULL;
		}

		return &pack;
	}


	bool InitOutputContext(char *outUrl)
	{
		if (!vc) {
			cout << "Video context has not been initlized" << endl;
			return false;
		}

		// a �����װ��������
		int ret = avformat_alloc_output_context2(&ic, 0, "flv", outUrl);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}

		// b �����Ƶ��
		vs = avformat_new_stream(ic, NULL);
		if (!vs)
		{
			cout << "avformat new stream failed ." << endl;
			return false;
		}
		vs->codecpar->codec_tag = 0;

		// c �ӱ��������Ʋ���
		avcodec_parameters_from_context(vs->codecpar, vc);
		av_dump_format(ic, 0, outUrl, 1);

		return true;
	}

	bool InitRtmp(char *outUrl)
	{
		if (!ic)
		{
			cout << "output context has not been initialized" << endl;
			return false;
		}
		ret = avio_open(&ic->pb, outUrl, AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}

		//д���װͷ��Ϣ
		ret = avformat_write_header(ic, 0);
		if (ret < 0)
		{
			XError(ret);
			return false;
		}

		return true;
	}


	bool PushMedia(AVPacket *pack)
	{
		if (!vs || !ic || !vc)
		{
			cout << "vs or ic vc is null" << endl;
			return false;
		}
		// push media
		pack->pts = av_rescale_q(pack->pts, vc->time_base, vs->time_base);
		pack->dts = av_rescale_q(pack->dts, vc->time_base, vs->time_base);

		ret = av_interleaved_write_frame(ic, pack);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}

		return true;

	}
private:
	SwsContext *vsc = NULL;  //���ظ�ʽת�������� C++ 11  linux: -std c++11
	AVFrame *yuv = NULL;   //�����YUV����
	AVCodecContext *vc = NULL;  // ������������
	AVFormatContext *ic = NULL; // rtmp flv��װͷ������
	AVStream *vs = NULL;
	AVPacket pack = {0};
	char *outUrl = "rtmp://192.168.103.139/live";
	int ret = -1;
	int vpts = 0;
};

XMediaEncode *XMediaEncode::Get(unsigned char index)
{
	static bool isFirst = true;
	if (isFirst)
	{
		//ע�����б������
		avcodec_register_all();
		isFirst = false;
	}

	static CXMediaEncode cxm[255];
	return &cxm[index];
}


XMediaEncode::XMediaEncode()
{
}


XMediaEncode::~XMediaEncode()
{
}
