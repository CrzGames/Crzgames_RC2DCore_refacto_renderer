#ifndef RC2D_VIDEO_H
#define RC2D_VIDEO_H

#if RC2D_VIDEO_MODULE_ENABLED

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <SDL3/SDL.h>

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Nombre de textures streaming pour éviter les stalls GPU */
#define RC2D_TEX_RING 3

/* Structure pour gérer le contexte de la vidéo (splash screen) */
typedef struct RC2D_Video {
    /* FFmpeg */
    AVFormatContext* format_ctx;
    AVCodecContext*  codec_ctx;
    AVFrame*         frame;       /* frame décodée (format source) */
    AVFrame*         frame_yuv;   /* frame convertie en YUV420P */
    SwsContext*      sws_ctx;
    int              video_stream_index;
    uint8_t*         buffer;      /* buffer pour frame_yuv (planar) */

    /* SDL textures : triple buffering */
    SDL_Texture*     textures[RC2D_TEX_RING];
    SDL_Texture*     texture;     /* texture courante à rendre */
    int              tex_index;   /* index de la prochaine texture à remplir */

    /* Infos vidéo */
    int              width;
    int              height;
    double           time_base;       /* secondes par unité de timestamp */
    double           frame_duration;  /* durée moyenne d'une frame (fallback) */

    /* Lecture / pacing */
    int              is_finished;     /* 1 si fin de la vidéo */
    double           clock_time;      /* horloge lecture (s), lissée */
    int              has_pending_frame; /* 1 si on a une frame convertie en attente (non uploadée) */
    double           next_frame_pts;    /* PTS en secondes de la frame en attente */

    /* Horloge haute résolution (réduit le jitter) */
    Uint64           perf_freq;
    Uint64           perf_t0;
} RC2D_Video;

/* API */
int  rc2d_video_open  (RC2D_Video* video, const char* filename, SDL_Renderer* renderer);
int  rc2d_video_update(RC2D_Video* video, double delta_time);
int  rc2d_video_draw  (RC2D_Video* video, SDL_Renderer* renderer);
void rc2d_video_close (RC2D_Video* video);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_VIDEO_MODULE_ENABLED
#endif // RC2D_VIDEO_H