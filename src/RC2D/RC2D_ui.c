#include <RC2D/RC2D_ui.h>
#include <RC2D/RC2D_engine.h>    /* rc2d_engine_getVisibleSafeRectRender */
#include <RC2D/RC2D_internal.h>  /* rc2d_engine_state.renderer */
#include <RC2D/RC2D_logger.h>

#include <SDL3/SDL.h>

/* ---------- helpers internes ---------- */

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

/* Fonction cœur : dessine selon margin_mode et renvoie le rect dessiné */
static bool rc2d__draw_anchored(RC2D_Image image,
                                RC2D_UIAnchor anchor,
                                RC2D_UIMarginMode margin_mode,
                                float margin_x, float margin_y,
                                SDL_FRect* out_drawn_rect)
{
    if (!image.sdl_texture || !rc2d_engine_state.renderer) return false;

    const SDL_FRect V = rc2d_engine_getVisibleSafeRectRender();
    if (V.w <= 0.f || V.h <= 0.f) return false;

    float tw = 0.f, th = 0.f;
    if (!rc2d__get_texture_size(image.sdl_texture, &tw, &th)) return false;

    float mx = margin_x;
    float my = margin_y;
    if (margin_mode == RC2D_UI_MARGIN_PERCENT) {
        mx = V.w * margin_x;
        my = V.h * margin_y;
    }

    const SDL_FRect dst = rc2d__anchor_rect_px(V, anchor, tw, th, mx, my);

    if (out_drawn_rect) *out_drawn_rect = dst;

    if (!SDL_RenderTexture(rc2d_engine_state.renderer, image.sdl_texture, NULL, &dst)) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_RenderTexture failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

/* ---------- API publique ---------- */

bool rc2d_ui_drawImage(RC2D_UIImage* uiImage)
{
    if (!uiImage) return false;

    SDL_FRect drawn = {0,0,0,0};
    bool ok = rc2d__draw_anchored(
        uiImage->image,
        uiImage->anchor,
        uiImage->margin_mode,
        uiImage->margin_x,
        uiImage->margin_y,
        &drawn
    );

    uiImage->last_drawn_rect = ok ? drawn : (SDL_FRect){0,0,0,0};
    return ok;
}
