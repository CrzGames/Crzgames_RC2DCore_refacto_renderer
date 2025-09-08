#ifndef RC2D_VIDEO_H
#define RC2D_VIDEO_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <SDL3/SDL.h>

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

// Structure pour gérer le contexte de la vidéo
typedef struct {
    AVFormatContext* format_ctx;   // Contexte du format de la vidéo
    AVCodecContext* codec_ctx;     // Contexte du codec vidéo
    AVFrame* frame;                // Frame décodée
    AVFrame* frame_rgb;            // Frame convertie en RGB
    SwsContext* sws_ctx;           // Contexte de conversion d'image
    int video_stream_index;        // Index du flux vidéo
    uint8_t* buffer;               // Buffer pour les données RGB
    SDL_Texture* texture;           // Texture SDL3 pour le rendu
    int width;                     // Largeur de la vidéo
    int height;                    // Hauteur de la vidéo
    double time_base;              // Base de temps pour la gestion des frames
    double current_time;           // Temps actuel dans la lecture
    int is_finished;               // Indicateur de fin de lecture
} RC2D_Video;

// Fonctions pour gérer les splash screens vidéo
int rc2d_video_open(RC2D_Video* video, const char* filename, SDL_Renderer* renderer); // Ouvre et initialise la vidéo
int rc2d_video_update(RC2D_Video* video, double delta_time);  // Met à jour la lecture (gère les frames)
int rc2d_video_draw(RC2D_Video* video, SDL_Renderer* renderer); // Dessine la frame actuelle
void rc2d_video_close(RC2D_Video* video);                    // Libère les ressources

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_VIDEO_H