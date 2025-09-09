#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_graphics.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>
#include <RC2D/RC2D_math.h>

/**
* Couleur de rendu courante.
*/
static RC2D_Color rc2d_graphics_currentRenderColor = {0, 0, 0, 255};

void rc2d_graphics_clear(void)
{
    if (rc2d_engine_state.renderer) 
    {
        // 1) On dessine dans la render target
        if (rc2d_engine_state.render_target)
        {
            //SDL_SetRenderTarget(rc2d_engine_state.renderer, rc2d_engine_state.render_target);
        }

        // 2) Clear avec écran noir
        SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 0, 0, 0, 255);
        SDL_RenderClear(rc2d_engine_state.renderer);
    }
}

void rc2d_graphics_present(void)
{
    if (rc2d_engine_state.renderer) 
    {
        // 1) On revient au backbuffer
        //SDL_SetRenderTarget(rc2d_engine_state.renderer, NULL);

        // 2) Un seul draw call final
        //SDL_RenderTexture(rc2d_engine_state.renderer,
        //    rc2d_engine_state.render_target,
        //    NULL, // src = texture entière
        //    NULL  // dst = laissé à SDL (logical presentation applique letterbox/intscale)
        //);

        // 3) Présentation à l'écran
        SDL_RenderPresent(rc2d_engine_state.renderer);
    }
}

void rc2d_graphics_draw(RC2D_Image image, float x, float y, double angle, float scaleX, float scaleY, float offsetX, float offsetY, bool flipHorizontal, bool flipVertical) 
{
    // Vérifier que la texture est valide
    if (!image.sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Invalid texture in rc2d_graphics_draw\n");
        return;
    }

    // Destination rectangle avec position, taille et scale
    SDL_FRect rectDest = {x, y, (float)image.width * scaleX, (float)image.height * scaleY};

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

RC2D_ImageData rc2d_graphics_newImageData(const char* path) 
{
    RC2D_ImageData imageData = {NULL, 0, 0};

    imageData.sdl_surface = IMG_Load(path);
    if (!imageData.sdl_surface) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Unable to create surface from %s: %s\n", path, SDL_GetError());
    } 
    else 
    {
        imageData.width = imageData.sdl_surface->w;
        imageData.height = imageData.sdl_surface->h;
    }

    return imageData;
}

void rc2d_graphics_freeImageData(RC2D_ImageData imageData) 
{
    if (imageData.sdl_surface) 
    {
        SDL_DestroySurface(imageData.sdl_surface);
        imageData.sdl_surface = NULL;
    }
}

RC2D_Image rc2d_graphics_newImage(const char* path) 
{
    RC2D_Image image = {NULL, 0, 0};

    image.sdl_texture = IMG_LoadTexture(rc2d_engine_state.renderer, path);
    if (!image.sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Unable to create texture from %s: %s\n", path, SDL_GetError());
    }
    else
    {
        SDL_GetTextureSize(image.sdl_texture, &image.width, &image.height);
    }

    return image;
}

void rc2d_graphics_freeImage(RC2D_Image image) 
{
    if (image.sdl_texture) 
    {
        SDL_DestroyTexture(image.sdl_texture);
        image.sdl_texture = NULL;
    }
}

RC2D_Text rc2d_graphics_newText(RC2D_Font font, const char* textString, RC2D_Color coloredText) 
{

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