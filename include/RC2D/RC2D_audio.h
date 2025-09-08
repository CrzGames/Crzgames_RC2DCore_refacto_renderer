#ifndef RC2D_AUDIO_H
#define RC2D_AUDIO_H

#include <SDL3_mixer/SDL_mixer.h>

/**
 * \brief Charge un fichier audio à partir d'un chemin.
 *
 * Cette fonction charge un fichier audio (par exemple WAV, MP3) dans un objet MIX_Audio.
 *
 * \param mixer Le mixer associé (peut être NULL).
 * \param path Chemin vers le fichier audio.
 * \param predecode Si true, décode complètement l'audio à la lecture.
 * \return Pointeur vers le MIX_Audio chargé, ou NULL en cas d'erreur.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
MIX_Audio* rc2d_audio_load(MIX_Mixer *mixer, const char *path, bool predecode);

/**
 * \brief Détruit un objet audio chargé.
 *
 * Cette fonction libère les ressources associées à un objet MIX_Audio.
 *
 * \param audio L'objet audio à détruire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_audio_destroy(MIX_Audio *audio);

/**
 * \brief Crée une piste audio pour le mixage.
 *
 * Cette fonction crée une nouvelle piste (track) pour jouer un son sur le mixer.
 *
 * \param mixer Le mixer sur lequel créer la piste.
 * \return Pointeur vers le MIX_Track créé, ou NULL en cas d'erreur.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
MIX_Track* rc2d_audio_createTrack(MIX_Mixer *mixer);

/**
 * \brief Détruit une piste audio.
 *
 * Cette fonction libère les ressources associées à une piste audio.
 *
 * \param track La piste à détruire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_audio_destroyTrack(MIX_Track *track);

/**
 * \brief Contrôle la lecture d'une piste audio (play, pause, stop).
 *
 * Cette fonction permet de jouer, mettre en pause ou arrêter une piste audio.
 *
 * \param track La piste audio à contrôler.
 * \param audio L'objet audio à jouer.
 * \param action Action à effectuer : 0 pour jouer, 1 pour pause, 2 pour arrêter.
 * \param loops Nombre de boucles pour la lecture (-1 pour boucle infinie, 0 pour une seule lecture).
 * \return 0 en cas de succès, -1 en cas d'erreur.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
int rc2d_audio_control(MIX_Track *track, MIX_Audio *audio, int action, int loops);

#endif // RC2D_AUDIO_H