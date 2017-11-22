#pragma once

struct AVFrame; //ģ��Qt������
struct AVPacket;
class AVCodecContext;

enum XSampleFMT
{
	X_S16 = 1,
	X_FLATP = 8,
};


//����Ƶ����ӿ��� 
class XMediaEncode
{
public:
	//�������
	int  inWidth = 1280;
	int  inHeight = 720;
	int  inPixSize = 3;
	int  channels = 2;
	int  sampleRate = 44100;
	int  inSampleFmt = X_S16;

	//�������
	int  outWidth = 1280;
	int  outHeight = 720;
	int  bitrate = 400000;  // bit
	int  fps = 25;
	int  outSampleFmt = X_FLATP;
	int  nbSample = 1024;

	//������������
	static XMediaEncode * Get(unsigned char index = 0);

	//��ʼ�����ظ�ʽת����������
	virtual bool InitScale() = 0;

	virtual AVFrame* RGBToYUV(char *rgb) = 0;

	// ��Ƶ��������ʼ��
	virtual bool InitVideoCodec() = 0;

	// ��Ƶ��������ʼ��
	virtual bool InitAudioCodec() = 0;

	// ��Ƶ����
	virtual AVPacket *EncodeVideo(AVFrame *yuv) = 0;

	// ��ȡ������������
	virtual AVCodecContext *GetCodecContext(void) = 0;

	//��Ƶ�ز��������ĳ�ʼ�� 
	virtual bool InitResample() = 0;

	// ��Ƶ�ز��� (����ֵ�������������)
	virtual AVFrame *Resample(char *pcm) = 0;

	// ��Ƶ���� (����ֵ�������������)
	virtual AVPacket *EncodeAudio(AVFrame *pcm) = 0;

	virtual ~XMediaEncode();

	AVCodecContext *ac = 0;  // ��Ƶ������������

protected:
	XMediaEncode();
};

