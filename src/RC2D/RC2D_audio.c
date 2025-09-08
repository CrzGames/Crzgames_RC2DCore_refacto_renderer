#include <RC2D/RC2D_audio.h>
#include <RC2D/RC2D_logger.h>

MIX_Audio* rc2d_audio_load(MIX_Mixer *mixer, const char *path, bool predecode)
{
    MIX_Audio *audio = MIX_LoadAudio(mixer, path, predecode);
    if (!audio)
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur lors du chargement de l'audio %s : %s\n", path, SDL_GetError());
        return NULL;
    }
    RC2D_log(RC2D_LOG_INFO, "Audio chargé avec succès : %s\n", path);
    return audio;
}

void rc2d_audio_destroy(MIX_Audio *audio)
{
    if (audio)
    {
        MIX_DestroyAudio(audio);
        RC2D_log(RC2D_LOG_INFO, "Audio détruit avec succès.\n");
    }
}

MIX_Track* rc2d_audio_createTrack(MIX_Mixer *mixer)
{
    MIX_Track *track = MIX_CreateTrack(mixer);
    if (!track)
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur lors de la création de la piste : %s\n", SDL_GetError());
        return NULL;
    }
    RC2D_log(RC2D_LOG_INFO, "Piste audio créée avec succès.\n");
    return track;
}

void rc2d_audio_destroyTrack(MIX_Track *track)
{
    if (track)
    {
        MIX_DestroyTrack(track);
        RC2D_log(RC2D_LOG_INFO, "Piste audio détruite avec succès.\n");
    }
}

int rc2d_audio_control(MIX_Track *track, MIX_Audio *audio, int action, int loops)
{
    if (!track || !audio)
    {
        RC2D_log(RC2D_LOG_ERROR, "Piste ou audio invalide.\n");
        return -1;
    }

    switch (action)
    {
        case 0: // Play
            if (!MIX_PlayAudio(track, audio))
            {
                RC2D_log(RC2D_LOG_ERROR, "Erreur lors de la lecture de l'audio : %s\n", SDL_GetError());
                return -1;
            }
            // Gestion des boucles (hypothétique, à ajuster selon SDL3_mixer)
            // Si MIX_SetTrackLoops existe, utilisez-le
            // MIX_SetTrackLoops(track, loops); // Commenté car non confirmé
            RC2D_log(RC2D_LOG_INFO, "Lecture de l'audio démarrée.\n");
            break;
        case 1: // Pause
            MIX_PauseTrack(track);
            RC2D_log(RC2D_LOG_INFO, "Piste audio mise en pause.\n");
            break;
        default:
            RC2D_log(RC2D_LOG_ERROR, "Action de contrôle audio invalide : %d\n", action);
            return -1;
    }

    return 0;
}