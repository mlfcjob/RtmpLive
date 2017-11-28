#include "XMediaEncode.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <iostream>

using namespace std;

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "swresample.lib")

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
			avcodec_free_context(&vc);
		}

		vpts = 0;
		apts = 0;
		av_packet_unref(&vpack);
		av_packet_unref(&apack);

		if (pcm)
		{
			av_frame_free(&pcm);
		}

		if (asc)
		{
			swr_free(&asc);
		}
	}

	//��ʼ�����ظ�ʽת����������
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

	XData RGBToYUV(XData d) 
	{
		XData r;
		r.pts = d.pts;
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		int insize[AV_NUM_DATA_POINTERS] = { 0 };

		indata[0] = (unsigned char*)d.data;
		insize[0] = inWidth * inPixSize;

		int h = sws_scale(vsc, indata, insize, 0, inHeight, yuv->data, yuv->linesize);
		if (h < 0)
		{
			cout << "sws_scale failed" << endl;
			return r;
		}

		yuv->pts = d.pts;

		r.data = (char*)yuv;
		int *p = yuv->linesize;
		while (*p)
		{
			r.size += (*p) * outHeight;
			p++;
		}

		return r;
	}

	// �������ĳ�ʼ��
	bool InitVideoCodec()
	{
		//a �ҵ�������
		//b ����������������
		if (!(vc = CreateCodec(AV_CODEC_ID_H264)))
		{
			return false;
		}

		// c ���ñ���������
		vc->bit_rate = bitrate;  // 50kB
		vc->width = outWidth;
		vc->height = outHeight;
		//vc->time_base = {1, fps};
		vc->framerate = {fps, 1};
		vc->gop_size = 50;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;

		// d �򿪱�����������
		return OpenCodec(&vc);
	}

	// ��Ƶ��������ʼ��
	bool InitAudioCodec()
	{
		if (!(ac = CreateCodec(AV_CODEC_ID_AAC)))
		{
			return false;
		}

		ac->bit_rate = 40000;
		ac->sample_rate = sampleRate;
		ac->sample_fmt = (AVSampleFormat)outSampleFmt;
		ac->channels = channels;
		ac->channel_layout = av_get_default_channel_layout(channels);

		// ����Ƶ������
		return OpenCodec(&ac);
	}

	long long lasta = -1;
	// ��Ƶ����
	XData EncodeAudio(XData frame)
	{
		XData r;
		if (frame.size <= 0 || !frame.data)
			return r;

		AVFrame *p = (AVFrame*)frame.data;
		if (lasta == p->pts)
		{
			p->pts += 1200;
		}

		lasta = p->pts;

		ret = avcodec_send_frame(ac, p);
		if (ret != 0)
		{
			return r;
		}

		av_packet_unref(&apack);
		ret = avcodec_receive_packet(ac, &apack);
		if (ret != 0)
		{
			return r;
		}
		cout << apack.size << " " << flush;

		r.data = (char*)&apack;
		r.size = apack.size;
		r.pts = frame.pts;
		return r;
	}

	XData EncodeVideo(XData frame)
	{
		av_packet_unref(&vpack);
		XData r;
		if (frame.size <= 0 || !frame.data)
			return r;

		AVFrame *p = (AVFrame*)frame.data;

		//frame->pts = vpts++;

		if (!yuv || !vc)
		{
			cout << "yuv or ic is NULL" << endl;
			return r;
		}

		ret = avcodec_send_frame(vc, p);
		if (ret != 0)
		{
			XError(ret);
			return r;
		}

		ret = avcodec_receive_packet(vc, &vpack);

		if (ret != 0 || vpack.size <= 0)
		{
			return r;
		}

		r.data = (char*)&vpack;
		r.size = vpack.size;
		r.pts = frame.pts;
		return r;
	}

	AVCodecContext *GetCodecContext(void)
	{
		return vc;
	}

	//��Ƶ�ز��������ĳ�ʼ�� 
	bool InitResample()
	{
		asc = NULL;
		asc = swr_alloc_set_opts(asc,
			av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt/*for aac*/, sampleRate,  //�����ʽ
			av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate, //�����ʽ
			0, 0);

		if (!asc)
		{
			cout << "swr_alloc_set_opts failed. " << endl;
			getchar();
			return -1;
		}

		int ret = swr_init(asc);
		if (ret != 0)
		{
			XError(ret);
		}

		cout << "swr init success." << endl;

		// ��Ƶ�ز�������ռ����
		pcm = av_frame_alloc();
		pcm->format = outSampleFmt;
		pcm->channels = channels;
		pcm->channel_layout = av_get_default_channel_layout(channels);
		pcm->nb_samples = nbSample;  // һ֡��Ƶһ��ͨ���Ĳ�������
		ret = av_frame_get_buffer(pcm, 0);  // ��pcm����洢�ռ�
		if (ret != 0)
		{
			XError(ret);
		}
		return true;
	}

	// ��Ƶ�ز���
	XData Resample(XData d)
	{
		XData r;
		//�ز���Դ����
		const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		indata[0] = (uint8_t *)d.data;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples,  //����������洢��ַ����������
			indata, pcm->nb_samples);

		if (len <= 0)
		{
			return r;
		}
		pcm->pts = d.pts;
		r.data = (char*)pcm;
		r.size = pcm->nb_samples * pcm->channels * 2;
		r.pts = d.pts;
		return r;
	}

private:
	bool OpenCodec(AVCodecContext **c)
	{
		// ����Ƶ������
		ret = avcodec_open2(*c, 0, 0);
		if (ret != 0)
		{
			XError(ret);
			avcodec_free_context(c);
			return false;
		}

		std::cout << "avcodec open success" << endl;
		return true;
	}

	AVCodecContext* CreateCodec(AVCodecID cid)
	{
		//��ʼ��������
		AVCodec  *codec = avcodec_find_encoder(cid);
		if (!codec)
		{
			cout << "avcodec_find_encoder failed" << endl;
			return NULL;
		}

		AVCodecContext *c = avcodec_alloc_context3(codec);
		if (!c)
		{
			cout << "avcodec_alloc_context3 failed" << endl;
			return NULL;
		}

		cout << "avcodec_alloc_context3 success" << endl;

		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		c->thread_count = XGetCpuNum();
		c->codec_id = cid;
		c->time_base = { 1, 1000000 };

		return c;
	}

	SwsContext *vsc = NULL;   // ���ظ�ʽת�������� C++ 11  linux: -std c++11
	AVFrame *yuv = NULL;      // �����YUV����
	//AVCodecContext *vc = 0;   // ������������
	SwrContext *asc = NULL;   // ��Ƶ�ز���������
	AVFrame *pcm = NULL;      // �ز��������pcm

	AVPacket vpack = {0};   // for video
	AVPacket apack = {0};   // for audio
	int ret = -1;
	int vpts = 0;
	int apts = 0;
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
