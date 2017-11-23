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


	// 1 qt音频开始录制
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

	// 3 初始化音频编码器
	if (!xe->InitAudioCodec())
	{
		getchar();
		return -1;
	}

	XRtmp *xr = XRtmp::Get(0);

	// 4 输出封装器和音频流配置
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
	

	//5 打开rtmp 的网络输出IO
	//// 写入封装头
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

		//已经读一帧源数据
		//重采样源数据
		AVFrame *pcm = xe->Resample(d.data);
		d.Drop();

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

	getchar();
	return a.exec();
}
