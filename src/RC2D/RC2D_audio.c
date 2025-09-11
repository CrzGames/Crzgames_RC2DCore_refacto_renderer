#include <RC2D/RC2D_audio.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_storage.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_memory.h>

#include <SDL3/SDL_properties.h>

/* ------------------------------------------------------------------------- */
/*  Assets audio                                                             */
/* ------------------------------------------------------------------------- */

MIX_Audio* rc2d_audio_load(const char* path, bool predecode)
{
    if (!rc2d_engine_state.mixer) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Mixer non initialisé.");
        return NULL;
    }
    if (!path || *path == '\0') 
    {
        RC2D_log(RC2D_LOG_ERROR, "Chemin audio invalide.");
        return NULL;
    }

    MIX_Audio* audio = MIX_LoadAudio(rc2d_engine_state.mixer, path, predecode);
    if (!audio) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_LoadAudio('%s') a échoué : %s", path, SDL_GetError());
        return NULL;
    }

    RC2D_log(RC2D_LOG_DEBUG, "Audio chargé: '%s' (predecode=%d).", path, (int)predecode);
    return audio;
}

MIX_Audio* rc2d_audio_loadAudioFromStorage(const char *storage_path, RC2D_StorageKind storage_kind, bool predecode)
{
    // Check si le mixer est initialisé et si le chemin est valide
    if (!rc2d_engine_state.mixer) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Mixer non initialisé.");
        return NULL;
    }

    // Validation du chemin et de la valeur du storage path
    if (!storage_path || *storage_path == '\0') 
    {
        RC2D_log(RC2D_LOG_ERROR, "Chemin audio invalide.");
        return NULL;
    }

    // Vérifier que le stockage est prêt
    if (storage_kind == RC2D_STORAGE_TITLE && !rc2d_storage_titleReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Title storage not ready when loading '%s'", storage_path);
        return NULL;
    }
    else if(storage_kind == RC2D_STORAGE_USER && !rc2d_storage_userReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "User storage not ready when loading '%s'", storage_path);
        return NULL;
    }

    // Lire le fichier depuis le stockage Title
    void *bytes = NULL; 
    Uint64 len = 0;
    if (storage_kind == RC2D_STORAGE_TITLE) 
    {
        if (!rc2d_storage_titleReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from Title storage", storage_path);
            return NULL;
        }
    }
    else if (storage_kind == RC2D_STORAGE_USER) 
    {
        if (!rc2d_storage_userReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from User storage", storage_path);
            return NULL;
        }
    } 
    else 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_audio_loadFromStorage: invalid storage kind");
        return NULL;
    }

    // Vérifier que le fichier n'est pas vide
    if (len == 0 || !bytes) 
    {
        RC2D_log(RC2D_LOG_ERROR, "File '%s' is empty in %s storage", storage_path, (storage_kind == RC2D_STORAGE_TITLE) ? "Title" : "User");
        return NULL;
    }

    // Crée un IO stream en lecture sur le buffer (pas de copie).
    SDL_IOStream *ioStream = SDL_IOFromConstMem(bytes, (size_t)len);
    if (!ioStream) 
    {
        RC2D_safe_free(bytes);
        RC2D_log(RC2D_LOG_ERROR, "SDL_IOFromConstMem failed for '%s': %s", storage_path, SDL_GetError());
        return NULL;
    }

    // Charger l'audio depuis la mémoire
    MIX_Audio* audio = MIX_LoadAudio_IO(rc2d_engine_state.mixer, ioStream, predecode, /*closeio=*/true);

    // Maintenant que MIX_LoadAudio_IO a fini (et a fermé le stream), on peut libérer le buffer.
    RC2D_safe_free(bytes);

    // Vérifier que l'audio a bien été chargé
    if (!audio) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_LoadAudio_IO('%s') a échoué : %s", storage_path, SDL_GetError());
        return NULL;
    }
    
    // Succès
    return audio;
}

void rc2d_audio_destroy(MIX_Audio* audio)
{
    if (!audio) return;

    MIX_DestroyAudio(audio);
    RC2D_log(RC2D_LOG_DEBUG, "Audio détruit.");
}

/* ------------------------------------------------------------------------- */
/*  Pistes de lecture                                                        */
/* ------------------------------------------------------------------------- */

MIX_Track* rc2d_track_create(void)
{
    if (!rc2d_engine_state.mixer) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Mixer non initialisé.");
        return NULL;
    }

    MIX_Track* track = MIX_CreateTrack(rc2d_engine_state.mixer);
    if (!track) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_CreateTrack a échoué : %s", SDL_GetError());
        return NULL;
    }

    RC2D_log(RC2D_LOG_DEBUG, "Piste créée.");
    return track;
}

void rc2d_track_destroy(MIX_Track* track)
{
    if (!track) return;

    MIX_DestroyTrack(track);
    RC2D_log(RC2D_LOG_DEBUG, "Piste détruite.");
}

bool rc2d_track_setAudio(MIX_Track* track, MIX_Audio* audio)
{
    if (!track || !audio) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Paramètre NULL dans rc2d_track_setAudio.");
        return false;
    }

    if (!MIX_SetTrackAudio(track, audio)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_SetTrackAudio a échoué : %s", SDL_GetError());
        return false;
    }

    RC2D_log(RC2D_LOG_DEBUG, "Audio assigné à la piste.");
    return true;
}

bool rc2d_track_play(MIX_Track* track, int loops)
{
    if (!track) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Paramètre NULL dans rc2d_track_play.");
        return false;
    }

    SDL_PropertiesID options = SDL_CreateProperties();
    if (!options) 
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateProperties a échoué : %s", SDL_GetError());
        return false;
    }

    /* 0=une fois ; -1=infini ; n>0 = n boucles supplémentaires */
    SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, (Sint64)loops);

    bool ok = MIX_PlayTrack(track, options);
    if (!ok) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_PlayTrack a échoué (loops=%d) : %s", loops, SDL_GetError());
    } 
    else 
    {
        RC2D_log(RC2D_LOG_DEBUG, "Lecture démarrée (loops=%d).", loops);
    }

    SDL_DestroyProperties(options);
    return ok;
}

bool rc2d_track_pause(MIX_Track* track)
{
    if (!track) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Paramètre NULL dans rc2d_track_pause.");
        return false;
    }

    if (!MIX_PauseTrack(track)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_PauseTrack a échoué : %s", SDL_GetError());
        return false;
    }

    RC2D_log(RC2D_LOG_DEBUG, "Piste mise en pause.");
    return true;
}

bool rc2d_track_resume(MIX_Track* track)
{
    if (!track) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Paramètre NULL dans rc2d_track_resume.");
        return false;
    }

    if (!MIX_ResumeTrack(track)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_ResumeTrack a échoué : %s", SDL_GetError());
        return false;
    }

    RC2D_log(RC2D_LOG_DEBUG, "Piste reprise.");
    return true;
}

bool rc2d_track_stop(MIX_Track* track)
{
    if (!track) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Paramètre NULL dans rc2d_track_stop.");
        return false;
    }

    /* Arrêt immédiat (fade_out_frames = 0) */
    if (!MIX_StopTrack(track, 0)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_StopTrack a échoué : %s", SDL_GetError());
        return false;
    }

    RC2D_log(RC2D_LOG_DEBUG, "Piste arrêtée.");
    return true;
}

bool rc2d_track_setGain(MIX_Track* track, float gain)
{
    if (!track) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Paramètre NULL dans rc2d_track_setGain.");
        return false;
    }
    if (gain < 0.0f) 
    {
        RC2D_log(RC2D_LOG_WARN, "Gain négatif demandé (%.3f). Clamping à 0.0f.", gain);
        gain = 0.0f;
    }

    if (!MIX_SetTrackGain(track, gain)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "MIX_SetTrackGain(%.3f) a échoué : %s", gain, SDL_GetError());
        return false;
    }

    RC2D_log(RC2D_LOG_DEBUG, "Gain de la piste réglé à %.3f.", gain);
    return true;
}
