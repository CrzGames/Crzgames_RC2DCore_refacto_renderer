#include <RC2D/RC2D_guid.h>

#include <SDL3/SDL_stdinc.h>

void rc2d_guid_toString(RC2D_GUID guid, char *buffer)
{
    SDL_GUID sdl_guid;
    SDL_memcpy(sdl_guid.data, guid.data, sizeof(sdl_guid.data));
    SDL_GUIDToString(sdl_guid, buffer, RC2D_GUID_STRING_LENGTH);
}

RC2D_GUID rc2d_guid_fromString(const char *string)
{
    SDL_GUID sdl_guid = SDL_StringToGUID(string);

    RC2D_GUID rc2d_guid;
    SDL_memcpy(rc2d_guid.data, sdl_guid.data, sizeof(rc2d_guid.data));

    return rc2d_guid;
}

bool rc2d_guid_equals(RC2D_GUID a, RC2D_GUID b)
{
    return SDL_memcmp(a.data, b.data, sizeof(a.data)) == 0;
}