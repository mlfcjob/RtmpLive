#pragma once

#include "XData.h"
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

	virtual XData RGBToYUV(XData rgb) = 0;

	// ��Ƶ��������ʼ��
	virtual bool InitVideoCodec() = 0;

	// ��Ƶ��������ʼ��
	virtual bool InitAudioCodec() = 0;

	// ��Ƶ����
	virtual XData EncodeVideo(XData yuv) = 0;

	// ��ȡ������������
	virtual AVCodecContext *GetCodecContext(void) = 0;

	//��Ƶ�ز��������ĳ�ʼ�� 
	virtual bool InitResample() = 0;

	// ��Ƶ�ز��� (����ֵ�������������)
	virtual XData Resample(XData frame) = 0;

	// ��Ƶ���� (����ֵ�������������)
	virtual XData EncodeAudio(XData frame) = 0;

	virtual void Close() = 0;

	virtual ~XMediaEncode();

	AVCodecContext *ac = 0;  // ��Ƶ������������
	AVCodecContext *vc = 0;

protected:
	XMediaEncode();
};

