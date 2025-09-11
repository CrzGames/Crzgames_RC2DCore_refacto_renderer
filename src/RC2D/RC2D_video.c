#if RC2D_VIDEO_MODULE_ENABLED

#include <RC2D/RC2D_video.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_audio.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_memory.h>

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

/* Récupère la durée totale (secondes) depuis FFmpeg, ou <=0 si inconnue */
double rc2d_video_totalSeconds(const RC2D_Video* v)
{
    if (!v || !v->format_ctx) return -1.0;
    if (v->format_ctx->duration <= 0 || v->format_ctx->duration == AV_NOPTS_VALUE)
        return -1.0;
    return (double)v->format_ctx->duration / (double)AV_TIME_BASE;
}

/* Temps courant (secondes) — on utilise l’horloge du lecteur */
double rc2d_video_currentSeconds(const RC2D_Video* v)
{
    if (!v) return 0.0;
    return (v->clock_time < 0.0) ? 0.0 : v->clock_time;
}

/* Ouvre et initialise une vidéo pour le splash screen */
int rc2d_video_open(RC2D_Video* video, const char* filename)
{
    /* Init champs */
    memset(video, 0, sizeof(RC2D_Video)); /* Réinitialisation complète */

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

    /* FFmpeg (audio) */
    video->audio_codec_ctx = NULL;
    video->audio_stream_index = -1;
    video->audio_frame = NULL;
    video->swr_ctx = NULL;
    video->audio_buffer = NULL;
    video->audio_buffer_size = 0;
    video->audio_buffer_used = 0;

    /* RC2D audio */
    video->mix_audio = NULL;
    video->mix_track = NULL;

    /* Infos audio */
    video->audio_time_base = 0.0;
    video->audio_clock = 0.0;

    /* Fade-out */
    video->fade_out_start_time = -1.0; /* -1 indique pas de fade-out en cours */
    video->fade_out_duration = 6.0;    /* 6s par défaut */

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

    /* Trouver + allouer codec vidéo */
    AVCodec* codec = avcodec_find_decoder(video->format_ctx->streams[video->video_stream_index]->codecpar->codec_id);
    if (!codec) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: décodeur vidéo non trouvé");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    video->codec_ctx = avcodec_alloc_context3(codec);
    if (!video->codec_ctx) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer le contexte du codec vidéo");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    if (avcodec_parameters_to_context(video->codec_ctx,
         video->format_ctx->streams[video->video_stream_index]->codecpar) < 0) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible de copier les paramètres du codec vidéo");
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    if (avcodec_open2(video->codec_ctx, codec, NULL) < 0) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'ouvrir le codec vidéo");
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    /* Allouer frames vidéo */
    video->frame = av_frame_alloc();
    video->frame_yuv = av_frame_alloc();
    if (!video->frame || !video->frame_yuv) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer les frames vidéo");
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
            rc2d_engine_state.renderer,
            SDL_PIXELFORMAT_IYUV,           /* Y + U + V (planar) */
            SDL_TEXTUREACCESS_STREAMING,
            video->width, video->height
        );
        if (!video->textures[i]) {
            RC2D_log(RC2D_LOG_ERROR, "SDL: impossible de créer texture IYUV: %s", SDL_GetError());
            /* cleanup partiel */
            for (int j = 0; j < i; ++j) {
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

    /* Initialisation audio */
    for (unsigned int i = 0; i < video->format_ctx->nb_streams; i++) {
        if (video->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            video->audio_stream_index = (int)i;
            break;
        }
    }

    if (video->audio_stream_index != -1) {
        /* Trouver et allouer le codec audio */
        AVCodec* audio_codec = avcodec_find_decoder(
            video->format_ctx->streams[video->audio_stream_index]->codecpar->codec_id);
        if (!audio_codec) {
            RC2D_log(RC2D_LOG_INFO, "FFmpeg: décodeur audio non trouvé, audio ignoré");
            video->audio_stream_index = -1;
        } else {
            video->audio_codec_ctx = avcodec_alloc_context3(audio_codec);
            if (!video->audio_codec_ctx) {
                RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer le contexte audio");
                video->audio_stream_index = -1;
            } else if (avcodec_parameters_to_context(video->audio_codec_ctx,
                video->format_ctx->streams[video->audio_stream_index]->codecpar) < 0) {
                RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible de copier les paramètres audio");
                avcodec_free_context(&video->audio_codec_ctx);
                video->audio_stream_index = -1;
            } else if (avcodec_open2(video->audio_codec_ctx, audio_codec, NULL) < 0) {
                RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'ouvrir le codec audio");
                avcodec_free_context(&video->audio_codec_ctx);
                video->audio_stream_index = -1;
            } else {
                /* Allouer frame audio */
                video->audio_frame = av_frame_alloc();
                if (!video->audio_frame) {
                    RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer le frame audio");
                    avcodec_free_context(&video->audio_codec_ctx);
                    video->audio_stream_index = -1;
                } else {
                    /* Base de temps audio */
                    AVRational audio_tb = video->format_ctx->streams[video->audio_stream_index]->time_base;
                    video->audio_time_base = av_q2d(audio_tb);

                    /* Contexte de conversion audio vers S16 stereo 44100Hz */
                    video->swr_ctx = swr_alloc();
                    if (!video->swr_ctx) {
                        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer swr_ctx");
                        av_frame_free(&video->audio_frame);
                        avcodec_free_context(&video->audio_codec_ctx);
                        video->audio_stream_index = -1;
                    } else {
                        AVChannelLayout out_layout = AV_CHANNEL_LAYOUT_STEREO;
                        AVChannelLayout in_layout;
                        av_channel_layout_copy(&in_layout, &video->audio_codec_ctx->ch_layout);
                        if (in_layout.nb_channels == 0) {
                            av_channel_layout_default(&in_layout, 2); // Default to stereo if undefined
                        }

                        /* Use swr_alloc_set_opts2 for modern FFmpeg */
                        if (swr_alloc_set_opts2(&video->swr_ctx,
                                                &out_layout, AV_SAMPLE_FMT_S16, 44100,
                                                &in_layout, video->audio_codec_ctx->sample_fmt, video->audio_codec_ctx->sample_rate,
                                                0, NULL) < 0) {
                            RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible de configurer swr_ctx");
                            swr_free(&video->swr_ctx);
                            av_frame_free(&video->audio_frame);
                            avcodec_free_context(&video->audio_codec_ctx);
                            video->audio_stream_index = -1;
                        } else if (swr_init(video->swr_ctx) < 0) {
                            RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'initialiser swr_ctx");
                            swr_free(&video->swr_ctx);
                            av_frame_free(&video->audio_frame);
                            avcodec_free_context(&video->audio_codec_ctx);
                            video->audio_stream_index = -1;
                        } else {
                            /* Créer une piste audio RC2D */
                            video->mix_track = MIX_CreateTrack(rc2d_engine_state.mixer);
                            if (!video->mix_track) {
                                RC2D_log(RC2D_LOG_ERROR, "RC2D: impossible de créer la piste audio");
                                swr_free(&video->swr_ctx);
                                av_frame_free(&video->audio_frame);
                                avcodec_free_context(&video->audio_codec_ctx);
                                video->audio_stream_index = -1;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Si audio présent, décoder tout l'audio upfront */
    if (video->audio_stream_index != -1) {
        /* Allouer buffer initial pour PCM (estimation grossière, SDL_realloc si besoin) */
        video->audio_buffer_size = 1024 * 1024; // 1MB initial
        video->audio_buffer = (uint8_t*)RC2D_malloc(video->audio_buffer_size);
        if (!video->audio_buffer) {
            RC2D_log(RC2D_LOG_ERROR, "Impossible d'allouer buffer audio initial");
            rc2d_video_close(video);
            return -1;
        }
        video->audio_buffer_used = 0;

        AVPacket* packet = av_packet_alloc();
        if (!packet) {
            RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer paquet audio");
            RC2D_free(video->audio_buffer);
            video->audio_buffer = NULL;
            rc2d_video_close(video);
            return -1;
        }

        while (av_read_frame(video->format_ctx, packet) >= 0) {
            if (packet->stream_index == video->audio_stream_index) {
                int ret = avcodec_send_packet(video->audio_codec_ctx, packet);
                if (ret < 0) {
                    RC2D_log(RC2D_LOG_ERROR, "FFmpeg: échec avcodec_send_packet audio");
                    av_packet_unref(packet);
                    continue;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(video->audio_codec_ctx, video->audio_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                    if (ret < 0) {
                        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: échec avcodec_receive_frame audio");
                        break;
                    }

                    /* Calculer taille sortie */
                    int out_samples = swr_get_out_samples(video->swr_ctx, video->audio_frame->nb_samples);
                    size_t needed = (size_t)out_samples * 4; // S16 stereo = 4 bytes/sample frame

                    /* Realloc si besoin */
                    if (video->audio_buffer_used + needed > video->audio_buffer_size) {
                        video->audio_buffer_size *= 2;
                        uint8_t* new_buffer = (uint8_t*)RC2D_realloc(video->audio_buffer, video->audio_buffer_size);
                        if (!new_buffer) {
                            RC2D_log(RC2D_LOG_ERROR, "Impossible de RC2D_realloc buffer audio");
                            RC2D_free(video->audio_buffer);
                            video->audio_buffer = NULL;
                            av_packet_unref(packet);
                            rc2d_video_close(video);
                            av_packet_free(&packet);
                            return -1;
                        }
                        video->audio_buffer = new_buffer;
                    }

                    /* Convertir */
                    uint8_t *out_buf = video->audio_buffer + video->audio_buffer_used;
                    ret = swr_convert(video->swr_ctx, &out_buf, out_samples,
                                      (const uint8_t**)video->audio_frame->data, video->audio_frame->nb_samples);
                    if (ret < 0) {
                        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: échec swr_convert");
                        break;
                    }
                    video->audio_buffer_used += (size_t)ret * 4; // Mise à jour used
                }
            }
            av_packet_unref(packet);
        }
        av_packet_free(&packet);

        /* Reset stream to start for video */
        if (av_seek_frame(video->format_ctx, -1, 0, AVSEEK_FLAG_BACKWARD) < 0) {
            RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible de seek au début pour vidéo");
            rc2d_video_close(video);
            return -1;
        }

        /* Créer MIX_Audio à partir du buffer PCM */
        if (video->audio_buffer_used > 0) {
            SDL_AudioSpec spec = { .freq = 44100, .format = SDL_AUDIO_S16, .channels = 2 };
            video->mix_audio = MIX_LoadRawAudio(rc2d_engine_state.mixer, video->audio_buffer, video->audio_buffer_used, &spec);
            if (!video->mix_audio) {
                RC2D_log(RC2D_LOG_ERROR, "MIX_LoadRawAudio a échoué pour audio vidéo: %s", SDL_GetError());
            } else if (!MIX_SetTrackAudio(video->mix_track, video->mix_audio)) {
                RC2D_log(RC2D_LOG_ERROR, "MIX_SetTrackAudio a échoué pour audio vidéo: %s", SDL_GetError());
            }
        }
    }

    /* Gérer synchronisation audio si présent */
    if (video->mix_track && MIX_PlayTrack(video->mix_track, 0)) {
        if (!rc2d_engine_state.mixer) {
            RC2D_log(RC2D_LOG_ERROR, "Mixer not initialized before playing track");
            rc2d_video_close(video);
            return -1;
        }
        Sint64 audio_frames = MIX_GetTrackPlaybackPosition(video->mix_track);
        if (audio_frames >= 0) {
            double audio_time = (double)MIX_TrackFramesToMS(video->mix_track, audio_frames) / 1000.0;
            video->audio_clock = audio_time;
            if (fabs(video->audio_clock - video->clock_time) > 0.1) {
                /* Resynchroniser si décalage > 100ms */
                Sint64 target_frames = MIX_TrackMSToFrames(video->mix_track, (Sint64)(video->clock_time * 1000));
                MIX_SetTrackPlaybackPosition(video->mix_track, target_frames);
            }
        }
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

    /* Gérer le fade-out si en cours */
    if (video->mix_track && video->fade_out_start_time >= 0.0) {
        if (!rc2d_engine_state.mixer) {
            RC2D_log(RC2D_LOG_ERROR, "Mixer not initialized during fade-out");
            return -1;
        }
        double elapsed = video->clock_time - video->fade_out_start_time;
        if (elapsed >= video->fade_out_duration) {
            /* Fade-out terminé : arrêter la piste */
            MIX_SetTrackGain(video->mix_track, 0.0f);
            MIX_StopTrack(video->mix_track, 0);
            video->mix_track = NULL;
            video->fade_out_start_time = -1.0; /* Réinitialiser */
        } else {
            /* Boucle pour mises à jour fréquentes du gain (toutes les 5ms) */
            const double update_interval = 0.005; /* 5ms */
            double t = elapsed;
            while (t <= video->fade_out_duration && t <= elapsed + update_interval) {
                float fraction = (float)(t / video->fade_out_duration);
                float volume = powf(1.0f - fraction, 2.0f); /* Fade-out quadratique */
                if (volume < 0.0f) volume = 0.0f;
                if (!MIX_SetTrackGain(video->mix_track, volume)) {
                    RC2D_log(RC2D_LOG_ERROR, "Failed to set track gain: %s", SDL_GetError());
                }
                t += update_interval;
                if (t <= video->fade_out_duration) {
                    /* Attendre jusqu'à la prochaine mise à jour */
                    SDL_Delay((Uint32)(update_interval * 1000));
                    video->clock_time += update_interval; /* Mettre à jour l'horloge */
                }
            }
            return 0; /* Attendre la fin du fade-out */
        }
    }

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

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        RC2D_log(RC2D_LOG_ERROR, "FFmpeg: impossible d'allouer paquet");
        return -1;
    }

    /* Décoder jusqu'à obtenir une frame "due" (ou future à préparer), sinon EOF */
    while (av_read_frame(video->format_ctx, packet) >= 0) {
        if (packet->stream_index != video->video_stream_index) {
            av_packet_unref(packet);
            continue;
        }

        int sret = avcodec_send_packet(video->codec_ctx, packet);
        av_packet_unref(packet);
        if (sret < 0) {
            RC2D_log(RC2D_LOG_ERROR, "FFmpeg: échec avcodec_send_packet");
            av_packet_free(&packet);
            return -1;
        }

        for (;;) {
            int rret = avcodec_receive_frame(video->codec_ctx, video->frame);
            if (rret == AVERROR(EAGAIN)) break;   /* besoin de plus de paquets */
            if (rret == AVERROR_EOF) {
                video->is_finished = 1;
                if (video->mix_track) {
                    video->fade_out_start_time = video->clock_time; /* Débuter le fade-out */
                }
                av_packet_free(&packet);
                return 0; /* fin de la vidéo */
            }
            if (rret < 0) {
                RC2D_log(RC2D_LOG_ERROR, "FFmpeg: échec avcodec_receive_frame");
                av_packet_free(&packet);
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
                      video->frame->data,
                      video->frame->linesize,
                      0, video->height,
                      video->frame_yuv->data,
                      video->frame_yuv->linesize);

            if (pts > video->clock_time) {
                /* Frame future : la garder prête, upload quand due */
                video->has_pending_frame = 1;
                video->next_frame_pts    = pts;
                av_packet_free(&packet);
                return 1;
            } else {
                /* Frame due maintenant : upload immédiatement */
                rc2d_upload_yuv_to_next_texture(video);
                video->has_pending_frame = 0;
                video->next_frame_pts = pts + video->frame_duration;
                av_packet_free(&packet);
                return 1;
            }
        }
    }

    /* Fin du flux (plus de paquets) */
    video->is_finished = 1;
    if (video->mix_track) {
        video->fade_out_start_time = video->clock_time; /* Débuter le fade-out */
    }
    av_packet_free(&packet);
    return 0;
}

/* Dessine la dernière texture publiée (respect ratio + présentation logique) */
int rc2d_video_draw(RC2D_Video* video)
{
    if (!video || !video->texture || video->is_finished) {
        return -1;
    }

    int logical_w = 0, logical_h = 0;
    SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &logical_w, &logical_h, &mode);

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

    if (!SDL_RenderTexture(rc2d_engine_state.renderer, video->texture, NULL, &dst_rect)) {
        RC2D_log(RC2D_LOG_ERROR, "SDL: échec rendu texture vidéo: %s", SDL_GetError());
        return -1;
    }

    return 0;
}

/* Libère les ressources */
void rc2d_video_close(RC2D_Video* video)
{
    if (!video) return;

    /* Libérer ressources audio */
    if (video->mix_track) {
        MIX_SetTrackGain(video->mix_track, 0.0f); /* Assurer volume à 0 */
        MIX_StopTrack(video->mix_track, 0); // Stop immediately
        MIX_DestroyTrack(video->mix_track);
        video->mix_track = NULL;
    }
    if (video->mix_audio) {
        MIX_DestroyAudio(video->mix_audio);
        video->mix_audio = NULL;
    }
    if (video->audio_buffer) {
        RC2D_free(video->audio_buffer);
        video->audio_buffer = NULL;
    }
    if (video->swr_ctx) {
        swr_free(&video->swr_ctx);
        video->swr_ctx = NULL;
    }
    if (video->audio_frame) {
        av_frame_free(&video->audio_frame);
        video->audio_frame = NULL;
    }
    if (video->audio_codec_ctx) {
        avcodec_free_context(&video->audio_codec_ctx);
        video->audio_codec_ctx = NULL;
    }

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