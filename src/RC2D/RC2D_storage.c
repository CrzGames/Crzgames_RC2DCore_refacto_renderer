#include <RC2D/RC2D_storage.h>

#include <SDL3/SDL_storage.h>
#include <SDL3/SDL_stdinc.h>

#include <stdint.h>

#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>

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
    rc2d_storage_closeTitle();
    rc2d_storage_closeUser();
}

void rc2d_storage_closeTitle(void)
{
    // Ferme le storage title s’il est ouvert
    if (storage_title) 
    {
        // Ferme le storage title
        if(!SDL_CloseStorage(storage_title))
        {
            RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_closeTitle: SDL_CloseStorage failed: %s", SDL_GetError());
        }

        // Marque le storage title comme fermé
        storage_title = NULL;
    }
}

void rc2d_storage_closeUser(void)
{
    // Ferme le storage user s’il est ouvert
    if (storage_user) 
    {
        // Ferme le storage user
        if(!SDL_CloseStorage(storage_user))
        {
            RC2D_log(RC2D_LOG_ERROR, "rc2d_storage_closeUser: SDL_CloseStorage failed: %s", SDL_GetError());
        }

        // Marque le storage user comme fermé
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

/* --------------------- File size ----------------------- */
bool rc2d_storage_userGetFileSize(const char *path, Uint64 *out_len)
{
    if (!path || !out_len)
    {
        return false;
    }
    if (!storage_user)
    {
        return false;
    }
    if (!SDL_StorageReady(storage_user))
    {
        return false;
    }

    Uint64 len = 0;
    if (!SDL_GetStorageFileSize(storage_user, path, &len))
    {
        return false;
    }

    *out_len = len;
    return true;
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
    // Fallback compat: certains builds (notamment Android) exposent le storage title
    // directement a la racine des assets. Dans ce cas, "assets/..." ne doit pas etre prefixe.
    Uint64 lengthFile = 0;
    const char* resolved_path = path;
    if (!SDL_GetStorageFileSize(storage, resolved_path, &lengthFile)) 
    {
        const char* fallback_path = NULL;
        if (SDL_strncmp(path, "assets/", 7) == 0)
        {
            fallback_path = path + 7;
        }
        else if (SDL_strncmp(path, "./assets/", 9) == 0)
        {
            fallback_path = path + 9;
        }

        if (fallback_path != NULL && fallback_path[0] != '\0' &&
            SDL_GetStorageFileSize(storage, fallback_path, &lengthFile))
        {
            static bool logged_assets_prefix_fallback = false;
            resolved_path = fallback_path;

            if (!logged_assets_prefix_fallback)
            {
                RC2D_log(
                    RC2D_LOG_WARN,
                    "read_all: remap storage path 'assets/...': '%s' -> '%s'",
                    path,
                    resolved_path);
                logged_assets_prefix_fallback = true;
            }
        }
        else
        {
            RC2D_log(
                RC2D_LOG_ERROR,
                "read_all: SDL_GetStorageFileSize failed for '%s'%s%s%s: %s",
                path,
                (fallback_path != NULL && fallback_path[0] != '\0') ? " and fallback '" : "",
                (fallback_path != NULL && fallback_path[0] != '\0') ? fallback_path : "",
                (fallback_path != NULL && fallback_path[0] != '\0') ? "'" : "",
                SDL_GetError());
            return false;
        }
    }

    if (lengthFile == 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "read_all: file is empty");
        return false;
    }

    if (lengthFile > (Uint64)SIZE_MAX)
    {
        RC2D_log(RC2D_LOG_ERROR, "read_all: file size exceeds SIZE_MAX");
        return false;
    }

    // Alloue un buffer pour lire le fichier
    void *buffer = RC2D_malloc((size_t)lengthFile);
    if (!buffer) 
    {
        RC2D_log(RC2D_LOG_ERROR, "read_all: allocation failed");
        return false;
    }

    // Lit le fichier à partir du storage et dans le buffer
    if (!SDL_ReadStorageFile(storage, resolved_path, buffer, lengthFile)) 
    {
        RC2D_safe_free(buffer);
        RC2D_log(RC2D_LOG_ERROR, "read_all: SDL_ReadStorageFile failed for '%s': %s", resolved_path, SDL_GetError());
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
