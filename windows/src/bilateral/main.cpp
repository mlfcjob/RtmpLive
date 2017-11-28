#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;

#pragma comment(lib, "opencv_world330d.lib")

int main(int argc, char *argv[])
{
	Mat src = imread("001.jpg");
	Mat image;

	if (!src.data)
	{
		cout << "open file failed " << endl;
		getchar();
		return -1;
	}

	namedWindow("src");
	namedWindow("image");
	moveWindow("src", 100, 100);
	imshow("src", src);

	int d = 3;

	for (;;)
	{
		long long  b = getTickCount();

		bilateralFilter(src, image, d, d * 2, d / 2);

		double sec = (double)(getTickCount() - b) / (double)getTickFrequency();
		cout << "d is " << d <<", sec is " << sec << "." <<endl;


		imshow("image", image);
		moveWindow("image", 600, 100);

		int k = waitKey(0);
		if (k == 'd')
		{
			d += 2;
		}
		else if (k == 'f')
		{
			d -= 2;
		}
		else
		{
			break;
		}
	}

	waitKey(0);

	return 0;
}