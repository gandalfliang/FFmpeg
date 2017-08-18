#include <stdio.h>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
}

#define SDL_main main

//Refresh Event  
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)  

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)  

int thread_exit = 0;
int thread_pause = 0;

int sfp_refresh_thread(void *framerate) {
	thread_exit = 0;
	thread_pause = 0;
	
	AVRational* fpsRational = (AVRational*)framerate;
	int fps = fpsRational->num / fpsRational->den;
	int delay = 1.0 / fps * 1000;
	while (!thread_exit) {
		if (!thread_pause) {
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		SDL_Delay(delay);
	}
	thread_exit = 0;
	thread_pause = 0;
	//Break  
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

int main(int argc, char **argv)
{
	if (argc <= 1)
		return 1;
	const char *filePath = "F:\\电影\\后会无期.HD1024高清国语中字.mp4";

	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pVCodec;

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

	int videoIndex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIndex = i;
			break;
		}
	}
	if (videoIndex == -1)
	{
		printf("cannot find video stream");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
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

	//sdl apis
	SDL_Window *pSDLWindow;
	SDL_Renderer *pSDLRenderer;
	SDL_Texture *pSDLTexture;
	SDL_Rect renderRect;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("cannot init sdl");
		return -1;
	}

	pSDLWindow = SDL_CreateWindow("player demo", 20, 20, pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
	if (pSDLWindow == NULL)
	{
		printf("cannot create window,error:%s\n", SDL_GetError());
		return -1;
	}

	pSDLRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0);
	if (pSDLRenderer == NULL)
	{
		printf("cannot create renderer,error:%s\n", SDL_GetError());
		return -1;
	}

	pSDLTexture = SDL_CreateTexture(pSDLRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
	if (pSDLTexture == NULL)
	{
		printf("cannot create texture,error:%s\n", SDL_GetError());
		return -1;
	}

	//decode and play process
	SDL_Event event;
	AVFrame *pFrame, *pYuvFrame;
	AVPacket *pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
	SDL_Thread *sdlThread = SDL_CreateThread(sfp_refresh_thread, NULL, &pCodecCtx->framerate);

	pFrame = av_frame_alloc();
	pYuvFrame = av_frame_alloc();
	unsigned char *out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pYuvFrame->data, pYuvFrame->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width
		, pCodecCtx->height, 1);
	SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	renderRect.x = 0;
	renderRect.y = 0;
	renderRect.w = pCodecCtx->width;
	renderRect.h = pCodecCtx->height;

	int got_picture;

	while (1)
	{
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			while (true)
			{
				if (av_read_frame(pFormatCtx, pPacket) < 0)
					thread_exit = 1;
				if (pPacket->stream_index == videoIndex)
					break;
			}
			int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, pPacket);
			if (ret < 0)
			{
				printf("cannot decode,\n");
				return -1;
			}
			if (got_picture)
			{
				sws_scale(swsContext, (const uint8_t *const *)pFrame->data, pFrame->linesize,
					0, pCodecCtx->height, pYuvFrame->data, pYuvFrame->linesize);

				SDL_UpdateTexture(pSDLTexture, NULL, pYuvFrame->data[0], pYuvFrame->linesize[0]);
				SDL_RenderClear(pSDLRenderer);
				SDL_RenderCopy(pSDLRenderer, pSDLTexture, NULL, NULL);
				SDL_RenderPresent(pSDLRenderer);
			}
			av_free_packet(pPacket);
		}
		else if (event.type == SFM_BREAK_EVENT)
		{
			break;
		}
	}

	sws_freeContext(swsContext);
	SDL_Quit();

	av_frame_free(&pFrame);
	av_frame_free(&pYuvFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}