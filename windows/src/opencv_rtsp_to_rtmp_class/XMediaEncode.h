#pragma once

struct AVFrame; //模仿Qt的做法
struct AVPacket;

//音视频编码接口类 
class XMediaEncode
{
public:
	//输入参数
	int  inWidth = 1280;
	int  inHeight = 720;
	int  inPixSize = 3;

	//输出参数
	int  outWidth = 1280;
	int  outHeight = 720;
	int  bitrate = 400000;  // bit
	int  fps = 25;

	//工厂生产方法
	static XMediaEncode * Get(unsigned char index = 0);

	//初始化像素格式转换的上下文
	virtual bool InitScale() = 0;

	virtual AVFrame* RGBToYUV(char *rgb) = 0;

	// 视频编码器初始化
	virtual bool InitVideoCodec() = 0;

	// 视频编码
	virtual AVPacket *EncodeVideo(AVFrame *yuv) = 0;

	virtual bool InitOutputContext(char *outUrl) = 0;

	virtual bool InitRtmp(char *outUrl) = 0;

	virtual bool PushMedia(AVPacket *pack)  = 0;

	virtual ~XMediaEncode();

protected:
	XMediaEncode();
};

