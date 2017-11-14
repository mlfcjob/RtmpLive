
#include <opencv2/highgui.hpp>
#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using namespace std;
using namespace cv;

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "opencv_world330d.lib")

int main(int argc, char *argv[])
{

	//char *inUrl = "rtsp:://test:test123456@192.168.1.64"; //hikon
	//char *inUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov"; //big bunny
	char *inUrl = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	//char *inUrl = "F:\\FFOutput\\2504.mkv";
	//nginx-rtmp 直播服务器
	char *outUrl = "rtmp://192.168.1.104/live";

	//注册所有编解码器信息
	avcodec_register_all();

	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	// pixel convert context
	SwsContext *vsc = NULL;
	AVFrame *yuv = NULL;

	//编码器上下文
	AVCodecContext   *vc = NULL;


	try {
		/// 1. 打开像机(opencv)
		cam.open(inUrl);

		if (!cam.isOpened()) {
			throw exception("cam open failed");
		}
		cout << inUrl << "cam open sucess" << endl;

		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
		int fps = cam.get(CAP_PROP_FPS);


		/// 2. init convert context
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, // src width, height
			AV_PIX_FMT_BGR24,
			inWidth, inHeight,// dst width, height, dst format
			AV_PIX_FMT_YUV420P,
			SWS_BICUBIC, // 尺寸变化使用算法
			0,0,0);

		if (!vsc)
		{
			throw exception("sws_getCacheContext faied");
		}

		/// 3 rgb to yuv
		// 输出的数据结构
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;

		// 分配yuv包含的数据空间
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf));
			throw exception("buf");
		}

		/// 4 初始化编码器上下文
		// a 找到编码器
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			throw exception("can not find h264 encoder!");
		}

		// b 创建编码器上下文
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			throw exception("avcodec_alloc_context3 failed!");
		}

		// c 配置编码器参数
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;  //全局参数
		vc->codec_id = codec->id;
		vc->thread_count = 8;

		// only for video
		vc->bit_rate = 50 * 1024 * 8;      // bits size per sec after compressed : 50kB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = { 1, fps };  // pts * 1 / fps = second 
		vc->framerate = { fps, 1 };

		// 画面组的大小，多少帧一个关键帧，
		//设得越大，压缩率越高，同时画质也会更容易损失，性能消耗也会更大
		vc->gop_size = 5;
		vc->max_b_frames = 0; // B 帧设为0， 这种情况下pts和dts就会一致
		vc->pix_fmt = AV_PIX_FMT_YUV420P;

		//d 打开编码器上下文
		ret = avcodec_open2(vc, NULL, NULL);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		cout << "avcodec Open 2 encoder success." << endl;

		AVPacket pack;
		memset(&pack, 0, sizeof(pack));

		int vpts = 0;

		for (;;)
		{
			/// read rtsp frame ---> decode
			if (!cam.grab())
			{
				continue;
			}

			/// yuv to rgb
			if (!cam.retrieve(frame))
			{
				continue;
			}
			imshow("video", frame);
			waitKey(1);

			/// rgb to yuv
			//输入的数据结构
			uint8_t  *indata[AV_NUM_DATA_POINTERS] = { 0 };
			// interlaced:BGR BGR BGR BGR
			// planed: indata[0]: BBBB  indata[1]: GGGG indata[2]:RRRR
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//一行的数据的字节数（width）
			insize[0] = frame.cols * frame.elemSize();
			int h = sws_scale(vsc, indata, insize, 0, frame.rows, // 源数据
				yuv->data, yuv->linesize);

			if (h <= 0)
			{
				continue;
			}					

			/// h264 encode
			yuv->pts = vpts++;

			ret = avcodec_send_frame(vc, yuv);
			if (ret != 0)
			{
				continue;
			}

			ret = avcodec_receive_packet(vc, &pack);
			if (ret != 0 || pack.size > 0)
			{
				cout << "*" << pack.size << flush;
			} 
			else 
			{
				continue;
			}

			/// push media
		}
	}
	catch(exception &ex) {
		if (cam.isOpened())
		{
			cam.release();
		}		

		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = NULL;
		}

		if (vc)
		{
			avcodec_free_context(&vc);
		}
		cerr << ex.what() << endl;
	}
	
	
	getchar();
	return 0;
}