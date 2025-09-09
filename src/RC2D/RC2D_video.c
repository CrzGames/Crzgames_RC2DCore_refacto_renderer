#if RC2D_VIDEO_MODULE_ENABLED
#include <RC2D/RC2D_video.h>
#include <RC2D/RC2D_logger.h>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include <stdio.h>
#include <stdlib.h>

/* -- Helpers internes ----------------------------------------------------- */

static inline double rc2d_now_wall_dt(RC2D_Video* v)
{
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 diff = now - v->perf_t0;
    v->perf_t0 = now;
    if (v->perf_freq == 0) {
        v->perf_freq = SDL_GetPerformanceFrequency();
        if (v->perf_freq == 0) return 0.0;
    }
    return (double)diff / (double)v->perf_freq;
}

static inline void rc2d_upload_yuv_to_next_texture(RC2D_Video* v)
{
    SDL_Texture* cur = v->textures[v->tex_index];

    /* Upload Y, U, V (planar) dans la texture courante */
    SDL_UpdateYUVTexture(
        cur, NULL,
        v->frame_yuv->data[0], v->frame_yuv->linesize[0], /* Y */
        v->frame_yuv->data[1], v->frame_yuv->linesize[1], /* U */
        v->frame_yuv->data[2], v->frame_yuv->linesize[2]  /* V */
    );

    /* Publier cette texture comme "actuelle" pour draw() */
    v->texture = cur;

    /* Prochaine texture à remplir (ring buffer) */
    v->tex_index = (v->tex_index + 1) % RC2D_TEX_RING;
}

/* -- API ------------------------------------------------------------------ */

/* Ouvre et initialise une vidéo pour le splash screen */
int rc2d_video_open(RC2D_Video* video, const char* filename, SDL_Renderer* renderer)
{
    /* Init champs */
    video->format_ctx = NULL;
    video->codec_ctx  = NULL;
    video->frame      = NULL;
    video->frame_yuv  = NULL;
    video->sws_ctx    = NULL;
    video->buffer     = NULL;

    for (int i = 0; i < RC2D_TEX_RING; ++i) video->textures[i] = NULL;
    video->texture    = NULL;
    video->tex_index  = 0;

    video->video_stream_index = -1;
    video->width       = 0;
    video->height      = 0;

    video->time_base   = 0.0;
    video->frame_duration = 1.0 / 30.0;

    video->is_finished = 0;
    video->clock_time  = 0.0;
    video->has_pending_frame = 0;
    video->next_frame_pts    = 0.0;

    video->perf_freq = SDL_GetPerformanceFrequency();
    video->perf_t0   = SDL_GetPerformanceCounter();

    /* Ouvrir le fichier */
    if (avformat_open_input(&video->format_ctx, filename, NULL, NULL) < 0) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'ouvrir %s", filename);
        return -1;
    }

    /* Récupérer infos de flux */
    if (avformat_find_stream_info(video->format_ctx, NULL) < 0) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible de récupérer les infos de flux");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    /* Trouver le flux vidéo */
    for (unsigned int i = 0; i < video->format_ctx->nb_streams; i++) {
        if (video->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video->video_stream_index = (int)i;
            break;
        }
    }
    if (video->video_stream_index == -1) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: aucun flux vidéo trouvé");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    /* Trouver + allouer codec */
    AVCodec* codec = avcodec_find_decoder(video->format_ctx->streams[video->video_stream_index]->codecpar->codec_id);
    if (!codec) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: décodeur non trouvé");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    video->codec_ctx = avcodec_alloc_context3(codec);
    if (!video->codec_ctx) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer le contexte du codec");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    if (avcodec_parameters_to_context(video->codec_ctx,
         video->format_ctx->streams[video->video_stream_index]->codecpar) < 0) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible de copier les paramètres du codec");
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    if (avcodec_open2(video->codec_ctx, codec, NULL) < 0) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'ouvrir le codec");
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    /* Allouer frames */
    video->frame = av_frame_alloc();
    video->frame_yuv = av_frame_alloc();
    if (!video->frame || !video->frame_yuv) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer les frames");
        if (video->frame) av_frame_free(&video->frame);
        if (video->frame_yuv) av_frame_free(&video->frame_yuv);
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    /* Dimensions */
    video->width  = video->codec_ctx->width;
    video->height = video->codec_ctx->height;

    /* Contexte SWS: src = pix_fmt d'origine -> dst = YUV420P (IYUV) */
    video->sws_ctx = sws_getContext(
        video->width, video->height, video->codec_ctx->pix_fmt,
        video->width, video->height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR, NULL, NULL, NULL
    );
    if (!video->sws_ctx) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'initialiser SWS (YUV420P)");
        av_frame_free(&video->frame);
        av_frame_free(&video->frame_yuv);
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    /* Buffer pour frame YUV */
    int yuv_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, video->width, video->height, 1);
    video->buffer = (uint8_t*)av_malloc((size_t)yuv_size);
    if (!video->buffer) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer le buffer YUV");
        sws_freeContext(video->sws_ctx);
        av_frame_free(&video->frame);
        av_frame_free(&video->frame_yuv);
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    av_image_fill_arrays(video->frame_yuv->data, video->frame_yuv->linesize,
                         video->buffer, AV_PIX_FMT_YUV420P,
                         video->width, video->height, 1);

    /* Créer les textures IYUV (triple buffering) */
    for (int i = 0; i < RC2D_TEX_RING; ++i) {
        video->textures[i] = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_IYUV,           /* Y + U + V (planar) */
            SDL_TEXTUREACCESS_STREAMING,
            video->width, video->height
        );
        if (!video->textures[i]) {
            RC2D_log(RC2D_LOG_ERROR, "SDL: impossible de créer texture IYUV: %s", SDL_GetError());
            /* cleanup partiel */
            for (int j = 0; j <= i; ++j) {
                if (video->textures[j]) {
                    SDL_DestroyTexture(video->textures[j]);
                    video->textures[j] = NULL;
                }
            }
            av_free(video->buffer);
            video->buffer = NULL;
            sws_freeContext(video->sws_ctx);
            av_frame_free(&video->frame);
            av_frame_free(&video->frame_yuv);
            avcodec_free_context(&video->codec_ctx);
            avformat_close_input(&video->format_ctx);
            return -1;
        }
    }
    video->texture   = video->textures[0];
    video->tex_index = 0;

    /* Base de temps (secs par tick) */
    AVRational tb = video->format_ctx->streams[video->video_stream_index]->time_base;
    video->time_base = av_q2d(tb);

    /* Durée moyenne d'image (fallback) */
    AVRational fr = video->format_ctx->streams[video->video_stream_index]->avg_frame_rate;
    if (fr.num > 0 && fr.den > 0) {
        video->frame_duration = (double)fr.den / (double)fr.num;
    } else {
        video->frame_duration = 1.0 / 30.0;
    }

    return 0; /* succès */
}

/* Met à jour la lecture : pacing PTS + drop quand en retard + triple buffering */
int rc2d_video_update(RC2D_Video* video, double delta_time)
{
    if (!video || video->is_finished || !video->format_ctx) {
        return -1; /* vidéo terminée ou non initialisée */
    }

    /* Horloge interne haute résolution, lissée (mélange wall_dt et dt) */
    double wall_dt = rc2d_now_wall_dt(video);
    const double alpha = 0.15; /* lissage léger */
    video->clock_time += alpha * wall_dt + (1.0 - alpha) * ((delta_time > 0.0) ? delta_time : wall_dt);

    /* Si une frame est déjà prête mais pas encore due, ne refais rien */
    if (video->has_pending_frame) {
        if (video->next_frame_pts <= video->clock_time) {
            /* C'est le moment : upload vers la prochaine texture et consommer */
            rc2d_upload_yuv_to_next_texture(video);
            video->has_pending_frame = 0;
            return 1;
        } else {
            /* On attend le moment exact (draw() rend la dernière texture publiée) */
            return 1;
        }
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    /* Décoder jusqu'à obtenir une frame "due" (ou future à préparer), sinon EOF */
    while (av_read_frame(video->format_ctx, &packet) >= 0) {
        if (packet.stream_index != video->video_stream_index) {
            av_packet_unref(&packet);
            continue;
        }

        int sret = avcodec_send_packet(video->codec_ctx, &packet);
        av_packet_unref(&packet);
        if (sret < 0) {
            RC2D_log(RC2D_LOG_ERROR, "FFmpeg: échec avcodec_send_packet");
            return -1;
        }

        for (;;) {
            int rret = avcodec_receive_frame(video->codec_ctx, video->frame);
            if (rret == AVERROR(EAGAIN)) break;   /* besoin de plus de paquets */
            if (rret == AVERROR_EOF) {
                video->is_finished = 1;
                return 0; /* fin de la vidéo */
            }
            if (rret < 0) {
                RC2D_log(RC2D_LOG_ERROR, "FFmpeg: échec avcodec_receive_frame");
                return -1;
            }

            /* PTS en secondes */
            int64_t ts = (video->frame->best_effort_timestamp != AV_NOPTS_VALUE)
                       ? video->frame->best_effort_timestamp
                       : video->frame->pts;
            double pts = (ts == AV_NOPTS_VALUE) ? video->clock_time : ts * video->time_base;

            /* Drop agressif si très en retard (2 frames) */
            if (pts + 2.0 * video->frame_duration < video->clock_time) {
                /* drop cette frame et continue pour rattraper */
                continue;
            }

            /* Convertir en YUV (dans frame_yuv/ buffer) */
            sws_scale(video->sws_ctx,
                      (const uint8_t* const*)video->frame->data,
                      video->frame->linesize,
                      0, video->height,
                      video->frame_yuv->data,
                      video->frame_yuv->linesize);

            if (pts > video->clock_time) {
                /* Frame future : la garder prête, upload quand due */
                video->has_pending_frame = 1;
                video->next_frame_pts    = pts;
                return 1;
            } else {
                /* Frame due maintenant : upload immédiatement */
                rc2d_upload_yuv_to_next_texture(video);
                video->has_pending_frame = 0;
                video->next_frame_pts = pts + video->frame_duration;
                return 1;
            }
        }
    }

    /* Fin du flux (plus de paquets) */
    video->is_finished = 1;
    return 0;
}

/* Dessine la dernière texture publiée (respect ratio + présentation logique) */
int rc2d_video_draw(RC2D_Video* video, SDL_Renderer* renderer)
{
    if (!video || !video->texture || video->is_finished) {
        return -1;
    }

    int logical_w = 0, logical_h = 0;
    SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(renderer, &logical_w, &logical_h, &mode);

    /* Si pas de logique définie, fallback sur la taille vidéo */
    if (logical_w <= 0 || logical_h <= 0) {
        logical_w = video->width;
        logical_h = video->height;
    }

    float video_aspect   = (float)video->width  / (float)video->height;
    float logical_aspect = (float)logical_w     / (float)logical_h;

    SDL_FRect dst_rect;
    if (video_aspect > logical_aspect) {
        /* vidéo plus large → ajuster la hauteur */
        dst_rect.w = (float)logical_w;
        dst_rect.h = (float)logical_w / video_aspect;
        dst_rect.x = 0.0f;
        dst_rect.y = ((float)logical_h - dst_rect.h) * 0.5f;
    } else {
        /* vidéo plus haute → ajuster la largeur */
        dst_rect.h = (float)logical_h;
        dst_rect.w = (float)logical_h * video_aspect;
        dst_rect.x = ((float)logical_w - dst_rect.w) * 0.5f;
        dst_rect.y = 0.0f;
    }

    if (!SDL_RenderTexture(renderer, video->texture, NULL, &dst_rect)) {
        RC2D_log(RC2D_LOG_ERROR, "SDL: échec rendu texture vidéo: %s", SDL_GetError());
        return -1;
    }

    return 0;
}

/* Libère les ressources */
void rc2d_video_close(RC2D_Video* video)
{
    if (!video) return;

    /* Détruire textures (ring) */
    for (int i = 0; i < RC2D_TEX_RING; ++i) {
        if (video->textures[i]) {
            SDL_DestroyTexture(video->textures[i]);
            video->textures[i] = NULL;
        }
    }
    video->texture = NULL;

    if (video->buffer) {
        av_free(video->buffer);
        video->buffer = NULL;
    }
    if (video->sws_ctx) {
        sws_freeContext(video->sws_ctx);
        video->sws_ctx = NULL;
    }
    if (video->frame_yuv) {
        av_frame_free(&video->frame_yuv);
    }
    if (video->frame) {
        av_frame_free(&video->frame);
    }
    if (video->codec_ctx) {
        avcodec_free_context(&video->codec_ctx);
    }
    if (video->format_ctx) {
        avformat_close_input(&video->format_ctx);
    }

    video->is_finished = 1;
}
#endif // RC2D_VIDEO_MODULE_ENABLED