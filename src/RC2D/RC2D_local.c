#include <RC2D/RC2D_local.h>

#include <SDL3/SDL_stdinc.h>

#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>

RC2D_Locale *rc2d_local_getPreferredLocales(void)
{
    int sdl_count = 0;
    SDL_Locale **sdl_locales = SDL_GetPreferredLocales(&sdl_count);

    if (!sdl_locales || sdl_count <= 0)
    {
        RC2D_log(
            RC2D_LOG_WARN,
            "rc2d_local_getPreferredLocales: no locale detected or SDL error: %s",
            SDL_GetError());
        return NULL;
    }

    // Keep a NULL-terminated RC2D copy so callers can iterate safely.
    RC2D_Locale *result = (RC2D_Locale *)RC2D_calloc((size_t)sdl_count + 1, sizeof(RC2D_Locale));
    if (!result)
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_local_getPreferredLocales: allocation failed.");
        RC2D_safe_free(sdl_locales);
        return NULL;
    }

    for (int i = 0; i < sdl_count; ++i)
    {
        result[i].language = RC2D_strdup(sdl_locales[i]->language ? sdl_locales[i]->language : "");
        if (!result[i].language)
        {
            RC2D_log(RC2D_LOG_ERROR, "rc2d_local_getPreferredLocales: failed to duplicate language.");
            rc2d_local_freeLocales(result);
            RC2D_safe_free(sdl_locales);
            return NULL;
        }

        if (sdl_locales[i]->country)
        {
            result[i].country = RC2D_strdup(sdl_locales[i]->country);
            if (!result[i].country)
            {
                RC2D_log(RC2D_LOG_ERROR, "rc2d_local_getPreferredLocales: failed to duplicate country.");
                rc2d_local_freeLocales(result);
                RC2D_safe_free(sdl_locales);
                return NULL;
            }
        }
    }

    RC2D_safe_free(sdl_locales);
    return result;
}

void rc2d_local_freeLocales(RC2D_Locale *locales)
{
    if (locales == NULL)
    {
        return;
    }

    for (size_t index = 0; locales[index].language != NULL; ++index)
    {
        if (locales[index].language != NULL)
        {
            RC2D_free((void*)locales[index].language);
            locales[index].language = NULL;
        }

        if (locales[index].country != NULL)
        {
            RC2D_free((void*)locales[index].country);
            locales[index].country = NULL;
        }
    }

    RC2D_safe_free(locales);
}
