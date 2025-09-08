#include <RC2D/RC2D_filesystem.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_filesystem.h>

static char* prefPath = NULL;
static char* basePath = NULL;

char* rc2d_filesystem_getWritableAppDataPath(const char* nameOrganisation, const char* nameApplication)
{
    if (nameOrganisation == NULL || nameApplication == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_filesystem_getWritableAppDataPath error : nameOrganisation or nameApplication is NULL \n");
        return NULL;
    }

    prefPath = SDL_GetPrefPath(nameOrganisation, nameApplication);

    // NULL en cas de probleme (échec de la création du répertoire, etc.)
    if (prefPath == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_filesystem_getPrefPath failed pointer SDL_GetPrefPath : %s", SDL_GetError());        
        return NULL;
    }

    return prefPath;
}

char* rc2d_filesystem_getPathApp(void)
{
    basePath = SDL_GetBasePath();
    
    // NULL sera renvoye en cas d'erreur ou lorsque la plateforme n'implemente pas cette fonctionnalite
    if (basePath == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_filesystem_getBasePath failed pointer SDL_GetBasePath : %s", SDL_GetError());
        return NULL;
    }

    return basePath;
}

void rc2d_filesystem_quit(void)
{
    RC2D_safe_free(prefPath);
    RC2D_safe_free(basePath);
}

char* rc2d_filesystem_getPathAssetsInResourceRRES(void)
{
    char* path = "./assets/";
    return path;
}