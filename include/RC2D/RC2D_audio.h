#ifndef RC2D_AUDIO_H
#define RC2D_AUDIO_H

#include <RC2D/RC2D_storage.h>

#include <stdbool.h>

#include <SDL3_mixer/SDL_mixer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Charger un asset audio depuis un dossier de stockage (Title ou User).
 *
 * \details
 * Cette fonction charge un fichier audio (WAV/OGG/MP3/FLAC, etc.) depuis le
 * dossier de stockage spécifié (Title ou User) pour être joué sur le mixeur
 * déjà initialisé de l’engine. Si \p predecode est vrai, l’audio est entièrement
 * décodé en mémoire (plus de RAM, démarrage instantané et seek rapide).
 *
 * \param storage_path  Chemin relatif dans le dossier de stockage.
 * \param storage_kind  Type de dossier de stockage (RC2D_STORAGE_TITLE ou RC2D_STORAGE_USER).
 * \param predecode     Si vrai, décodage complet en mémoire ; sinon, décodage à la volée.
 *
 * \return (MIX_Audio*) Pointeur valide en cas de succès, NULL en cas d’échec.
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
MIX_Audio* rc2d_audio_loadAudioFromStorage(const char *storage_path, RC2D_StorageKind storage_kind, bool predecode);

/**
 * \brief Détruire un asset audio précédemment chargé.
 *
 * \param audio  L’asset à détruire (NULL autorisé, ne fait rien).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_audio_destroy(MIX_Audio* audio);

/**
 * \brief Créer une piste de lecture liée au mixeur courant.
 *
 * \details
 * Une piste est une « voie » unique de lecture. Créez-en quelques-unes et réutilisez-les.
 *
 * \return (MIX_Track*) Pointeur valide en cas de succès, NULL en cas d’échec.
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
MIX_Track* rc2d_track_create(void);

/**
 * \brief Détruire une piste de lecture.
 *
 * \param track  La piste à détruire (NULL autorisé, ne fait rien).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_track_destroy(MIX_Track* track);

/**
 * \brief Assigner un asset audio à une piste.
 *
 * \details
 * La piste doit avoir une entrée (audio/stream/IOStream) avant d’être lue.
 *
 * \param track  Piste cible.
 * \param audio  Asset audio chargé à associer.
 *
 * \return (bool) true si succès, false sinon (consulter les logs avec SDL_GetError()).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_track_setAudio(MIX_Track* track, MIX_Audio* audio);

/**
 * \brief Démarrer ou redémarrer la lecture d’une piste.
 *
 * \details
 * - \p loops : 0 = une seule fois ; -1 = boucle infinie ; n>0 = n boucles supplémentaires.
 * - La fonction redémarre la piste si elle est déjà en cours/pausée (comportement SDL_mixer).
 *
 * \param track  Piste à lire.
 * \param loops  Nombre de boucles (0, -1, ou >0).
 *
 * \return (bool) true si succès, false sinon (consulter les logs avec SDL_GetError()).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_track_play(MIX_Track* track, int loops);

/**
 * \brief Mettre en pause une piste en lecture.
 *
 * \param track  Piste à mettre en pause.
 *
 * \return (bool) true si succès, false sinon (consulter les logs avec SDL_GetError()).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_track_pause(MIX_Track* track);

/**
 * \brief Reprendre une piste actuellement en pause.
 *
 * \param track  Piste à reprendre.
 *
 * \return (bool) true si succès, false sinon (consulter les logs avec SDL_GetError()).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_track_resume(MIX_Track* track);

/**
 * \brief Arrêter immédiatement une piste (sans fondu de sortie).
 *
 * \param track  Piste à arrêter.
 *
 * \return (bool) true si succès, false sinon (consulter les logs avec SDL_GetError()).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_track_stop(MIX_Track* track);

/**
 * \brief Régler le gain (volume) d’une piste.
 *
 * \details
 * - 0.0f = silence, 1.0f = volume neutre, >1.0f = amplification (attention aux niveaux !).
 * - Valeurs négatives interdites (voir doc SDL_mixer).
 *
 * \param track  Piste à régler.
 * \param gain   Nouveau gain (>= 0.0f).
 *
 * \return (bool) true si succès, false sinon (consulter les logs avec SDL_GetError()).
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_track_setGain(MIX_Track* track, float gain);

#ifdef __cplusplus
}
#endif

#endif /* RC2D_AUDIO_H */
