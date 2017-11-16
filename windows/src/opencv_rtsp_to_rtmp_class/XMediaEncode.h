#pragma once

struct AVFrame; //ģ��Qt������

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

	//������������
	static XMediaEncode * Get(unsigned char index = 0);

	//��ʼ�����ظ�ʽת����������
	virtual bool InitScale() = 0;

	virtual AVFrame* RGBToYUV(char *rgb) = 0;

	virtual ~XMediaEncode();

protected:
	XMediaEncode();
};

