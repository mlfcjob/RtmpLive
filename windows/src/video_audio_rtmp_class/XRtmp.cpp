#include "XRtmp.h"

extern "C"
{
#include <libavformat/avformat.h>
}

#include <iostream>
#include <string>
using namespace std;

#pragma comment(lib, "avformat.lib")

class CXRtmp :public XRtmp {
public:
	void XError(int ret)
	{
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << buf << endl;
	}

	void Close()
	{
		if (ic)
		{
			avformat_close_input(&ic);
			vs = NULL;
		}

		vc = NULL;
		url = "";
	}

	//初始化封装器上下文
	bool Init(const char *url)
	{
		this->url = url;
		// a 输出封装器上下文
		int ret = avformat_alloc_output_context2(&ic, 0, "flv",url);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}
		return true;
	}

	//添加视频或者音频流
	int AddStream(const AVCodecContext *c)
	{	
		if (!c)
		{
			return -1;
		}

		// b 添加视频流
		AVStream *st = avformat_new_stream(ic, NULL);
		if (!st)
		{
			cout << "avformat new stream failed ." << endl;
			return -1;
		}
		st->codecpar->codec_tag = 0;

		// c 从编码器复制参数
		avcodec_parameters_from_context(st->codecpar, c);
		//av_dump_format(ic, 0, url.c_str(), 1);

		if (c->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vc = c;
			vs = st;
		}
		else if (c->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ac = c;
			as = st;
		}

		return st->index;
	}

	//打开rtmp网络IO， 发送封装头
	bool SendHead() 
	{
		int ret = avio_open(&ic->pb, url.c_str(), AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}

		//写入封装头信息
		ret = avformat_write_header(ic, 0);
		if (ret < 0)
		{
			XError(ret);
			return false;
		}

		return true;
	}

	bool SendFrame(XData d, int streamIndex)
	{
		if (!d.data || d.size <= 0) return false;

		AVPacket *pkt = (AVPacket *)d.data;
		pkt->stream_index = streamIndex;

		AVRational stime;
		AVRational dtime;

		// 判断是音频还是视频
		if (vs && vc &&pkt->stream_index == vs->index)
		{
			stime = vc->time_base;
			dtime = vs->time_base;
		} else if (ac && as &&pkt->stream_index == as->index)
		{
			stime = ac->time_base;
			dtime = as->time_base;
		} else {
			return false;
		}

		// push media
		pkt->pts = av_rescale_q(pkt->pts, stime, dtime);
		pkt->dts = av_rescale_q(pkt->dts, stime, dtime);
		pkt->duration = av_rescale_q(pkt->duration, stime, dtime);

		int ret = av_interleaved_write_frame(ic, pkt);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}

		return true;
	}
private:
	//rtmp flv封装器
	AVFormatContext *ic = NULL;

	// 视频编码器
	const AVCodecContext *vc = NULL;

	// 音频编码器
	const AVCodecContext *ac = NULL;

	// 视频流
	AVStream  *vs = NULL;
	// 音频流
	AVStream  *as = NULL;

	string url = "";
};


XRtmp *XRtmp::Get(unsigned char index)
{
	static CXRtmp cxr[256];

	static bool isFirst = true;
	if (isFirst)
	{
		//注册所有封装器
		av_register_all();

		//注册所有网络协议
		avformat_network_init();

		isFirst = false;
	}
	return &cxr[index];
}

XRtmp::XRtmp()
{
}


XRtmp::~XRtmp()
{
}

