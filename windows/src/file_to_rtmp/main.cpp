#include <iostream>

using namespace std;

extern "C" {
#include "libavformat/avformat.h"
}

#pragma comment(lib, "avformat.lib")

int main(int argc, char *argv[])
{
	av_register_all();

	cout << "file to rtmp test" << endl;
	getchar();

	return 0;
}
