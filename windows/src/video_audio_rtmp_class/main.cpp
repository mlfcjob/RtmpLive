#include <QtCore/QCoreApplication>
#include <iostream>
#include "XController.h"

using namespace std;

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

    XController::Get()->Stop();
    XController::Get()->camIndex = 0;
	XController::Get()->outUrl = "rtmp://192.168.103.139/live";
    XController::Get()->Start();

	XController::Get()->wait();
	return a.exec();
}
