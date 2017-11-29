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
			// 一次读取一帧音频
			XData ad = XAudioRecord::Get()->Pop();
			XData vd = XVideoCapture::Get()->Pop();

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
				XData pcm = XMediaEncode::Get()->Resample(ad);
				ad.Drop();

				//编码
				XData pkt = XMediaEncode::Get()->EncodeAudio(pcm);
				if (pkt.size > 0)
				{
					// 推流
					if (XRtmp::Get()->SendFrame(pkt, aindex))
					{
						cout << "Audio #." << endl;
					}
				}
			}

			// 处理视频
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

	//1 设置磨皮过滤器
	XVideoCapture::Get()->AddFilter(XFilter::Get());

	// 2 打开相机
	if (camIndex >= 0)
	{
		if (!XVideoCapture::Get()->Init(camIndex))
		{
			err = "2 打开相机失败";
			cout << err << endl;
			return false;
		}
	}
	else if (!inUrl.empty())
	{
		if (!XVideoCapture::Get()->Init(inUrl.c_str()))
		{
			err = "打开";
			err += inUrl;
			err += "相机失败";
			cout << err << endl;
			return false;
		}
	}
	else
	{
		err = "请设置相机参数";
		cout << "请设置相机参数" << endl;
	}

	cout << "相机打开成功"<< endl;

	// 3 QT音频开始录制
	if (!XAudioRecord::Get()->Init())
	{
		err = " 3 录音设备打开失败";
		cout << err << endl;
		return false;
	}

	cout << "录音设备打开成功" << endl;

	XAudioRecord::Get()->Start();
	XVideoCapture::Get()->Start();

	// 音视频编码类

	// 4 初始化格式转换上下文 初始化输出的数据结构
	XMediaEncode::Get()->inWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->inHeight = XVideoCapture::Get()->height;
	XMediaEncode::Get()->outWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->outHeight = XVideoCapture::Get()->height;
	if (!XMediaEncode::Get()->InitScale())
	{
		err = " 4 视频像素格式转换上下文打开失败";
		cout << err << endl;
		return false;
	}

	cout << "视频像素格式转换上下文打开成功"  << endl;
	
	// 5 音频重采样上下文初始化
	XMediaEncode::Get()->channels = XAudioRecord::Get()->channels;
	XMediaEncode::Get()->sampleRate = XAudioRecord::Get()->sampleRate;
	XMediaEncode::Get()->nbSample = XAudioRecord::Get()->nbSample;
	if (!XMediaEncode::Get()->InitResample())
	{
		err = " 5 音频重采样上下文打开失败";
		cout << err << endl;
		return false;
	}

	cout << "音频重采样上下文打开成功" << endl;


	// 6 初始化音频编码器
	if (!XMediaEncode::Get()->InitAudioCodec())
	{
		err = " 6 初始化音频编码器失败";
		cout << err << endl;
		return false;
	}

	cout << "初始化音频编码器成功" << endl;

	// 7 初始化视频编码器
	if (!XMediaEncode::Get()->InitVideoCodec())
	{
		err = " 7 初始化视频编码器失败";
		cout << err << endl;
		return false;
	}

	cout << "初始化视频编码器成功" << endl;

	// 8 创建输出封装器上下文
	if (!XRtmp::Get()->Init(outUrl.c_str()))
	{
		err = " 8 输出封装器上下文初始化失败";
		cout << err << endl;
		return false;
	}

	cout << "创建输出封装器上下文成功" << endl;

	// 9 添加音视频流
	vindex =  XRtmp::Get()->AddStream(XMediaEncode::Get()->vc);
	aindex = XRtmp::Get()->AddStream(XMediaEncode::Get()->ac);

	if (vindex < 0 || aindex < 0)
	{
		err = "9 添加音视频流失败";
		cout << err << endl;
		return false;
	}

	cout << "添加音视频流成功" << endl;

	// 10 写入rtmp封装头
	if (!XRtmp::Get()->SendHead())
	{
		err  = "发送封装头失败";
		cout << err << endl;
		return false;
	}

	// 11 启动音视频录制线程

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