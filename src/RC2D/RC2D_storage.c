#include <RC2D/RC2D_storage.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>

#include <SDL3/SDL_storage.h>

static SDL_Storage *storage_title = NULL;
static SDL_Storage *storage_user  = NULL;

/* --------------------- Open / Close --------------------- */

bool rc2d_storage_openTitle(const char *override_path)
{
    // Vérifie si le storage est déjà ouvert
    if (storage_title) 
    {
        // Déjà ouvert, rien à faire
        return true;
    }

    // Ouvre le storage title
    SDL_PropertiesID props = 0; /* pas de props spécifiques pour l’instant */
    storage_title = SDL_OpenTitleStorage(override_path, props);
    if (!storage_title) 
    {
        // Échec de l’ouverture du storage title, log l’erreur
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_openTitle: SDL_OpenTitleStorage failed: %s", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_storage_openUser(const char *org, const char *app)
{
    // Vérifie si le storage est déjà ouvert
    if (storage_user) 
    {
        return true; /* déjà ouvert */
    }

    // Vérifie si org et app ne sont pas NULL
    if (!org || !app) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_openUser: org or app is NULL");
        return false;
    }

    // Ouvre le storage user
    SDL_PropertiesID props = 0; /* pas de props spécifiques pour l’instant */
    storage_user = SDL_OpenUserStorage(org, app, props);
    if (!storage_user) 
    {
        // Échec de l’ouverture du storage user, log l’erreur
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_openUser: SDL_OpenUserStorage failed: %s", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

void rc2d_storage_closeAll(void)
{
    // Ferme les storages s’ils sont ouverts
    if (storage_title) 
    {
        if(!SDL_CloseStorage(storage_title))
        {
            RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_closeAll: SDL_CloseStorage failed: %s", SDL_GetError());
        }
        storage_title = NULL;
    }
    if (storage_user) 
    {
        if(!SDL_CloseStorage(storage_user))
        {
            RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_closeAll: SDL_CloseStorage failed: %s", SDL_GetError());
        }
        storage_user = NULL;
    }
}

/* --------------------- Ready flags --------------------- */

bool rc2d_storage_titleReady(void)
{
    // Vérifie si le storage title est ouvert et prêt
    return (storage_title != NULL) && SDL_StorageReady(storage_title);
}

bool rc2d_storage_userReady(void)
{
    // Vérifie si le storage user est ouvert et prêt
    return (storage_user != NULL) && SDL_StorageReady(storage_user);
}

/* --------------------- User mkdir ---------------------- */
bool rc2d_storage_userMkdir(const char *path)
{
    // Vérifie si le storage user est ouvert
    if (!storage_user) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_userMkdir: user storage non ouvert");
        return false;
    }

    // Vérifie si le storage user est prêt
    if (!SDL_StorageReady(storage_user)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_userMkdir: user storage non prêt");
        return false;
    }

    // Si le storage est ouvert et prêt, crée le répertoire
    return SDL_CreateStorageDirectory(storage_user, path);
}

/* -------------- Read helpers (title / user) ------------ */

static bool read_all(SDL_Storage *storage, const char *path, void **out_data, Uint64 *out_len)
{
    // Valide si les arguments sont valides
    if (!storage || !path || !out_data || !out_len) 
    {
        RC2D_log(RC2D_LOG_ERROR, "read_all: invalid arguments");
        return false;
    }

    // Vérifie si le storage est prêt
    if (!SDL_StorageReady(storage)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "read_all: storage not ready");
        return false;
    }

    // Obtient la taille du fichier
    Uint64 lengthFile = 0;
    if (!SDL_GetStorageFileSize(storage, path, &lengthFile) || lengthFile == 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "read_all: SDL_GetStorageFileSize failed or file is empty: %s", SDL_GetError());
        return false;
    }

    // Alloue un buffer pour lire le fichier
    void *buffer = RC2D_malloc(lengthFile);
    if (!buffer) 
    {
        RC2D_log(RC2D_LOG_ERROR, "read_all: allocation failed");
        return false;
    }

    // Lit le fichier à partir du storage et dans le buffer
    if (!SDL_ReadStorageFile(storage, path, buffer, lengthFile)) 
    {
        RC2D_safe_free(buffer);
        return false;
    }

    // Remplit les sorties pour l'appelant
    *out_data = buffer;
    *out_len = lengthFile;

    // Succès
    return true;
}

bool rc2d_storage_titleReadFile(const char *path, void **out_data, Uint64 *out_len)
{
    return read_all(storage_title, path, out_data, out_len);
}

bool rc2d_storage_userReadFile(const char *path, void **out_data, Uint64 *out_len)
{
    return read_all(storage_user, path, out_data, out_len);
}

/* ---------------------- Write (user) -------------------- */

bool rc2d_storage_userWriteFile(const char *path, const void *src, Uint64 len)
{
    // Valide les arguments
    if (!src || len == 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_userWriteFile: source invalide");
        return false;
    }

    // Vérifie si le storage user est ouvert
    if (!storage_user) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_userWriteFile: user storage non ouvert");
        return false;
    }

    // Vérifie si le storage user est prêt
    if (!SDL_StorageReady(storage_user)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_userWriteFile: user storage non prêt");
        return false;
    }

    // Écrit les données dans le fichier du storage user
    if(!SDL_WriteStorageFile(storage_user, path, src, len))
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_userWriteFile: SDL_WriteStorageFile failed: %s", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}
