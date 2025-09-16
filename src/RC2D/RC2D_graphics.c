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

RC2D_Quad rc2d_graphics_newQuad(RC2D_Image* image, float x, float y, float width, float height)
{
    // Initialiser un Quad vide
    RC2D_Quad q = (RC2D_Quad){0};

    // Vérifier que l'image et la texture sont valides
    if (!image || !image->sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_newQuad: invalid image/texture");
        return q;
    }

    // Obtenir la taille de la texture
    float texW = 0.0f, texH = 0.0f;
    if (!SDL_GetTextureSize(image->sdl_texture, &texW, &texH)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_newQuad: SDL_GetTextureSize failed: %s", SDL_GetError());
        return q;
    }

    // Vérifier les dimensions si valeurs négatives ou nulles
    if (width <= 0.0f || height <= 0.0f) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_newQuad: width/height must be > 0 (got %.2f x %.2f)", width, height);
        return q;
    }

    /* Clamp dans les bornes de la texture */
    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;
    if (x + width  > texW) width  = texW - x;
    if (y + height > texH) height = texH - y;

    // Vérifier si la zone est vide après clamp
    if (width <= 0.0f || height <= 0.0f) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_newQuad: clamped area is empty");
        return q;
    }

    // Remplir le Quad avec les coordonnées source
    q.src.x = x;
    q.src.y = y;
    q.src.w = width;
    q.src.h = height;

    // Retourner le Quad valide
    return q;
}

void rc2d_graphics_drawQuad(RC2D_Image* image, const RC2D_Quad* quad,
                            float x, float y,
                            double angle,
                            float scaleX, float scaleY,
                            float offsetX, float offsetY,
                            bool flipHorizontal, bool flipVertical)
{
    if (!image || !image->sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_drawQuad: invalid image/texture");
        return;
    }
    if (!quad || quad->src.w <= 0.0f || quad->src.h <= 0.0f) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_drawQuad: invalid quad");
        return;
    }

    // Destination = taille brute (sans appliquer scaleX/scaleY ici)
    SDL_FRect dst = { x, y, quad->src.w, quad->src.h };

    // Flip
    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flipHorizontal) flip = (SDL_FlipMode)(flip | SDL_FLIP_HORIZONTAL);
    if (flipVertical)   flip = (SDL_FlipMode)(flip | SDL_FLIP_VERTICAL);

    // Centre de rotation
    const SDL_FPoint* pCenter = NULL;
    SDL_FPoint center;
    if (offsetX >= 0.0f && offsetY >= 0.0f) 
    {
        center.x = offsetX;
        center.y = offsetY;
        pCenter = &center;
    }

    // Appliquer le scale globalement
    SDL_SetRenderScale(rc2d_engine_state.renderer, scaleX, scaleY);

    if (!SDL_RenderTextureRotated(rc2d_engine_state.renderer,
                                  image->sdl_texture,
                                  &quad->src,
                                  &dst,
                                  angle,
                                  pCenter,
                                  flip))
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderTextureRotated (quad) failed: %s", SDL_GetError());
    }

    // Reset du scale pour ne pas impacter le reste
    SDL_SetRenderScale(rc2d_engine_state.renderer, 1.0f, 1.0f);
}

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

void rc2d_graphics_drawImage(RC2D_Image* image, float x, float y, double angle, float scaleX, float scaleY, float offsetX, float offsetY, bool flipHorizontal, bool flipVertical) 
{
    // Vérifier que la texture est valide
    if (!image->sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Invalid texture in rc2d_graphics_draw\n");
        return;
    }

    // Destination rectangle avec position, taille et scale
    SDL_FRect rectDest = {x, y, (float)image->sdl_texture->w * scaleX, (float)image->sdl_texture->h * scaleY};

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
        if (!SDL_RenderTextureRotated(rc2d_engine_state.renderer, image->sdl_texture, NULL, &rectDest, angle, &pointOffset, flip)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "SDL_RenderTextureRotated failed in rc2d_graphics_draw: %s\n", SDL_GetError());
        }
    }
    else 
    {
        if (!SDL_RenderTextureRotated(rc2d_engine_state.renderer, image->sdl_texture, NULL, &rectDest, angle, NULL, flip)) 
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

RC2D_ImageData rc2d_graphics_loadImageDataFromStorage(const char *storage_path, RC2D_StorageKind storage_kind)
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

RC2D_Image rc2d_graphics_loadImageFromStorage(const char *storage_path, RC2D_StorageKind storage_kind)
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

RC2D_Font rc2d_graphics_openFontFromStorage(const char* storage_path, RC2D_StorageKind storage_kind, float fontSize)
{
    // Initialisation de la police vide
    RC2D_Font font = {0};
    
    // Validation du chemin
    if (!storage_path || !*storage_path) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_openFontFromStorage: invalid path");
        return font;
    }

    // Lis le fichier via tes helpers existants
    void* bytes = NULL;
    Uint64 len  = 0;
    if (storage_kind == RC2D_STORAGE_TITLE) 
    {
        if (!rc2d_storage_titleReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from Title storage", storage_path);
            return font;
        }
    } 
    else if (storage_kind == RC2D_STORAGE_USER) 
    {
        if (!rc2d_storage_userReadFile(storage_path, &bytes, &len)) 
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to read '%s' from User storage", storage_path);
            return font;
        }
    } 
    else 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_openFontFromStorage: invalid storage kind");
        return font;
    }

    // Vérifier que le fichier n'est pas vide
    if (len == 0 || !bytes) 
    {
        RC2D_log(RC2D_LOG_ERROR, "File '%s' is empty in %s storage", storage_path, (storage_kind == RC2D_STORAGE_TITLE) ? "Title" : "User");
        return font;
    }

    // Créer un IO stream en lecture sur le buffer (pas de copie).
    SDL_IOStream* io = SDL_IOFromConstMem(bytes, (size_t)len);
    if (!io) 
    {
        RC2D_safe_free(bytes);
        RC2D_log(RC2D_LOG_ERROR, "SDL_IOFromConstMem failed: %s", SDL_GetError());
        return font;
    }

    // Charger la police depuis le stream (SDL va fermer et libérer le stream automatiquement grâce au flag closeio=true)
    TTF_Font* sdl_font = TTF_OpenFontIO(io, /*closeio=*/true, fontSize);
    if (!sdl_font) 
    {
        RC2D_safe_free(bytes);
        RC2D_log(RC2D_LOG_ERROR, "TTF_OpenFontIO('%s') failed: %s", storage_path, SDL_GetError());
        return font;
    }

    // Initialisation de la structure RC2D_Font
    font.sdl_font = sdl_font;
    font.fontSize = fontSize;
    font.style      = TTF_STYLE_NORMAL;
    font.alignment  = TTF_HORIZONTAL_ALIGN_LEFT;
    font._file_data = bytes;

    // Retour de la police créée
    return font;
}

bool rc2d_graphics_createRendererTextEngine(void)
{
    // Création du moteur de texte pour le renderer
    rc2d_engine_state.text_engine = TTF_CreateRendererTextEngine(rc2d_engine_state.renderer);
    if(!rc2d_engine_state.text_engine) 
    {
        // Si la création échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "Erreur lors de la création du moteur de texte SDL3_ttf: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

void rc2d_graphics_destroyRendererTextEngine(void)
{
    if (rc2d_engine_state.text_engine)
    {
        TTF_DestroyRendererTextEngine(rc2d_engine_state.text_engine);
        rc2d_engine_state.text_engine = NULL;
    }
}

void rc2d_graphics_closeFont(RC2D_Font* font)
{
    if (!font) return;

    if (font->sdl_font) 
    {
        TTF_CloseFont(font->sdl_font);
        font->sdl_font = NULL;
    }
    if (font->_file_data) 
    {
        RC2D_safe_free(font->_file_data);
        font->_file_data = NULL;
    }
    font->fontSize  = 0.0f;
}

/* ------------------------------------------------------------------------- */
/*                          Réglages de la police (RC2D_Font)                */
/* ------------------------------------------------------------------------- */

void rc2d_graphics_setFontStyle(RC2D_Font* font)
{
    // Changer le style de la police actuelle (lu depuis font->style)
    if (!font || !font->sdl_font) return;
    TTF_SetFontStyle(font->sdl_font, font->style);
}

bool rc2d_graphics_setFontSize(RC2D_Font* font)
{
    // Changer la taille de la police actuelle (lu depuis font->fontSize)
    if (!font || !font->sdl_font) return false;

    if(!TTF_SetFontSize(font->sdl_font, font->fontSize)) 
    {
        // Si le changement échoue, on log l'erreur et on retourne false
        RC2D_log(RC2D_LOG_ERROR, "Failed to set font size: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

void rc2d_graphics_setFontWrapAlignment(RC2D_Font* font)
{
    // Changer l'alignement du texte pour le wrapping (lu depuis font->alignment)
    if (!font || !font->sdl_font) return;
    TTF_SetFontWrapAlignment(font->sdl_font, font->alignment);
}

/* ------------------------------------------------------------------------- */
/*                                   Texte                                   */
/* ------------------------------------------------------------------------- */

RC2D_Text rc2d_graphics_createText(RC2D_Font* font, const char* string)
{
    RC2D_Text text = {0};

    // Vérification des paramètres
    if (!font || !font->sdl_font || !string || !*string) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_createText: invalid font or string");
        return text;
    }

    // Création du texte avec la police et la chaîne de caractères spécifiées
    text.sdl_text = TTF_CreateText(rc2d_engine_state.text_engine, font->sdl_font, string, 0);
    if (!text.sdl_text) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to create text: %s\n", SDL_GetError());
        return text;
    }

    // Mémoriser la chaîne (non copiée) et une couleur par défaut
    text.string = string;
    text.color  = (RC2D_Color){255, 255, 255, 255};

    // Retour du texte créé
    return text;
}

void rc2d_graphics_destroyText(RC2D_Text* text)
{
    // Vérification du paramètre
    if (!text) return;

    // Destruction du texte s'il existe
    if (text->sdl_text) 
    {
        TTF_DestroyText(text->sdl_text);
        text->sdl_text = NULL;
    }

    // On ne libère pas text->string (propriété de l'appelant)
}

/**
 * \note Version “struct-driven” : lit la chaîne depuis text->string.
 */
bool rc2d_graphics_setTextString(RC2D_Text* text)
{
    // Vérification des paramètres
    if (!text || !text->sdl_text || !text->string) 
        return false;

    // Mise à jour de la chaîne de caractères du texte
    if(!TTF_SetTextString(text->sdl_text, text->string, 0))
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to set text string: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

/**
 * \note Version “struct-driven” : lit la chaîne à appendre depuis text->string.
 */
bool rc2d_graphics_appendTextString(RC2D_Text* text)
{
    // Vérification des paramètres
    if (!text || !text->sdl_text || !text->string) 
        return false;

    // Ajout de la chaîne de caractères au texte
    if(!TTF_AppendTextString(text->sdl_text, text->string, 0))
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to append text string: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_graphics_setTextWrapWidth(RC2D_Text* text, int wrapWidth)
{
    // Vérification des paramètres
    if (!text || !text->sdl_text) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_setTextWrapWidth: invalid text or text->sdl_text");
        return false;
    }

    // Application de la largeur de wrapping au texte
    if(!TTF_SetTextWrapWidth(text->sdl_text, wrapWidth)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to set text wrap width: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

/**
 * \note Version “struct-driven” : lit la couleur depuis text->color.
 */
bool rc2d_graphics_setTextColor(RC2D_Text* text)
{
    // Vérification des paramètres
    if (!text || !text->sdl_text)
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_setTextColor: invalid text or text->sdl_text");
        return false;
    }

    // Application de la couleur au texte
    if(!TTF_SetTextColor(text->sdl_text, text->color.r, text->color.g, text->color.b, text->color.a)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to set text color: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_graphics_getTextSize(RC2D_Text* text, int* w, int* h)
{
    // Vérification des paramètres
    if (!text || !text->sdl_text || !w || !h) return false;

    // Obtenir la taille du texte
    if(!TTF_GetTextSize(text->sdl_text, w, h))
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to get text size: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

/* ------------------------------------------------------------------------- */
/*                        Mesures de chaînes avec police                      */
/* ------------------------------------------------------------------------- */

bool rc2d_graphics_getStringSize(RC2D_Font* font, const char* text,
                                 size_t length, int *w, int *h)
{
    // Vérification des paramètres
    if (!font || !font->sdl_font || !text || !w || !h) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_getStringSize: invalid parameters");
        return false;
    }

    // Obtenir la taille de la chaîne de caractères avec la police spécifiée
    if (!TTF_GetStringSize(font->sdl_font, text, length, w, h)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to get string size: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

bool rc2d_graphics_getStringSizeWrapped(RC2D_Font* font, const char* text,
                                        size_t length, int wrapLength,
                                        int *w, int *h)
{
    // Vérification des paramètres
    if (!font || !font->sdl_font || !text || !w || !h) 
    {
        RC2D_log(RC2D_LOG_ERROR, "rc2d_graphics_getStringSizeWrapped: invalid parameters");
        return false;
    }

    // Obtenir la taille de la chaîne de caractères avec la police spécifiée et le wrapping
    if (!TTF_GetStringSizeWrapped(font->sdl_font, text, length, wrapLength, w, h)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to get wrapped string size: %s\n", SDL_GetError());
        return false;
    }

    // Succès
    return true;
}

/* ------------------------------------------------------------------------- */
/*                                  Draw                                     */
/* ------------------------------------------------------------------------- */

bool rc2d_graphics_drawText(RC2D_Text* text, float x, float y) 
{
    // Vérification des paramètres
    if (!text || !text->sdl_text) return false;

    if(!TTF_DrawRendererText(text->sdl_text, x, y))
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to draw text: %s\n", SDL_GetError());
        return false;
    }

    return true;
}