#include "xrtmpstreamer.h"
#include <iostream>
#include "XController.h"

using namespace std;

static bool isStream = false;

XRtmpStreamer::XRtmpStreamer(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}


void XRtmpStreamer::Stream()
{
	if (isStream)
	{
		isStream = false;
		ui.startButton->setText(QString::fromLocal8Bit("¿ªÊ¼"));
		XController::Get()->Stop();
	}
	else
	{
		isStream = true;
		ui.startButton->setText(QString::fromLocal8Bit("Í£Ö¹"));
		
		if (ui.outUrl->text() == "")
		{
			ui.outUrl->setText("rtmp://192.168.103.139/live");
		}

		QString url = ui.inUrl->text();
		if (ui.inUrl->text() == "")
		{
			ui.inUrl->setText("0");
		}

		bool ok = false;
		int camIndex = url.toInt(&ok);
		if (!ok)
		{
			XController::Get()->inUrl = url.toStdString();
		}
		else {
			XController::Get()->camIndex = camIndex;
		}

		XController::Get()->outUrl = ui.outUrl->text().toStdString();
		XController::Get()->Set("d", (ui.face->currentIndex() + 1) * 3);
		XController::Get()->Start();
	}
}