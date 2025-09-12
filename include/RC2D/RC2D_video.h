#ifndef RC2D_VIDEO_H
#define RC2D_VIDEO_H

/**
* IMPORTANT:
* Sur Android, on'as renssencé un problème ou ça n'afficher jamais les vidéo à partir de la callback rc2d_load().
* Par contre, si on lance la vidéo plus tard dans le code (après l'appel de rc2d_load()), à partir de
* rc2d_update() par exemple, ça fonctionne parfaitement.
* Donc pour l'instant, on ne supporte pas l'ouverture de vidéo dans rc2d_load() sur Android.
*/

#if RC2D_VIDEO_MODULE_ENABLED

#include <RC2D/RC2D_storage.h>

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

/**
 * \def RC2D_TEX_RING
 * \brief Nombre de textures utilisées pour le triple buffering vidéo.
 *
 * \details
 * Le décodeur écrit dans une texture pendant qu'une autre est affichée.
 * Un ring buffer de 3 textures permet de limiter le tearing et les stalls GPU.
 *
 * \since This macro is available since RC2D 1.0.0.
 */
#define RC2D_TEX_RING 3

/**
 * \brief Contexte de lecture vidéo (et audio) pour films / splash screens.
 *
 * \details
 * Cette structure encapsule tout l'état nécessaire à la lecture d'une vidéo :
 * - Décodage vidéo via FFmpeg (packet -> frame -> conversion YUV).
 * - Conversion de couleurs (SWS) vers YUV420P/IYUV pour un upload rapide dans SDL.
 * - Un ring de textures SDL (triple buffering) pour un rendu fluide.
 * - Décodage audio optionnel, conversion vers PCM S16 stéréo 44.1 kHz,
 *   et lecture via le mixer RC2D (SDL3_mixer).
 * - Une horloge interne lissée pour synchroniser le pacing vidéo, avec drop
 *   des frames trop en retard.
 *
 * \since This struct is available since RC2D 1.0.0.
 */
typedef struct RC2D_Video {
    /* ---------------- FFmpeg (vidéo) ---------------- */

    /**
     * \brief Contexte format (container) FFmpeg.
     */
    AVFormatContext* format_ctx;

    /**
     * \brief Contexte codec vidéo FFmpeg (décodeur).
     */
    AVCodecContext*  codec_ctx;

    /**
     * \brief Frame décodée dans le format source (pix_fmt du flux).
     */
    AVFrame*         frame;

    /**
     * \brief Frame convertie en YUV420P (IYUV) prête à uploader dans SDL.
     */
    AVFrame*         frame_yuv;

    /**
     * \brief Contexte SWS (conversion de couleurs / resize).
     */
    SwsContext*      sws_ctx;

    /**
     * \brief Index du flux vidéo dans le container.
     */
    int              video_stream_index;

    /**
     * \brief Buffer détenu par FFmpeg pour stocker frame_yuv.
     */
    uint8_t*         buffer;

    /* ---------------- SDL (textures vidéo) ---------------- */

    /**
     * \brief Ring de textures IYUV (triple buffering).
     */
    SDL_Texture*     textures[RC2D_TEX_RING];

    /**
     * \brief Pointeur vers la texture courante publiée pour le rendu.
     */
    SDL_Texture*     texture;

    /**
     * \brief Index circulaire dans le ring de textures.
     */
    int              tex_index;

    /* ---------------- Métadonnées vidéo ---------------- */

    /**
     * \brief Largeur de la vidéo (pixels).
     */
    int              width;

    /**
     * \brief Hauteur de la vidéo (pixels).
     */
    int              height;

    /**
     * \brief Valeur en secondes d'un tick PTS (time base du flux vidéo).
     */
    double           time_base;

    /**
     * \brief Durée moyenne d'une frame (fallback si FPS inconnu).
     */
    double           frame_duration;

    /* ---------------- Lecture / pacing ---------------- */

    /**
     * \brief Indique si la vidéo a atteint la fin (EOF).
     */
    int              is_finished;

    /**
     * \brief Horloge logique de lecture (secondes), lissée (wall_dt/dt).
     */
    double           clock_time;

    /**
     * \brief Indique si une frame est prête mais pas encore "due" (PTS futur).
     */
    int              has_pending_frame;

    /**
     * \brief PTS (secondes) de la prochaine frame à publier.
     */
    double           next_frame_pts;

    /* ---------------- Horloge haute résolution ---------------- */

    /**
     * \brief Fréquence du compteur de performance SDL (ticks par seconde).
     */
    Uint64           perf_freq;

    /**
     * \brief Dernière valeur de référence du compteur (pour delta).
     */
    Uint64           perf_t0;

    /* ---------------- FFmpeg (audio) ---------------- */

    /**
     * \brief Contexte codec audio FFmpeg (décodeur) si flux audio présent.
     */
    AVCodecContext*  audio_codec_ctx;

    /**
     * \brief Index du flux audio dans le container, ou -1 si absent.
     */
    int              audio_stream_index;

    /**
     * \brief Frame audio décodée au format source.
     */
    AVFrame*         audio_frame;

    /**
     * \brief Contexte SWR (resampling / conversion de format).
     */
    SwrContext*      swr_ctx;

    /**
     * \brief Buffer PCM interleavé (S16 stéréo), propriété de RC2D_Video.
     */
    uint8_t*         audio_buffer;

    /**
     * \brief Taille allouée du buffer PCM (octets).
     */
    size_t           audio_buffer_size;

    /**
     * \brief Octets utiles écrits dans le buffer PCM.
     */
    size_t           audio_buffer_used;

    /* ---------------- RC2D audio (SDL3_mixer) ---------------- */

    /**
     * \brief Ressource audio du mixer (PCM prêt à jouer), peut être NULL.
     */
    MIX_Audio*       mix_audio;

    /**
     * \brief Piste de lecture du mixer, peut être NULL si audio absent.
     */
    MIX_Track*       mix_track;

    /* ---------------- Infos audio / horloge ---------------- */

    /**
     * \brief Time base du flux audio (secondes par tick).
     */
    double           audio_time_base;

    /**
     * \brief Horloge audio courante (secondes), utile pour la resync.
     */
    double           audio_clock;

    /**
     * \brief Spécification du format audio utilisé pour le mixage.
     *
     * \details
     * Fréquence: 44100 Hz, Format: S16, Canaux: 2 (stéréo).
     */
    SDL_AudioSpec    audio_spec;

    /* Fade-out */
    double           fade_out_start_time; /* Temps de début du fade-out (en secondes) */
    double           fade_out_duration;   /* Durée du fade-out (en secondes) */

    AVIOContext    *avio;          // IO custom FFmpeg
    SDL_IOStream   *sdl_io;        // stream SDL (mémoire)
    void           *owned_mem;     // si on possède un buffer mémoire (à libérer)
    size_t          owned_len;     // taille du buffer
    int64_t         io_size;       // taille connue (pour AVSEEK_SIZE)
} RC2D_Video;

/**
 * \brief Renvoie la durée totale de la vidéo en secondes (si connue).
 *
 * \details
 * Retourne une valeur <= 0 si la durée n'est pas disponible dans le container
 * (par ex. flux live, ou absence de métadonnée de durée).
 *
 * \param {const RC2D_Video*} video - Pointeur constant vers le contexte vidéo.
 * \return {double} - Durée totale en secondes, ou <= 0 si inconnue.
 *
 * \threadsafety Appeler depuis le thread principal uniquement si vous rendez via SDL_Renderer.
 *
 * \since This function is available since RC2D 1.0.0.
 */
double rc2d_video_totalSeconds(const RC2D_Video* video);

/**
 * \brief Renvoie le temps courant (en secondes) de l'horloge du lecteur.
 *
 * \details
 * Il s'agit de l'horloge logique utilisée pour décider quand publier les frames.
 *
 * \param {const RC2D_Video*} video - Pointeur constant vers le contexte vidéo.
 * \return {double} - Temps courant en secondes (>= 0).
 *
 * \threadsafety Thread principal recommandé.
 *
 * \since This function is available since RC2D 1.0.0.
 */
double rc2d_video_currentSeconds(const RC2D_Video* video);

/**
 * \brief Ouvre et joue une vidéo depuis le storage (Title ou User).
 *
 * \details
 * - Charge le fichier vidéo depuis le storage (Title ou User) en mémoire.
 * - Initialise FFmpeg pour décoder la vidéo (et l'audio si présent).
 * - Prépare les textures SDL pour le rendu.
 * - Prépare le contexte audio et la piste de mixage si audio présent.
 *
 * \param {RC2D_Video*} video - Contexte vidéo à initialiser (doit être alloué).
 * \param {const char*} storage_path - Chemin du fichier dans le storage.
 * \param {RC2D_StorageKind} storage_kind - Type de storage (Title ou User).
 * \return {int} - 0 en cas de succès, -1 en cas d'erreur.
 *
 * \threadsafety Doit être appelé sur le thread principal.
 *
 * \since This function is available since RC2D 1.0.0.
 *
 * \see rc2d_video_close
 */
int rc2d_video_openFromStorage(RC2D_Video *video,
                               const char *storage_path,
                               RC2D_StorageKind storage_kind);

/**
 * \brief Met à jour la lecture (décodage, pacing PTS, drop des frames en retard).
 *
 * \details
 * - Avance l'horloge interne (mélange wall time et dt fourni).
 * - Décode des paquets jusqu'à obtenir une frame "due" (PTS <= clock) ou
 *   une frame future mise en attente (upload différé).
 * - Si la frame est très en retard (> 2 * frame_duration), elle est droppée
 *   pour rattraper la synchro.
 * - Démarre/maintient la synchro audio si une piste est présente.
 *
 * \param {RC2D_Video*} video - Contexte vidéo.
 * \param {double} delta_time - Delta seconde du moteur (si 0, utilise l'horloge murale).
 * \return {int} - 1 si une frame est prête/affichable, 0 si fin/EOF, -1 en cas d'erreur.
 *
 * \threadsafety Thread principal recommandé (accès renderer/texture indirect).
 *
 * \since This function is available since RC2D 1.0.0.
 *
 * \see rc2d_video_draw
 */
int rc2d_video_update(RC2D_Video* video, double delta_time);

/**
 * \brief Dessine la dernière texture vidéo publiée, en respectant le ratio d'aspect.
 *
 * \details
 * - Interroge le logical presentation du renderer (letterbox/overscan, etc.).
 * - Calcule un rectangle destination centré pour préserver le ratio de la vidéo.
 * - Rend la texture IYUV courante via \c SDL_RenderTexture .
 *
 * \param {RC2D_Video*} video - Contexte vidéo.
 * \return {int} - 0 en cas de succès, -1 en cas d'erreur (texture absente/finie).
 *
 * \threadsafety Doit être appelé sur le thread principal (SDL).
 *
 * \since This function is available since RC2D 1.0.0.
 *
 * \see rc2d_video_update
 */
int rc2d_video_draw(RC2D_Video* video);

/**
 * \brief Libère toutes les ressources associées à une vidéo.
 *
 * \details
 * - Arrête et détruit la piste audio du mixer si nécessaire.
 * - Libère les buffers PCM, SWR, frames et contexts FFmpeg.
 * - Détruit toutes les textures du ring et réinitialise les pointeurs.
 *
 * \param {RC2D_Video*} video - Contexte vidéo à fermer (peut être NULL).
 * \return void
 *
 * \threadsafety À appeler sur le thread principal (destruction de textures SDL).
 *
 * \since This function is available since RC2D 1.0.0.
 *
 * \see rc2d_video_open
 */
void rc2d_video_close(RC2D_Video* video);

#ifdef __cplusplus
}
#endif

#endif /* RC2D_VIDEO_MODULE_ENABLED */
#endif /* RC2D_VIDEO_H */
