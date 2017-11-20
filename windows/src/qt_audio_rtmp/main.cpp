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
	SwrContext *asc = NULL;
	asc = swr_alloc_set_opts(asc, 
		              av_get_default_channel_layout(channels), AV_SAMPLE_FMT_FLTP/*for aac*/, sampleRate,  //�����ʽ
		              av_get_default_channel_layout(channels), AV_SAMPLE_FMT_S16, sampleRate, //�����ʽ
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
