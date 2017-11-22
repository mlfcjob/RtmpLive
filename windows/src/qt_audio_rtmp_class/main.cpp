#include <QtCore/QCoreApplication>
#include <qaudioinput.h>
#include <iostream>
#include <QThread>
#include "XMediaEncode.h"
#include "XRtmp.h"

using namespace std;

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	
	const char *outUrl = "rtmp://192.168.103.139/live";
	//const char *outUrl = "r.mp3";

	int sampleRate = 44100;
	int channels = 2;
	int sampleByte = 2;


	// 1 qt��Ƶ��ʼ¼��
	QAudioFormat fmt;
	fmt.setSampleRate(44100);
	fmt.setSampleSize(16);
	fmt.setChannelCount(2);
	fmt.setCodec("audio/pcm");
	fmt.setByteOrder(QAudioFormat::LittleEndian);
	fmt.setSampleType(QAudioFormat::UnSignedInt);

	QAudioDeviceInfo  info = QAudioDeviceInfo::defaultInputDevice();
	if (!info.isFormatSupported(fmt))
	{
		cout << "AudioFormat not supported" << endl;
		fmt = info.nearestFormat(fmt);
	}
	cout << "success" << endl;

	QAudioInput *input = new QAudioInput(fmt);

	//��ʼ¼����Ƶ���ڲ������̣߳��ŵ��������У�
	QIODevice *io = input->start();


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

	// 4 ��ʼ����Ƶ������
	if (!xe->InitAudioCodec())
	{
		getchar();
		return -1;
	}

	
	XRtmp *xr = XRtmp::Get(0);

	// 5 �����װ������Ƶ������
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
	

	//6 ��rtmp ���������IO
	//// д���װͷ
	if (!xr->SendHead())
	{
		getchar();
		return -1;
	}

	// һ�ζ�ȡһ֡��Ƶ���ֽ���
	int readSize = xe->nbSample * xe->channels * sampleByte;
	char *buf = new char[readSize];

	int apts = 0;

	for (;;)
	{
		if (input->bytesReady() < readSize)
		{
			QThread::msleep(1);
			continue;
		}

		int size = 0;
		while (size != readSize)
		{
			int len =  io->read(buf + size, readSize - size);
			if (len < 0)
			{
				break;
			}
			size += len;
		}

		if (size != readSize)
		{
			continue;
		}

		//�Ѿ���һ֡Դ����
		//�ز���Դ����
		AVFrame *pcm = xe->Resample(buf);

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

	delete buf;
	getchar();
	return a.exec();
}
