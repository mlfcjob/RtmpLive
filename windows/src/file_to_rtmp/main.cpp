#include <iostream>

using namespace std;

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

int XError(int errNum)
{
	char buf[1024] = { 0 };
	av_strerror(errNum, buf, sizeof(buf));
	cout << buf << endl;
	getchar();
	return -1;
}

static double r2d(AVRational r)
{
	return (r.num == 0 || r.den == 0) ? 0.: (double)r.num / (double)r.den;
}

int main(int argc, char *argv[])
{
	char *inUrl = "swxf.flv";
	//char *inUrl = "parent.flv";
	char *outUrl = "rtmp://192.168.103.139/live";

	//��ʼ�����еķ�װ�ͽ��װ flv mp4 mov mp3
	av_register_all();

	//��ʼ�������
	avformat_network_init();

	/////////////////////////////////////////////////////////////////////////////////////////
	//// ������
	//1 ���ļ������װ
	// �����װ������
	AVFormatContext *ictx = NULL;

	//���ļ������Э��ͷ
	int re = avformat_open_input(&ictx, inUrl, 0, 0);
	if (re != 0) {
		return XError(re);
	}

	cout << "open file " << inUrl << "success" << endl;

	// ��ȡ��Ƶ��Ƶ����Ϣ
	re = avformat_find_stream_info(ictx, NULL);
	if (re != 0) {
		return XError(re);
	}

	av_dump_format(ictx, 0, inUrl, 0);
	//////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////////////////////
	//// ���������������
	AVFormatContext *octx = NULL;
	re = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
	if (!octx) {
		return XError(re);
	}

	cout << "octx create success" << endl;
	
	//���������
	//���������AVStream
	for (int i = 0; i < ictx->nb_streams; i++) {
		// ���������
		AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
		if (!out) {
			return XError(0);
		}

		//����������Ϣ,����mp4
		//re = avcodec_copy_context(out->codec, ictx->streams[i]->codec);
		re = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
		out->codec->codec_tag = 0;
	}

	av_dump_format(octx, 0, outUrl, 1);
	///////////////////////////////////////////////////////////////////////////////////////////

	// rtmp����
	// ��io(����Э������ͨ������)
	re = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
	if (!octx->pb) {
		return XError(re);
	}

	// д��ͷ��Ϣ
	re = avformat_write_header(octx, 0);
	if (re < 0) {
		return XError(re);
	}
	cout << "avfomat_write_header " << re << endl;
 
	//����ÿһ֡����
	AVPacket pkt;
	long long startTime = av_gettime();  //us

	for (;;) {		
		re = av_read_frame(ictx, &pkt);

		if (re != 0) {
			break;
		}
		cout <<  pkt.pts << " " << flush;
		// ����ת��dts pts
		AVRational itime =  ictx->streams[pkt.stream_index]->time_base;
		AVRational otime = octx->streams[pkt.stream_index]->time_base;
		pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.pos = -1;

		
		re = av_interleaved_write_frame(octx, &pkt);
		
		//��Ƶ֡
		if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
			AVRational tb = ictx->streams[pkt.stream_index]->time_base;
			//�Ѿ���ȥ��ʱ��
			long long now = av_gettime() - startTime;
			long long dts = 0;

			dts = pkt.dts * (r2d(tb) * 1000 * 1000);
			if (dts > now) {
				av_usleep(dts - now);
			}
		}
	
		if (re < 0) {
			return XError(re);
		}

		//av_packet_unref(&pkt);  //�ͷ�pkt->data�ռ�
	}
	cout << "file to rtmp test" << endl;
	getchar();

	return 0;
}
