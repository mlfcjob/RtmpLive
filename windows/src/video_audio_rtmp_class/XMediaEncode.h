#pragma once

struct AVFrame; //模仿Qt的做法
struct AVPacket;
class AVCodecContext;

enum XSampleFMT
{
	X_S16 = 1,
	X_FLATP = 8,
};


//音视频编码接口类 
class XMediaEncode
{
public:
	//输入参数
	int  inWidth = 1280;
	int  inHeight = 720;
	int  inPixSize = 3;
	int  channels = 2;
	int  sampleRate = 44100;
	int  inSampleFmt = X_S16;

	//输出参数
	int  outWidth = 1280;
	int  outHeight = 720;
	int  bitrate = 400000;  // bit
	int  fps = 25;
	int  outSampleFmt = X_FLATP;
	int  nbSample = 1024;

	//工厂生产方法
	static XMediaEncode * Get(unsigned char index = 0);

	//初始化像素格式转换的上下文
	virtual bool InitScale() = 0;

	virtual AVFrame* RGBToYUV(char *rgb) = 0;

	// 视频编码器初始化
	virtual bool InitVideoCodec() = 0;

	// 音频编码器初始化
	virtual bool InitAudioCodec() = 0;

	// 视频编码
	virtual AVPacket *EncodeVideo(AVFrame *yuv) = 0;

	// 获取编码器上下文
	virtual AVCodecContext *GetCodecContext(void) = 0;

	//音频重采样上下文初始化 
	virtual bool InitResample() = 0;

	// 音频重采样 (返回值无需调用者清理)
	virtual AVFrame *Resample(char *pcm) = 0;

	// 音频编码 (返回值无需调用者清理)
	virtual AVPacket *EncodeAudio(AVFrame *pcm) = 0;

	virtual ~XMediaEncode();

	AVCodecContext *ac = 0;  // 音频编码器上下文

protected:
	XMediaEncode();
};

