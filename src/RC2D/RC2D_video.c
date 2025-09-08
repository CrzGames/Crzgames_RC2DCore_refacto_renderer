#include <RC2D/RC2D_video.h>
#include <RC2D/RC2D_logger.h>

#include <SDL3/SDL_render.h>

#include <stdio.h>
#include <stdlib.h>

// Ouvre et initialise une vidéo pour le splash screen
int rc2d_video_open(RC2D_Video* video, const char* filename, SDL_Renderer* renderer) 
{
    // Initialisation des structures
    video->format_ctx = NULL;
    video->codec_ctx = NULL;
    video->frame = NULL;
    video->frame_rgb = NULL;
    video->sws_ctx = NULL;
    video->buffer = NULL;
    video->texture = NULL;
    video->video_stream_index = -1;
    video->current_time = 0.0;
    video->is_finished = 0;

    // Ouvre le fichier vidéo
    if (avformat_open_input(&video->format_ctx, filename, NULL, NULL) < 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible d'ouvrir le fichier %s\n", filename);
        return -1;
    }

    // Récupère les informations sur les flux
    if (avformat_find_stream_info(video->format_ctx, NULL) < 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible de récupérer les infos du flux\n");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Cherche le flux vidéo
    for (unsigned int i = 0; i < video->format_ctx->nb_streams; i++) 
    {
        if (video->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) 
        {
            video->video_stream_index = i;
            break;
        }
    }
    if (video->video_stream_index == -1) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : aucun flux vidéo trouvé\n");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Trouve le décodeur
    AVCodec* codec = avcodec_find_decoder(video->format_ctx->streams[video->video_stream_index]->codecpar->codec_id);
    if (!codec) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : décodeur non trouvé\n");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Alloue le contexte du codec
    video->codec_ctx = avcodec_alloc_context3(codec);
    if (!video->codec_ctx) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible d'allouer le contexte du codec\n");
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Copie les paramètres du codec
    if (avcodec_parameters_to_context(video->codec_ctx, video->format_ctx->streams[video->video_stream_index]->codecpar) < 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible de copier les paramètres du codec\n");
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Ouvre le codec
    if (avcodec_open2(video->codec_ctx, codec, NULL) < 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible d'ouvrir le codec\n");
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Alloue les frames
    video->frame = av_frame_alloc();
    video->frame_rgb = av_frame_alloc();
    if (!video->frame || !video->frame_rgb) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible d'allouer les frames\n");
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Configure le contexte de conversion vers RGB
    video->width = video->codec_ctx->width;
    video->height = video->codec_ctx->height;
    video->sws_ctx = sws_getContext(
        video->width, video->height, video->codec_ctx->pix_fmt,
        video->width, video->height, AV_PIX_FMT_RGB24,
        SWS_FAST_BILINEAR, NULL, NULL, NULL
    );
    if (!video->sws_ctx) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible d'initialiser le contexte de conversion\n");
        av_frame_free(&video->frame);
        av_frame_free(&video->frame_rgb);
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Alloue le buffer pour les données RGB
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, video->width, video->height, 1);
    video->buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    if (!video->buffer) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible d'allouer le buffer RGB\n");
        sws_freeContext(video->sws_ctx);
        av_frame_free(&video->frame);
        av_frame_free(&video->frame_rgb);
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Configure le frame RGB
    av_image_fill_arrays(video->frame_rgb->data, video->frame_rgb->linesize, video->buffer,
                         AV_PIX_FMT_RGB24, video->width, video->height, 1);

    // Crée la texture SDL3
    video->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, video->width, video->height);
    if (!video->texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible de créer la texture SDL3 : %s\n", SDL_GetError());
        av_free(video->buffer);
        sws_freeContext(video->sws_ctx);
        av_frame_free(&video->frame);
        av_frame_free(&video->frame_rgb);
        avcodec_free_context(&video->codec_ctx);
        avformat_close_input(&video->format_ctx);
        return -1;
    }

    // Calcule la base de temps
    AVRational time_base = video->format_ctx->streams[video->video_stream_index]->time_base;
    video->time_base = av_q2d(time_base);

    return 0; // Succès
}

// Met à jour la lecture de la vidéo
int rc2d_video_update(RC2D_Video* video, double delta_time) 
{
    if (video->is_finished || !video->format_ctx) 
    {
        return -1; // Vidéo terminée ou non initialisée
    }

    // Met à jour le temps courant
    video->current_time += delta_time;

    // Lit les paquets jusqu'à obtenir une frame
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    while (av_read_frame(video->format_ctx, &packet) >= 0) 
    {
        if (packet.stream_index == video->video_stream_index) 
        {
            // Décode le paquet
            int ret = avcodec_send_packet(video->codec_ctx, &packet);
            if (ret < 0) 
            {
                RC2D_log(RC2D_LOG_ERROR, "Erreur : échec de l'envoi du paquet au décodeur\n");
                av_packet_unref(&packet);
                return -1;
            }

            // Récupère la frame
            ret = avcodec_receive_frame(video->codec_ctx, video->frame);
            if (ret == 0) 
            {
                // Frame décodée avec succès
                double frame_time = video->frame->best_effort_timestamp * video->time_base;
                if (frame_time <= video->current_time) 
                {
                    // Convertit la frame en RGB
                    sws_scale(video->sws_ctx, (const uint8_t* const*)video->frame->data,
                              video->frame->linesize, 0, video->height,
                              video->frame_rgb->data, video->frame_rgb->linesize);

                    // Met à jour la texture SDL3
                    SDL_UpdateTexture(video->texture, NULL, video->frame_rgb->data[0], video->frame_rgb->linesize[0]);
                    av_packet_unref(&packet);
                    return 1; // Nouvelle frame prête
                }
            } 
            else if (ret == AVERROR(EAGAIN)) 
            {
                // Pas encore de frame disponible
            } 
            else if (ret == AVERROR_EOF) 
            {
                video->is_finished = 1;
                av_packet_unref(&packet);
                return 0; // Fin de la vidéo
            } 
            else 
            {
                RC2D_log(RC2D_LOG_ERROR, "Erreur : échec de la réception de la frame\n");
                av_packet_unref(&packet);
                return -1;
            }
        }
        av_packet_unref(&packet);
    }

    // Fin du flux
    video->is_finished = 1;
    return 0;
}

// Dessine la frame actuelle
int rc2d_video_draw(RC2D_Video* video, SDL_Renderer* renderer) 
{
    if (!video->texture || video->is_finished) 
    {
        return -1; // Pas de texture ou vidéo terminée
    }

    // Utiliser la résolution logique pour centrer la vidéo
    int logical_w, logical_h;
    SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(renderer, &logical_w, &logical_h, &mode);

    // Calculer le rectangle de destination pour respecter le ratio d'aspect
    float video_aspect = (float)video->width / video->height;
    float logical_aspect = (float)logical_w / logical_h;

    SDL_FRect dst_rect;
    if (video_aspect > logical_aspect) 
    {
        // La vidéo est plus large : ajuster la hauteur
        dst_rect.w = (float)logical_w;
        dst_rect.h = logical_w / video_aspect;
        dst_rect.x = 0;
        dst_rect.y = (logical_h - dst_rect.h) / 2; // Centrer verticalement
    } 
    else 
    {
        // La vidéo est plus haute : ajuster la largeur
        dst_rect.h = (float)logical_h;
        dst_rect.w = logical_h * video_aspect;
        dst_rect.x = (logical_w - dst_rect.w) / 2; // Centrer horizontalement
        dst_rect.y = 0;
    }

    if (!SDL_RenderTexture(renderer, video->texture, NULL, &dst_rect)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : échec du rendu de la texture : %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

// Libère les ressources
void rc2d_video_close(RC2D_Video* video) 
{
    if (video->texture) 
    {
        SDL_DestroyTexture(video->texture);
        video->texture = NULL;
    }
    if (video->buffer) 
    {
        av_free(video->buffer);
        video->buffer = NULL;
    }
    if (video->sws_ctx) 
    {
        sws_freeContext(video->sws_ctx);
        video->sws_ctx = NULL;
    }
    if (video->frame_rgb) 
    {
        av_frame_free(&video->frame_rgb);
    }
    if (video->frame) 
    {
        av_frame_free(&video->frame);
    }
    if (video->codec_ctx) 
    {
        avcodec_free_context(&video->codec_ctx);
    }
    if (video->format_ctx) 
    {
        avformat_close_input(&video->format_ctx);
    }
}