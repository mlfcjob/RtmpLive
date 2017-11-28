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

	long long beginTime = GetCurTime();

	XMediaEncode *xe = XMediaEncode::Get(0);

	// 0 打开摄像机
	XVideoCapture *xv = XVideoCapture::Get();
	if (!xv->Init(0))
	{
		cout<<"open camera failed!"<<endl;
		getchar();
		return -1;
	}
	cout << "打开摄像机成功" << endl;
	
	xv->Clear();
	xv->Start();

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

	ar->Clear();
	ar->Start();

	/// 1-1 视频编码类
	xe->inWidth = xv->width;
	xe->inHeight = xv->height;
	xe->outWidth = xv->width;
	xe->outHeight = xv->height;
	if (!xe->InitScale())
	{
		getchar();
		return -1;
	}

	cout << "初始化视频像素转换上下文成功" << endl;

	// 2 音频重采样上下文初始化

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

	// 3-1 初始化视频编码器
	if (!xe->InitVideoCodec())
	{
		getchar();
		return -1;
	}

	cout << "初始化视频编码器成功" << endl;

	XRtmp *xr = XRtmp::Get(0);

	// 4 输出封装器和音频流配置
	if (!xr->Init(outUrl))
	{
		getchar();
		return -1;
	}

	// a 添加视频流
	int vindex = 0;
	vindex = xr->AddStream(xe->vc);
		
	if (vindex < 0)
	{
		getchar();
		return -1;
	}

	//// b 添加音频流
	////从编码器复制参数
	int aindex = 0;
	aindex = xr->AddStream(xe->ac);
	if (aindex < 0)
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
		// 一次读取一帧音频
		XData ad = ar->Pop();
		XData vd = xv->Pop();

		if (vd.size <= 0 && ad.size <= 0)
		{
			QThread::msleep(1);
			continue;
		}

		//处理音频
		if (ad.size > 0)
		{
			ad.pts = ad.pts - beginTime;
			//已经读一帧源数据
			//重采样源数据
			XData pcm = xe->Resample(ad);
			ad.Drop();

			//编码
			XData pkt = xe->EncodeAudio(pcm);
			if (pkt.size > 0)
			{
				// 推流
				if (xr->SendFrame(pkt, aindex))
				{
					cout << "Audio #." << flush;
				}
			}
		}

		// 处理视频
		if (vd.size > 0)
		{
			vd.pts = vd.pts - beginTime;
			XData yuv = xe->RGBToYUV(vd);
			vd.Drop();

			XData pkt = xe->EncodeVideo(yuv);
			if (pkt.size > 0)
			{
				if (xr->SendFrame(pkt, vindex))
				{
					cout << "Video @" << endl;
				}
			}
		}		
	}

	getchar();
	return a.exec();
}
