#ifndef RC2D_VIDEO_H
#define RC2D_VIDEO_H

#if RC2D_VIDEO_MODULE_ENABLED

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RC2D_TEX_RING 3

typedef struct RC2D_Video {
    /* FFmpeg */
    AVFormatContext* format_ctx;
    AVCodecContext*  codec_ctx;
    AVFrame*         frame;
    AVFrame*         frame_yuv;
    SwsContext*      sws_ctx;
    int              video_stream_index;
    uint8_t*         buffer;

    /* SDL textures : triple buffering */
    SDL_Texture*     textures[RC2D_TEX_RING];
    SDL_Texture*     texture;
    int              tex_index;

    /* Infos vidéo */
    int              width;
    int              height;
    double           time_base;
    double           frame_duration;

    /* Lecture / pacing */
    int              is_finished;
    double           clock_time;
    int              has_pending_frame;
    double           next_frame_pts;

    /* Horloge haute résolution */
    Uint64           perf_freq;
    Uint64           perf_t0;

    /* FFmpeg (audio) */
    AVCodecContext*  audio_codec_ctx;
    int              audio_stream_index;
    AVFrame*         audio_frame;
    SwrContext*      swr_ctx;
    uint8_t*         audio_buffer;
    size_t           audio_buffer_size;
    size_t           audio_buffer_used;

    /* RC2D audio */
    MIX_Audio*       mix_audio;
    MIX_Track*       mix_track;

    /* Infos audio */
    double           audio_time_base;
    double           audio_clock;

    /* Format audio du mixeur */
    SDL_AudioSpec    audio_spec;
} RC2D_Video;

int  rc2d_video_open  (RC2D_Video* video, const char* filename);
int  rc2d_video_update(RC2D_Video* video, double delta_time);
int  rc2d_video_draw  (RC2D_Video* video);
void rc2d_video_close (RC2D_Video* video);

#ifdef __cplusplus
}
#endif

#endif // RC2D_VIDEO_MODULE_ENABLED
#endif // RC2D_VIDEO_H