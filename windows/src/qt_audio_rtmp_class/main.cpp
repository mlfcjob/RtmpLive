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


	// 1 qt音频开始录制
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

	//开始录制音频（内部开启线程，放到缓冲区中）
	QIODevice *io = input->start();


	// 2 音频重采样上下文初始化

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

	// 4 初始化音频编码器
	if (!xe->InitAudioCodec())
	{
		getchar();
		return -1;
	}

	
	XRtmp *xr = XRtmp::Get(0);

	// 5 输出封装器和音频流配置
	if (!xr->Init(outUrl))
	{
		getchar();
		return -1;
	}


	//// b 添加音频流
	////从编码器复制参数
	if (!xr->AddStream(xe->ac))
	{
		getchar();
		return -1;
	}
	

	//6 打开rtmp 的网络输出IO
	//// 写入封装头
	if (!xr->SendHead())
	{
		getchar();
		return -1;
	}

	// 一次读取一帧音频的字节数
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

		//已经读一帧源数据
		//重采样源数据
		AVFrame *pcm = xe->Resample(buf);

		//编码
		AVPacket *pkt = xe->EncodeAudio(pcm);
		if (NULL == pkt)
		{
			continue;
		}

		// 推流
		if (!xr->SendFrame(pkt))
		{
			continue;
		}
	}

	delete buf;
	getchar();
	return a.exec();
}
