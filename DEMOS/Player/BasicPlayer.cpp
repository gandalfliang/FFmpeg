#include <stdio.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "SDL/SDL.h"

int main(int argc, char *argv[])
{
	if (argc == 0)
		return 1;
	const char *filePath = argv[0];

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

	pSDLWindow = SDL_CreateWindow("player demo", 0, 0, pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
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
}