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

	//初始化所有的封装和解封装 flv mp4 mov mp3
	av_register_all();

	//初始化网络库
	avformat_network_init();

	/////////////////////////////////////////////////////////////////////////////////////////
	//// 输入流
	//1 打开文件，解封装
	// 输入封装上下文
	AVFormatContext *ictx = NULL;

	//打开文件，解封协议头
	int re = avformat_open_input(&ictx, inUrl, 0, 0);
	if (re != 0) {
		return XError(re);
	}

	cout << "open file " << inUrl << "success" << endl;

	// 获取音频视频流信息
	re = avformat_find_stream_info(ictx, NULL);
	if (re != 0) {
		return XError(re);
	}

	av_dump_format(ictx, 0, inUrl, 0);
	//////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////////////////////
	//// 创建输出流上下文
	AVFormatContext *octx = NULL;
	re = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
	if (!octx) {
		return XError(re);
	}

	cout << "octx create success" << endl;
	
	//配置输出流
	//遍历输入的AVStream
	for (int i = 0; i < ictx->nb_streams; i++) {
		// 创建输出流
		AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
		if (!out) {
			return XError(0);
		}

		//复制配置信息,用于mp4
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
