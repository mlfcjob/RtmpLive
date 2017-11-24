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

//获取cpu数量
static int XGetCpuNum()
{
#if defined WIN32 || defined _WIN32  // 64位
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
			av_frame_free(&yuv);  //不需要 yuv = NULL
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

	//初始化像素格式转换的上下文
	bool InitScale()
	{
		vsc = sws_getCachedContext(vsc, inWidth, inHeight, AV_PIX_FMT_BGR24,
			outWidth, outHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

		if (!vsc)
		{
			cout<<"sws_getCachedContext failed"<<endl;
			return false;
		}
		
		//初始化输出的数据结构
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;

		//分配yuv指向的数据空间
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

	// 编码器的初始化
	bool InitVideoCodec()
	{
		//a 找到编码器
		//b 创建编码器上下文
		if (!(vc = CreateCodec(AV_CODEC_ID_H264)))
		{
			return false;
		}

		// c 配置编码器参数
		vc->bit_rate = bitrate;  // 50kB
		vc->width = outWidth;
		vc->height = outHeight;
		vc->time_base = {1, fps};
		vc->framerate = {fps, 1};
		vc->gop_size = 50;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;

		// d 打开编码器上下文
		return OpenCodec(&vc);
	}

	// 音频编码器初始化
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

		// 打开音频编码器
		return OpenCodec(&ac);
	}

	// 音频编码
	AVPacket *EncodeAudio(AVFrame *pcm)
	{
		// pts运算
		// nb_sample / smaple_rate  = 一帧音频的秒数
		// timebase pts = sec * timebase.den
		pcm->pts = apts;
		apts += av_rescale_q(pcm->nb_samples, { 1, sampleRate }, ac->time_base);


		ret = avcodec_send_frame(ac, pcm);
		if (ret != 0)
		{
			return NULL;
		}

		av_packet_unref(&apack);
		ret = avcodec_receive_packet(ac, &apack);
		if (ret != 0)
		{
			return NULL;
		}
		cout << apack.size << " " << flush;
		return &apack;
	}

	AVPacket *EncodeVideo(AVFrame *frame)
	{
		av_packet_unref(&vpack);
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

		ret = avcodec_receive_packet(vc, &vpack);

		if (ret != 0 || vpack.size <= 0)
		{
			return NULL;
		}

		return &vpack;
	}

	AVCodecContext *GetCodecContext(void)
	{
		return vc;
	}

	//音频重采样上下文初始化 
	bool InitResample()
	{
		asc = NULL;
		asc = swr_alloc_set_opts(asc,
			av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt/*for aac*/, sampleRate,  //输出格式
			av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate, //输入格式
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

		// 音频重采样输出空间分配
		pcm = av_frame_alloc();
		pcm->format = outSampleFmt;
		pcm->channels = channels;
		pcm->channel_layout = av_get_default_channel_layout(channels);
		pcm->nb_samples = nbSample;  // 一帧音频一个通道的采样数量
		ret = av_frame_get_buffer(pcm, 0);  // 给pcm分配存储空间
		if (ret != 0)
		{
			XError(ret);
		}
		return true;
	}

	// 音频重采样
	AVFrame *Resample(char *data)
	{
		//重采样源数据
		const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		indata[0] = (uint8_t *)data;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples,  //输出参数：存储地址和样本数量
			indata, pcm->nb_samples);

		if (len <= 0)
		{
			return NULL;
		}
		return pcm;
	}

private:
	bool OpenCodec(AVCodecContext **c)
	{
		// 打开音频编码器
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
		//初始化编码器
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

		return c;
	}

	SwsContext *vsc = NULL;   // 像素格式转换上下文 C++ 11  linux: -std c++11
	AVFrame *yuv = NULL;      // 输出的YUV数据
	//AVCodecContext *vc = 0;   // 编码器上下文
	SwrContext *asc = NULL;   // 音频重采样上下文
	AVFrame *pcm = NULL;      // 重采样输出的pcm

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
		//注册所有编解码器
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
