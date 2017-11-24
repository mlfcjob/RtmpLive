#pragma once

#include "XDataThread.h"

enum XAUDIOTYPE
{
	X_AUDIO_QT,
};

class XAudioRecord :public XDataThread
{
public:
	int channels = 2;
	int sampleRate = 44100;
	int sampleByte = 2; 
	int nbSamples = 1024;  // 一帧音频每个通道的样本数量

	static XAudioRecord *Get(XAUDIOTYPE type = X_AUDIO_QT, unsigned char index = 0);

	// start record
	virtual bool Init() = 0;

	//stop record
	virtual void Stop() = 0;

	virtual ~XAudioRecord();
protected:
	XAudioRecord();
};

