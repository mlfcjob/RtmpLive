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

	//��ʼ����װ��������
	bool Init(const char *url)
	{
		this->url = url;
		// a �����װ��������
		int ret = avformat_alloc_output_context2(&ic, 0, "flv",url);
		if (ret != 0)
		{
			XError(ret);
			return false;
		}
		return true;
	}

	//�����Ƶ������Ƶ��
	int AddStream(const AVCodecContext *c)
	{	
		if (!c)
		{
			return -1;
		}

		// b �����Ƶ��
		AVStream *st = avformat_new_stream(ic, NULL);
		if (!st)
		{
			cout << "avformat new stream failed ." << endl;
			return -1;
		}
		st->codecpar->codec_tag = 0;

		// c �ӱ��������Ʋ���
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

	//��rtmp����IO�� ���ͷ�װͷ
	bool SendHead() 
	{
		int ret = avio_open(&ic->pb, url.c_str(), AVIO_FLAG_WRITE);
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

	bool SendFrame(XData d, int streamIndex)
	{
		if (!d.data || d.size <= 0) return false;

		AVPacket *pkt = (AVPacket *)d.data;
		pkt->stream_index = streamIndex;

		AVRational stime;
		AVRational dtime;

		// �ж�����Ƶ������Ƶ
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
	//rtmp flv��װ��
	AVFormatContext *ic = NULL;

	// ��Ƶ������
	const AVCodecContext *vc = NULL;

	// ��Ƶ������
	const AVCodecContext *ac = NULL;

	// ��Ƶ��
	AVStream  *vs = NULL;
	// ��Ƶ��
	AVStream  *as = NULL;

	string url = "";
};


XRtmp *XRtmp::Get(unsigned char index)
{
	static CXRtmp cxr[256];

	static bool isFirst = true;
	if (isFirst)
	{
		//ע�����з�װ��
		av_register_all();

		//ע����������Э��
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

