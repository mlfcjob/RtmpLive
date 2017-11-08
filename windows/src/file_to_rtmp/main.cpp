#include <iostream>

using namespace std;

extern "C" {
#include "libavformat/avformat.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

int XError(int errNum)
{
	char buf[1024] = { 0 };
	av_strerror(errNum, buf, sizeof(buf));
	cout << buf << endl;
	getchar();
	return -1;
}

int main(int argc, char *argv[])
{
	char *inUrl = "swxf.flv";
	char *outUrl = "rtmp://192.168.103.139/live";

	//��ʼ�����еķ�װ�ͽ��װ flv mp4 mov mp3
	av_register_all();

	//��ʼ�������
	avformat_network_init();

	/////////////////////////////////////////////////////////////////////////////////////////
	//// ������
	//1 ���ļ������װ
	// �����װ������
	AVFormatContext *ictx = NULL;

	//���ļ������Э��ͷ
	int re = avformat_open_input(&ictx, inUrl, 0, 0);
	if (re != 0) {
		return XError(re);
	}

	cout << "open file " << inUrl << "success" << endl;

	// ��ȡ��Ƶ��Ƶ����Ϣ
	re = avformat_find_stream_info(ictx, NULL);
	if (re != 0) {
		return XError(re);
	}

	av_dump_format(ictx, 0, inUrl, 0);
	//////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////////////////////
	//// ���������������
	AVFormatContext *octx = NULL;
	re = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
	if (!octx) {
		return XError(re);
	}

	cout << "octx create success" << endl;
	
	//���������
	//���������AVStream
	for (int i = 0; i < ictx->nb_streams; i++) {
		// ���������
		AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
		if (!out) {
			return XError(0);
		}

		//����������Ϣ,����mp4
		//re = avcodec_copy_context(out->codec, ictx->streams[i]->codec);
		re = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
		out->codec->codec_tag = 0;
	}

	av_dump_format(octx, 0, outUrl, 1);
	///////////////////////////////////////////////////////////////////////////////////////////

	cout << "file to rtmp test" << endl;
	getchar();

	return 0;
}
