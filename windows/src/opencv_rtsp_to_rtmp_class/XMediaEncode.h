#pragma once

struct AVFrame; //ģ��Qt������
struct AVPacket;

//����Ƶ����ӿ��� 
class XMediaEncode
{
public:
	//�������
	int  inWidth = 1280;
	int  inHeight = 720;
	int  inPixSize = 3;

	//�������
	int  outWidth = 1280;
	int  outHeight = 720;
	int  bitrate = 400000;  // bit
	int  fps = 25;

	//������������
	static XMediaEncode * Get(unsigned char index = 0);

	//��ʼ�����ظ�ʽת����������
	virtual bool InitScale() = 0;

	virtual AVFrame* RGBToYUV(char *rgb) = 0;

	// ��Ƶ��������ʼ��
	virtual bool InitVideoCodec() = 0;

	// ��Ƶ����
	virtual AVPacket *EncodeVideo(AVFrame *yuv) = 0;

	virtual bool InitOutputContext(char *outUrl) = 0;

	virtual bool InitRtmp(char *outUrl) = 0;

	virtual bool PushMedia(AVPacket *pack)  = 0;

	virtual ~XMediaEncode();

protected:
	XMediaEncode();
};

