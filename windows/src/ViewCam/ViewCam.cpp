
#include <opencv2/highgui.hpp>
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

#pragma comment(lib, "opencv_world330d.lib")

int main(int argc, char *argv[])
{
	VideoCapture cam;

	//char *inUrl = "rtsp:://test:test123456@192.168.1.64"; //hikon
	//char *inUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov"; //big bunny
	// char *inUrl = rtsp://192.168.52.6:9020/h264.sd
	string url = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	namedWindow("video");

	//if (cam.open(url))
	if(cam.open(0))
	{
		cout << "open cam succes" << endl;
	}
	else {
		cout << "open cam faild" << endl;
		waitKey(0);
		return -1;
	}

	Mat frame;
	for (;;) {
		cam.read(frame);
		imshow("video", frame);
		waitKey(1);
	}
	getchar();
	return 0;
}