#include "XAudioRecord.h"
#include <QAudioInput>
#include <QMutex>
#include <iostream>
#include <list>

using namespace std;

class CXAudioRecord :public XAudioRecord
{
public:
	bool isExit = false;
	QMutex mutex;
	list <XData> datas;
	int maxList = 100;

	void run()
	{
		cout << "audio record thread" << endl;
		// 一次读取一帧音频的字节数
		int readSize = nbSamples * channels * sampleByte;


		while (!isExit)
		{
			char *buf = new char[readSize];
			//读取已录制的音频
			if (input->bytesReady() < readSize)
			{
				QThread::msleep(1);
				continue;
			}

			int size = 0;
			while (size != readSize)
			{
				int len = io->read(buf + size, readSize - size);
				if (len < 0)
				{
					break;
				}
				size += len;
			}

			if (size != readSize)
			{
				delete buf;
				continue;
			}

			//已经读取一帧
			XData d;
			d.data = buf;
			d.size = readSize;

			mutex.lock();
			if (datas.size() > maxList)
			{
				datas.front().Drop();
				datas.pop_front();
			}

			datas.push_back(d);
			mutex.unlock();
		}

		cout << "退出 audio record thread" << endl;
	}

	XData Pop()
	{
		mutex.lock();
		if (datas.empty())
		{
			mutex.unlock();
			return XData();
		}
		XData d = datas.front();
		datas.pop_front();

		mutex.unlock();
		return d;
	}

	// start record
	virtual bool Init()
	{
		Stop();

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

	    input = new QAudioInput(fmt);

		//开始录制音频（内部开启线程，放到缓冲区中）
	    io = input->start(); 

		if (!io)
		{
			return false;
		}

		QThread::start();
		isExit = false;

		return true;
	}

	//stop record
	virtual void Stop() 
	{
		isExit = true;
		wait();  //等待当前线程退出

		if (input) {
			input->stop();
		}

		if (io)
		{
			io->close();
		}

		input = NULL;
		io = NULL;
	}

	QAudioInput *input = NULL;
	QIODevice *io = NULL;
};

XAudioRecord::XAudioRecord()
{
}


XAudioRecord::~XAudioRecord()
{
}

XAudioRecord *XAudioRecord::Get(XAUDIOTYPE type, unsigned char index)
{
	static CXAudioRecord record[256];
	return &record[index];
}