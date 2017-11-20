#include <QtCore/QCoreApplication>
#include <qaudioinput.h>
#include <iostream>

extern "C"
{
#include <libswresample/swresample.h>
}

#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "avutil.lib")


using namespace std;

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	
	int sampleRate = 44100;
	int channels = 2;
	int sampleSize = 2;


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
	SwrContext *asc = NULL;
	asc = swr_alloc_set_opts(asc, 
		              av_get_default_channel_layout(channels), AV_SAMPLE_FMT_FLTP/*for aac*/, sampleRate,  //输出格式
		              av_get_default_channel_layout(channels), AV_SAMPLE_FMT_S16, sampleRate, //输入格式
		              0, 0);

	if (!asc)
	{
		cout << "swr_alloc_set_opts failed. " << endl;
		getchar();
		return -1;
	}

	int ret = swr_init(asc);
	if (ret != 0)
	{
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err));
		getchar();
		return -1;
	}

	cout << "swr init success." << endl;

	char buf[4096] = { 0 };
	for (;;)
	{

		if (input->bytesReady() > 4096)
		{
			cout << "read len: "<< io->read(buf, sizeof(buf)) << endl;
		}
	}
	return a.exec();
}
