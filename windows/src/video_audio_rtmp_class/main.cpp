#include <QtCore/QCoreApplication>
#include <iostream>
#include <QThread>
#include "XMediaEncode.h"
#include "XRtmp.h"
#include "XAudioRecord.h"
#include "XVideoCapture.h"
#include "XData.h"

using namespace std;

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	
	const char *outUrl = "rtmp://192.168.103.139/live";
	//const char *outUrl = "r.mp3";

	int sampleRate = 44100;
	int channels = 2;
	int sampleByte = 2;
	int nbSample = 1024;

	XMediaEncode *xe = XMediaEncode::Get(0);

	// 0 �������
	XVideoCapture *xv = XVideoCapture::Get();
	if (!xv->Init(0))
	{
		cout<<"open camera failed!"<<endl;
		getchar();
		return -1;
	}
	cout << "��������ɹ�" << endl;
	
	xv->Start();

	// 1 qt��Ƶ��ʼ¼��
	XAudioRecord *ar = XAudioRecord::Get();
	ar->sampleRate = sampleRate;
	ar->sampleByte = sampleByte;
	ar->channels = channels;
	ar->nbSamples = nbSample;
	if (!ar->Init())
	{
		cout << "XaudioRecord init failed" << endl;
		getchar();
		return -1;
	}

	ar->Start();
	/// 1-1 ��Ƶ������
	xe->inWidth = xv->width;
	xe->inHeight = xv->height;
	xe->outWidth = xv->width;
	xe->outHeight = xv->height;
	if (!xe->InitScale())
	{
		getchar();
		return -1;
	}

	cout << "��ʼ����Ƶ����ת�������ĳɹ�" << endl;

	// 2 ��Ƶ�ز��������ĳ�ʼ��

	xe->channels = channels;
	xe->nbSample = 1024;
	xe->sampleRate = sampleRate;
	xe->inSampleFmt = XSampleFMT::X_S16;
	xe->outSampleFmt = XSampleFMT::X_FLATP;

	if (!xe->InitResample())
	{
		getchar();
		return -1;
	}

	// 3 ��ʼ����Ƶ������
	if (!xe->InitAudioCodec())
	{
		getchar();
		return -1;
	}

	// 3-1 ��ʼ����Ƶ������
	if (!xe->InitVideoCodec())
	{
		getchar();
		return -1;
	}

	cout << "��ʼ����Ƶ�������ɹ�" << endl;

	XRtmp *xr = XRtmp::Get(0);

	// 4 �����װ������Ƶ������
	if (!xr->Init(outUrl))
	{
		getchar();
		return -1;
	}

	// a �����Ƶ��
	int vindex = 0;
	vindex = xr->AddStream(xe->vc);
		
	if (vindex < 0)
	{
		getchar();
		return -1;
	}

	//// b �����Ƶ��
	////�ӱ��������Ʋ���
	int aindex = 0;
	aindex = xr->AddStream(xe->ac);
	if (aindex < 0)
	{
		getchar();
		return -1;
	}
	

	//5 ��rtmp ���������IO
	//// д���װͷ
	if (!xr->SendHead())
	{
		getchar();
		return -1;
	}

	for (;;)
	{
		// һ�ζ�ȡһ֡��Ƶ
		XData ad = ar->Pop();
		XData vd = xv->Pop();

		if (vd.size <= 0 && ad.size <= 0)
		{
			QThread::msleep(1);
			continue;
		}

		//������Ƶ
		if (ad.size > 0)
		{
			//�Ѿ���һ֡Դ����
			//�ز���Դ����
			AVFrame *pcm = xe->Resample(ad.data);
			ad.Drop();

			//����
			AVPacket *pkt = xe->EncodeAudio(pcm);
			if (pkt)
			{
				// ����
				if (xr->SendFrame(pkt, aindex))
				{
					cout << "#" << flush;
				}
			}
		}

		// ������Ƶ
		if (vd.size > 0)
		{
			AVFrame *yuv = xe->RGBToYUV(vd.data);
			//vd.Drop();

			AVPacket *pkt = xe->EncodeVideo(yuv);
			if (pkt)
			{
				if (xr->SendFrame(pkt, vindex))
				{
					cout << "@" << endl;
				}
			}
		}		
	}

	getchar();
	return a.exec();
}
