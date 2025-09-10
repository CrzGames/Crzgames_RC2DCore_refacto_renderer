#include <RC2D/rc2d_ui.h>
#include <RC2D/RC2D_engine.h>    /* rc2d_engine_getVisibleSafeRectRender */
#include <RC2D/RC2D_internal.h>  /* rc2d_engine_state.renderer */
#include <RC2D/RC2D_logger.h>

#include <SDL3/SDL.h>          /* SDL_Texture, SDL_GetTextureSize, SDL_RenderTexture */

static bool rc2d__get_texture_size(SDL_Texture* tex, float* out_w, float* out_h)
{
    float tw = 0.f, th = 0.f;
    if (!tex) return false;
    if (!SDL_GetTextureSize(tex, &tw, &th)) return false;
    if (tw <= 0.f || th <= 0.f) return false;
    if (out_w) *out_w = tw;
    if (out_h) *out_h = th;
    return true;
}

static SDL_FRect rc2d__anchor_rect_px(const SDL_FRect V,
                                             RC2D_UIAnchor anchor,
                                             float w, float h,
                                             float mx, float my)
{
    SDL_FRect dst = { 0.f, 0.f, w, h };

    switch (anchor) {
        case RC2D_UI_ANCHOR_TOP_LEFT:
            dst.x = V.x + mx;
            dst.y = V.y + my;
            break;

        case RC2D_UI_ANCHOR_TOP_RIGHT:
            dst.x = V.x + V.w - mx - w;     /* marge depuis la DROITE */
            dst.y = V.y + my;
            break;

        case RC2D_UI_ANCHOR_BOTTOM_LEFT:
            dst.x = V.x + mx;
            dst.y = V.y + V.h - my - h;     /* marge depuis le BAS */
            break;

        case RC2D_UI_ANCHOR_BOTTOM_RIGHT:
            dst.x = V.x + V.w - mx - w;
            dst.y = V.y + V.h - my - h;
            break;

        case RC2D_UI_ANCHOR_TOP_CENTER:
            dst.x = V.x + (V.w - w) * 0.5f + mx;  /* mx = décalage relatif au centre horizontal */
            dst.y = V.y + my;
            break;

        case RC2D_UI_ANCHOR_BOTTOM_CENTER:
            dst.x = V.x + (V.w - w) * 0.5f + mx;
            dst.y = V.y + V.h - my - h;
            break;

        case RC2D_UI_ANCHOR_CENTER:
            dst.x = V.x + (V.w - w) * 0.5f + mx;
            dst.y = V.y + (V.h - h) * 0.5f + my;
            break;
    }

    return dst;
}

static SDL_FRect rc2d__anchor_rect_percent(const SDL_FRect V,
                                                  RC2D_UIAnchor anchor,
                                                  float w, float h,
                                                  float mx_pct, float my_pct)
{
    const float mx = V.w * mx_pct;
    const float my = V.h * my_pct;
    return rc2d__anchor_rect_px(V, anchor, w, h, mx, my);
}

/* -------------------------------- API ----------------------------------------- */

bool rc2d_ui_drawImageAnchoredPixels(RC2D_Image image,
                                     RC2D_UIAnchor anchor,
                                     float margin_x_pixels, float margin_y_pixels)
{
    if (!image.sdl_texture || !rc2d_engine_state.renderer) return false;

    const SDL_FRect V = rc2d_engine_getVisibleSafeRectRender();
    if (V.w <= 0.f || V.h <= 0.f) return false;

    float tw = 0.f, th = 0.f;
    if (!rc2d__get_texture_size(image.sdl_texture, &tw, &th)) return false;

    const SDL_FRect dst = rc2d__anchor_rect_px(V, anchor, tw, th, margin_x_pixels, margin_y_pixels);

    if (!SDL_RenderTexture(rc2d_engine_state.renderer, image.sdl_texture, NULL, &dst)) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderTexture failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool rc2d_ui_drawImageAnchoredPercentage(RC2D_Image image,
                                         RC2D_UIAnchor anchor,
                                         float margin_x_percentage, float margin_y_percentage)
{
    if (!image.sdl_texture || !rc2d_engine_state.renderer) return false;

    const SDL_FRect V = rc2d_engine_getVisibleSafeRectRender();
    if (V.w <= 0.f || V.h <= 0.f) return false;

    float tw = 0.f, th = 0.f;
    if (!rc2d__get_texture_size(image.sdl_texture, &tw, &th)) return false;

    const SDL_FRect dst = rc2d__anchor_rect_percent(V, anchor, tw, th, margin_x_percentage, margin_y_percentage);

    if (!SDL_RenderTexture(rc2d_engine_state.renderer, image.sdl_texture, NULL, &dst)) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderTexture failed: %s", SDL_GetError());
        return false;
    }
    return true;
}
