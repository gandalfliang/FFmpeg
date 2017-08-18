#include "demoplayer.h"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio  

//Buffer:  
//|-----------|-------------|  
//chunk-------pos---len-----|  
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

void  fill_audio(void *udata, Uint8 *stream, int len) {
	//SDL 2.0  
	SDL_memset(stream, 0, len);
	if (audio_len == 0)
		return;

	len = (len > audio_len ? audio_len : len);   /*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

int main(int argc, char *argv[])
{
	if (argc <= 1)
		return 1;
	const char *filePath = "F:\\电影\\后会无期.HD1024高清国语中字.mp4";

	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pVCodec;
	AVPacket *pPacket;

	//ffmpeg apis
	av_register_all();
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx, filePath, NULL, NULL) != 0)
	{
		printf("cannot open input");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("cannot find stream info");
		return -1;
	}

	int audioIndex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioIndex = i;
			break;
		}
	}
	if (audioIndex == -1)
	{
		printf("cannot find video stream");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[audioIndex]->codec;
	pVCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pVCodec == NULL)
	{
		printf("cannot find video decoder");
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pVCodec, NULL) != 0)
	{
		printf("cannot open decoder");
		return -1;
	}

	pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
	av_init_packet(pPacket);

	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	int out_nb_samples = pCodecCtx->frame_size;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate = pCodecCtx->sample_rate;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);
	uint8_t* out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	AVFrame *pFrame = av_frame_alloc();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("cannot init sdl");
		return -1;
	}

	SDL_AudioSpec sdlAudioSpc;
	sdlAudioSpc.freq = out_sample_rate;
	sdlAudioSpc.format = AUDIO_S16SYS;
	sdlAudioSpc.channels = out_channels;
	sdlAudioSpc.silence = 0;
	sdlAudioSpc.callback = fill_audio;
	sdlAudioSpc.samples = out_nb_samples;
	sdlAudioSpc.userdata = pCodecCtx;

	if (SDL_OpenAudio(&sdlAudioSpc, NULL) < 0)
	{
		printf("cannot open audio,%s\n", SDL_GetError());
		return -1;
	}

	int64_t in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);

	SwrContext* au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate, in_channel_layout, pCodecCtx->sample_fmt,
		pCodecCtx->sample_rate, 0, NULL);
	swr_init(au_convert_ctx);

	SDL_PauseAudio(0);

	int got_frame;
	while (av_read_frame(pFormatCtx, pPacket) >= 0)
	{
		if (pPacket->stream_index == audioIndex)
		{
			int ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_frame, pPacket);
			if (ret < 0)
			{
				printf("cannot decode audio");
				return -1;
			}

			if (got_frame > 0)
			{
				swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t**)pFrame->data, pFrame->nb_samples);

			}

			while (audio_len > 0)//Wait until finish  
				SDL_Delay(1);

			//Set audio buffer (PCM data)  
			audio_chunk = (Uint8 *)out_buffer;
			//Audio buffer length  
			audio_len = out_buffer_size;
			audio_pos = audio_chunk;
		}
		av_free_packet(pPacket);
	}

	swr_free(&au_convert_ctx);
	SDL_CloseAudio();//Close SDL  
	SDL_Quit();

	av_free(out_buffer);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}