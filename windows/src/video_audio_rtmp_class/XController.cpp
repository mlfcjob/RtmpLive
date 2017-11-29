#include "XController.h"
#include "XVideoCapture.h"
#include "XAudioRecord.h"
#include "XFilter.h"
#include "XMediaEncode.h"
#include "XRtmp.h"

#include <iostream>

using namespace std;


XController::XController()
{
}


XController::~XController()
{
}

void XController::run()
{
	long long beginTime = GetCurTime();
	while (!isExit)
	{
			// һ�ζ�ȡһ֡��Ƶ
			XData ad = XAudioRecord::Get()->Pop();
			XData vd = XVideoCapture::Get()->Pop();

			if (vd.size <= 0 && ad.size <= 0)
			{
				QThread::msleep(1);
				continue;
			}

			//������Ƶ
			if (ad.size > 0)
			{
				ad.pts = ad.pts - beginTime;
				//�Ѿ���һ֡Դ����
				//�ز���Դ����
				XData pcm = XMediaEncode::Get()->Resample(ad);
				ad.Drop();

				//����
				XData pkt = XMediaEncode::Get()->EncodeAudio(pcm);
				if (pkt.size > 0)
				{
					// ����
					if (XRtmp::Get()->SendFrame(pkt, aindex))
					{
						cout << "Audio #." << endl;
					}
				}
			}

			// ������Ƶ
			if (vd.size > 0)
			{
				vd.pts = vd.pts - beginTime;
				XData yuv = XMediaEncode::Get()->RGBToYUV(vd);
				vd.Drop();

				XData pkt = XMediaEncode::Get()->EncodeVideo(yuv);
				if (pkt.size > 0)
				{
					if (XRtmp::Get()->SendFrame(pkt, vindex))
					{
						cout << "Video @" << endl;
					}
				}
			}		
	}
}

bool XController::Set(std::string key, double val)
{
	XFilter::Get()->Set(key, val);
	return true;
}

bool XController::Start()
{

	//1 ����ĥƤ������
	XVideoCapture::Get()->AddFilter(XFilter::Get());

	// 2 �����
	if (camIndex >= 0)
	{
		if (!XVideoCapture::Get()->Init(camIndex))
		{
			err = "2 �����ʧ��";
			cout << err << endl;
			return false;
		}
	}
	else if (!inUrl.empty())
	{
		if (!XVideoCapture::Get()->Init(inUrl.c_str()))
		{
			err = "��";
			err += inUrl;
			err += "���ʧ��";
			cout << err << endl;
			return false;
		}
	}
	else
	{
		err = "�������������";
		cout << "�������������" << endl;
	}

	cout << "����򿪳ɹ�"<< endl;

	// 3 QT��Ƶ��ʼ¼��
	if (!XAudioRecord::Get()->Init())
	{
		err = " 3 ¼���豸��ʧ��";
		cout << err << endl;
		return false;
	}

	cout << "¼���豸�򿪳ɹ�" << endl;

	XAudioRecord::Get()->Start();
	XVideoCapture::Get()->Start();

	// ����Ƶ������

	// 4 ��ʼ����ʽת�������� ��ʼ����������ݽṹ
	XMediaEncode::Get()->inWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->inHeight = XVideoCapture::Get()->height;
	XMediaEncode::Get()->outWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->outHeight = XVideoCapture::Get()->height;
	if (!XMediaEncode::Get()->InitScale())
	{
		err = " 4 ��Ƶ���ظ�ʽת�������Ĵ�ʧ��";
		cout << err << endl;
		return false;
	}

	cout << "��Ƶ���ظ�ʽת�������Ĵ򿪳ɹ�"  << endl;
	
	// 5 ��Ƶ�ز��������ĳ�ʼ��
	XMediaEncode::Get()->channels = XAudioRecord::Get()->channels;
	XMediaEncode::Get()->sampleRate = XAudioRecord::Get()->sampleRate;
	XMediaEncode::Get()->nbSample = XAudioRecord::Get()->nbSample;
	if (!XMediaEncode::Get()->InitResample())
	{
		err = " 5 ��Ƶ�ز��������Ĵ�ʧ��";
		cout << err << endl;
		return false;
	}

	cout << "��Ƶ�ز��������Ĵ򿪳ɹ�" << endl;


	// 6 ��ʼ����Ƶ������
	if (!XMediaEncode::Get()->InitAudioCodec())
	{
		err = " 6 ��ʼ����Ƶ������ʧ��";
		cout << err << endl;
		return false;
	}

	cout << "��ʼ����Ƶ�������ɹ�" << endl;

	// 7 ��ʼ����Ƶ������
	if (!XMediaEncode::Get()->InitVideoCodec())
	{
		err = " 7 ��ʼ����Ƶ������ʧ��";
		cout << err << endl;
		return false;
	}

	cout << "��ʼ����Ƶ�������ɹ�" << endl;

	// 8 ���������װ��������
	if (!XRtmp::Get()->Init(outUrl.c_str()))
	{
		err = " 8 �����װ�������ĳ�ʼ��ʧ��";
		cout << err << endl;
		return false;
	}

	cout << "���������װ�������ĳɹ�" << endl;

	// 9 �������Ƶ��
	vindex =  XRtmp::Get()->AddStream(XMediaEncode::Get()->vc);
	aindex = XRtmp::Get()->AddStream(XMediaEncode::Get()->ac);

	if (vindex < 0 || aindex < 0)
	{
		err = "9 �������Ƶ��ʧ��";
		cout << err << endl;
		return false;
	}

	cout << "�������Ƶ���ɹ�" << endl;

	// 10 д��rtmp��װͷ
	if (!XRtmp::Get()->SendHead())
	{
		err  = "���ͷ�װͷʧ��";
		cout << err << endl;
		return false;
	}

	// 11 ��������Ƶ¼���߳�

	XAudioRecord::Get()->Clear();
	XVideoCapture::Get()->Clear();

	XDataThread::Start();

	return true;
}


void XController::Stop()
{
	XDataThread::Stop();

	XAudioRecord::Get()->Stop();
	XVideoCapture::Get()->Stop();
	XMediaEncode::Get()->Close();
	XRtmp::Get()->Close();

	camIndex = -1;
	inUrl = "";
}