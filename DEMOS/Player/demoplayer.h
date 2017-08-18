#pragma once

#ifndef _DEMOPLAYER_H_
#define _DEMOPLAYER_H_

#include <stdio.h>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "SDL/SDL.h"
}

#define SDL_main main

#endif // !DEMOPLAYER_H
