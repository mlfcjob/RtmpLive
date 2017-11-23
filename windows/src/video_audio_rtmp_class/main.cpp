#include <QtCore/QCoreApplication>
#include <iostream>
#include <QThread>
#include "XMediaEncode.h"
#include "XRtmp.h"
#include "XAudioRecord.h"

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


	// 2 ��Ƶ�ز��������ĳ�ʼ��
	XMediaEncode *xe = XMediaEncode::Get(0);
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

	XRtmp *xr = XRtmp::Get(0);

	// 4 �����װ������Ƶ������
	if (!xr->Init(outUrl))
	{
		getchar();
		return -1;
	}


	//// b �����Ƶ��
	////�ӱ��������Ʋ���
	if (!xr->AddStream(xe->ac))
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
		XData d = ar->Pop();
		if (d.size <= 0)
		{
			QThread::msleep(1);
			continue;
		}

		//�Ѿ���һ֡Դ����
		//�ز���Դ����
		AVFrame *pcm = xe->Resample(d.data);
		d.Drop();

		//����
		AVPacket *pkt = xe->EncodeAudio(pcm);
		if (NULL == pkt)
		{
			continue;
		}

		// ����
		if (!xr->SendFrame(pkt))
		{
			continue;
		}
	}

	getchar();
	return a.exec();
}
