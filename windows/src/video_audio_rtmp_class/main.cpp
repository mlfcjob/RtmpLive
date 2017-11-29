#include <QtCore/QCoreApplication>
#include "XController.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	const char *outUrl = "rtmp://192.168.103.139/live";

    XController::Get()->Stop();
    XController::Get()->camIndex = 0;
	XController::Get()->outUrl = outUrl;
    XController::Get()->Start();

	XController::Get()->wait();
	return a.exec();
}
