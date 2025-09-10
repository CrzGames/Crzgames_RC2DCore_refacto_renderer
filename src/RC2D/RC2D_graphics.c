#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_graphics.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>
#include <RC2D/RC2D_math.h>

#include <SDL3/SDL_iostream.h>

/**
* Couleur de rendu courante.
*/
static RC2D_Color rc2d_graphics_currentRenderColor = {0, 0, 0, 255};

void rc2d_graphics_clear(void)
{
    if (rc2d_engine_state.renderer) 
    {
        // Effacer avec la couleur noire par défaut
        SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 0, 0, 0, 255);

        // Effacer l'écran
        SDL_RenderClear(rc2d_engine_state.renderer);
    }
}

void rc2d_graphics_present(void)
{
    if (rc2d_engine_state.renderer) 
    {
        // Présenter le rendu à l'écran
        SDL_RenderPresent(rc2d_engine_state.renderer);
    }
}

void rc2d_graphics_drawImage(RC2D_Image image, float x, float y, double angle, float scaleX, float scaleY, float offsetX, float offsetY, bool flipHorizontal, bool flipVertical) 
{
    // Vérifier que la texture est valide
    if (!image.sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Invalid texture in rc2d_graphics_draw\n");
        return;
    }

    // Destination rectangle avec position, taille et scale
    SDL_FRect rectDest = {x, y, (float)image.sdl_texture->w * scaleX, (float)image.sdl_texture->h * scaleY};

    // Savoir si on doit faire un flip horizontal et/ou vertical
    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flipHorizontal) flip |= SDL_FLIP_HORIZONTAL;
    if (flipVertical) flip |= SDL_FLIP_VERTICAL;

    // Point d'origine pour la rotation (offsetX, offsetY)
    SDL_FPoint pointOffset = {(float)offsetX, (float)offsetY};

    // Appliquer le scale avant de dessiner (si scaleX et/ou scaleY != 1.0)
    SDL_SetRenderScale(rc2d_engine_state.renderer, scaleX, scaleY);

    // Si offsetX et offsetY sont valides, on les utilise, sinon on centre la rotation
    if (offsetX >= 0 && offsetY >= 0) 
    {
        if (!SDL_RenderTextureRotated(rc2d_engine_state.renderer, image.sdl_texture, NULL, &rectDest, angle, &pointOffset, flip)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "SDL_RenderTextureRotated failed in rc2d_graphics_draw: %s\n", SDL_GetError());
        }
    }
    else 
    {
        if (!SDL_RenderTextureRotated(rc2d_engine_state.renderer, image.sdl_texture, NULL, &rectDest, angle, NULL, flip)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "SDL_RenderTextureRotated failed in rc2d_graphics_draw: %s\n", SDL_GetError());
        }
    }

    // Remettre le scale à 1.0 après le dessin
    SDL_SetRenderScale(rc2d_engine_state.renderer, 1.0f, 1.0f);
}

bool rc2d_graphics_rectangle(const char* mode, const SDL_FRect *rect) 
{
    // Dessin du rectangle avec le mode spécifié (fill ou line)
    if (strcmp(mode, "fill") == 0) 
    {
        if (!SDL_RenderFillRect(rc2d_engine_state.renderer, rect)) 
        {
            // Si le dessin échoue, on log l'erreur et on retourne false
            RC2D_log(RC2D_LOG_ERROR, "SDL_RenderFillRect failed: %s\n", SDL_GetError());
            return false;
        }
    } 
    else if (strcmp(mode, "line") == 0) 
    {
        if (!SDL_RenderRect(rc2d_engine_state.renderer, rect)) 
        {
            // Si le dessin échoue, on log l'erreur et on retourne false
            RC2D_log(RC2D_LOG_ERROR, "SDL_RenderRect failed: %s\n", SDL_GetError());
            return false;
        }
    }

    // Succès
    return true;
}

bool rc2d_graphics_rectangles(const char* mode, const int numRects, const SDL_FRect *rects) 
{
    // Dessin des rectangles avec le mode spécifié (fill ou line)
    if (strcmp(mode, "fill") == 0) 
    {
        if (!SDL_RenderFillRects(rc2d_engine_state.renderer, rects, numRects)) 
        {
            // Si le dessin échoue, on log l'erreur et on retourne false
            RC2D_log(RC2D_LOG_ERROR, "SDL_RenderFillRects failed: %s\n", SDL_GetError());
            return false;
        }
    } 
    else if (strcmp(mode, "line") == 0) 
    {
        if (!SDL_RenderRects(rc2d_engine_state.renderer, rects, numRects)) 
        {
            // Si le dessin échoue, on log l'erreur et on retourne false
            RC2D_log(RC2D_LOG_ERROR, "SDL_RenderRects failed: %s\n", SDL_GetError());
            return false;
        }
    }

    // Succès
    return true;
}

bool rc2d_graphics_line(const float x1, const float y1, const float x2, const float y2)
{
    // Dessin une ligne
    if (!SDL_RenderLine(rc2d_engine_state.renderer, x1, y1, x2, y2)) 
    {
        // Si le dessin échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderLine failed: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_graphics_lines(const int numPoints, const SDL_FPoint *points) 
{
    // Dessin des lignes
    if (!SDL_RenderLines(rc2d_engine_state.renderer, points, numPoints)) 
    {
        // Si le dessin échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderLines failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

bool rc2d_graphics_point(const float x, const float y)
{
    // Dessin un point
    if (!SDL_RenderPoint(rc2d_engine_state.renderer, x, y)) 
    {
        // Si le dessin échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderPoint failed: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_graphics_points(const int numPoints, const SDL_FPoint *points) 
{
    // Dessin des points
    if (!SDL_RenderPoints(rc2d_engine_state.renderer, points, numPoints)) 
    {
        // Si le dessin échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderPoints failed: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

RC2D_ImageData rc2d_graphics_newImageDataFromStorage(const char *storage_path, RC2D_StorageKind storage_kind)
{
    // Initialisation de l'image vide
    RC2D_ImageData imageData = { NULL };

    // Validation du chemin
    if (!storage_path || !*storage_path)
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_newImageDataFromStorage: invalid storage_path");
        return imageData;
    }

    // Vérifier que le stockage est prêt
    if (storage_kind == RC2D_STORAGE_TITLE && !rc2d_storage_titleReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Title storage not ready when loading '%s'", storage_path);
        return imageData;
    }
    else if(storage_kind == RC2D_STORAGE_USER && !rc2d_storage_userReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "User storage not ready when loading '%s'", storage_path);
        return imageData;
    }

    // Lire le fichier depuis le stockage Title
    void *bytes = NULL; 
    Uint64 len = 0;
    if (storage_kind == RC2D_STORAGE_TITLE) 
    {
        if (!rc2d_storage_titleReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from Title storage", storage_path);
            return imageData;
        }
    }
    else if (storage_kind == RC2D_STORAGE_USER) 
    {
        if (!rc2d_storage_userReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from User storage", storage_path);
            return imageData;
        }
    }

    // Vérifier que le fichier n'est pas vide
    if (len == 0 || !bytes) 
    {
        RC2D_log(RC2D_LOG_ERROR, "File '%s' is empty in %s storage", storage_path, (storage_kind == RC2D_STORAGE_TITLE) ? "Title" : "User");
        return imageData;
    }

    // Crée un IO stream en lecture sur le buffer (pas de copie).
    SDL_IOStream *ioStream = SDL_IOFromConstMem(bytes, (size_t)len);
    if (!ioStream) 
    {
        RC2D_safe_free(bytes);
        RC2D_log(RC2D_LOG_ERROR, "SDL_IOFromConstMem failed for '%s': %s", storage_path, SDL_GetError());
        return imageData;
    }

    /**
     * Charger la surface depuis le stream
     * (SDL va fermer et libérer le stream automatiquement grâce au flag closeio=true)
     */
    SDL_Surface *surface = IMG_Load_IO(ioStream, /*closeio=*/true);

    // Libération du buffer mémoire
    RC2D_safe_free(bytes);

    // Vérification de la surface
    if (!surface) 
    {
        RC2D_log(RC2D_LOG_ERROR, "IMG_LoadSurface_IO('%s') failed: %s", storage_path, SDL_GetError());
        return imageData;
    }

    // Initialisation de l'image avec la surface chargée
    imageData.sdl_surface = surface;

    // Retour de l'image
    return imageData;
}

void rc2d_graphics_freeImageData(RC2D_ImageData *imageData) 
{
    if (!imageData) return;

    if (imageData->sdl_surface) 
    {
        SDL_DestroySurface(imageData->sdl_surface);
        imageData->sdl_surface = NULL;
    }
}

RC2D_Image rc2d_graphics_newImageFromStorage(const char *storage_path, RC2D_StorageKind storage_kind)
{
    // Initialisation de l'image vide
    RC2D_Image image = { NULL };

    // Validation du chemin
    if (!storage_path || !*storage_path)
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_newImageFromStorage: invalid storage_path");
        return image;
    }

    // Vérifier que le stockage est prêt
    if (storage_kind == RC2D_STORAGE_TITLE && !rc2d_storage_titleReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Title storage not ready when loading '%s'", storage_path);
        return image;
    }
    else if(storage_kind == RC2D_STORAGE_USER && !rc2d_storage_userReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "User storage not ready when loading '%s'", storage_path);
        return image;
    }

    // Lire le fichier depuis le stockage Title
    void *bytes = NULL; 
    Uint64 len = 0;
    if (storage_kind == RC2D_STORAGE_TITLE) 
    {
        if (!rc2d_storage_titleReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from Title storage", storage_path);
            return image;
        }
    }
    else if (storage_kind == RC2D_STORAGE_USER) 
    {
        if (!rc2d_storage_userReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from User storage", storage_path);
            return image;
        }
    }

    // Vérifier que le fichier n'est pas vide
    if (len == 0 || !bytes) 
    {
        RC2D_log(RC2D_LOG_ERROR, "File '%s' is empty in %s storage", storage_path, (storage_kind == RC2D_STORAGE_TITLE) ? "Title" : "User");
        return image;
    }

    // Crée un IO stream en lecture sur le buffer (pas de copie).
    SDL_IOStream *ioStream = SDL_IOFromConstMem(bytes, (size_t)len);
    if (!ioStream) 
    {
        RC2D_safe_free(bytes);
        RC2D_log(RC2D_LOG_ERROR, "SDL_IOFromConstMem failed for '%s': %s", storage_path, SDL_GetError());
        return image;
    }

    /**
     * Charger la texture depuis le stream
     * (SDL va fermer et libérer le stream automatiquement grâce au flag closeio=true)
     */
    SDL_Texture *texture = IMG_LoadTexture_IO(rc2d_engine_state.renderer, ioStream, /*closeio=*/true);

    // Libération du buffer mémoire
    RC2D_safe_free(bytes);

    // Vérification de la texture
    if (!texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "IMG_LoadTexture_IO('%s') failed: %s", storage_path, SDL_GetError());
        return image;
    }

    // Initialisation de l'image avec la texture chargée
    image.sdl_texture = texture;

    // Retour de l'image
    return image;
}

void rc2d_graphics_freeImage(RC2D_Image* image) 
{
    if (!image) return;

    if (image->sdl_texture) 
    {
        SDL_DestroyTexture(image->sdl_texture);
        image->sdl_texture = NULL;
    }
}

void rc2d_graphic_setFont(TTF_Font* font, TTF_FontStyleFlags style)
{
    // Changer le style de la police actuelle
    TTF_SetFontStyle(font, style);
}

bool rc2d_graphic_setFontSize(TTF_Font* font, float fontSize) 
{
    // Changer la taille de la police actuelle
    if(!TTF_SetFontSize(font, fontSize)) 
    {
        // Si le le changement échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "Failed to set font size: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_graphics_setColor(const RC2D_Color color)
{
    // Mise à jour de la couleur courante
    rc2d_graphics_currentRenderColor = color;

    // Application de la couleur au renderer
    if(!SDL_SetRenderDrawColor(rc2d_engine_state.renderer, color.r, color.g, color.b, color.a)) 
    {
        // Si l'application échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "Failed to set render draw color: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

void rc2d_graphics_getPixel(RC2D_ImageData imageData, const RC2D_Point point, const RC2D_Color color) {
    // Non implémenté pour l'instant
}

void rc2d_graphics_setPixel(RC2D_ImageData imageData, const RC2D_Point point, const RC2D_Color color) {
    // Non implémenté pour l'instant
}

bool rc2d_graphics_setBlendMode(const RC2D_BlendMode blendMode) 
{
    // Conversion du mode de blend RC2D en SDL
    SDL_BlendMode sdlBlendMode;
    switch (blendMode) {
        case RC2D_BLENDMODE_NONE:
            sdlBlendMode = SDL_BLENDMODE_NONE;
            break;
        case RC2D_BLENDMODE_BLEND:
            sdlBlendMode = SDL_BLENDMODE_BLEND;
            break;
        case RC2D_BLENDMODE_BLEND_PREMULTIPLIED:
            sdlBlendMode = SDL_BLENDMODE_BLEND_PREMULTIPLIED;
            break;
        case RC2D_BLENDMODE_ADD_PREMULTIPLIED:
            sdlBlendMode = SDL_BLENDMODE_ADD_PREMULTIPLIED;
            break;
        case RC2D_BLENDMODE_MOD:
            sdlBlendMode = SDL_BLENDMODE_MOD;
            break;
        case RC2D_BLENDMODE_MUL:
            sdlBlendMode = SDL_BLENDMODE_MUL;
            break;
        default:
            sdlBlendMode = SDL_BLENDMODE_INVALID;
            break;
    }

    // Vérification de la validité du mode de blend et application
    if(!SDL_SetRenderDrawBlendMode(rc2d_engine_state.renderer, sdlBlendMode)) 
    {
        // Si l'application échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "Failed to set render draw blend mode: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_graphics_scale(float scaleX, float scaleY) 
{
    // Application du scale au renderer
    if(!SDL_SetRenderScale(rc2d_engine_state.renderer, scaleX, scaleY)) 
    {
        // Si l'application échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "Failed to set render scale: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}