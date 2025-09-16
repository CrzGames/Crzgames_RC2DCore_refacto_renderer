#include <RC2D/RC2D_texturepacker.h>

#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>
#include <RC2D/RC2D_storage.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_platform_defines.h>

#include <SDL3/SDL_stdinc.h>

#include <cjson/cJSON.h>

#include <string.h>

/* ========================================================================= */
/*                                 UTILITAIRES                                */
/* ========================================================================= */

static char* tp_strdup(const char* s) {
    if (!s) return NULL;
    size_t n = SDL_strlen(s) + 1;
    char* r = (char*)RC2D_malloc(n);
    if (r) SDL_memcpy(r, s, n);
    return r;
}

/* Renvoie le dossier de `path` dans `out` (inclut le slash final si présent). */
static bool tp_dirname(const char* path, char* out, size_t cap) {
    if (!path || !*path) return false;
    const char* last = SDL_strrchr(path, '/');
#ifdef RC2D_PLATFORM_WINDOWS
    const char* lastb = SDL_strrchr(path, '\\');
    if (!last || (lastb && lastb > last)) last = lastb;
#endif
    if (!last) { if (cap) out[0] = '\0'; return true; }
    size_t len = (size_t)(last - path) + 1; /* inclure le slash */
    if (len >= cap) return false;
    SDL_memcpy(out, path, len);
    out[len] = '\0';
    return true;
}

/* Concatène `a` + `b` dans `out`. */
static bool tp_join2(const char* a, const char* b, char* out, size_t cap) {
    if (!a || !b) return false;
    size_t la = SDL_strlen(a), lb = SDL_strlen(b);
    if (la + lb >= cap) return false;
    SDL_memcpy(out, a, la);
    SDL_memcpy(out + la, b, lb + 1);
    return true;
}

/* Lit un SDL_FRect {x,y,w,h} depuis un objet JSON. */
static bool tp_read_rect_f(const cJSON* obj, const char* name, SDL_FRect* out) {
    const cJSON* r = cJSON_GetObjectItemCaseSensitive(obj, name);
    if (!cJSON_IsObject(r)) return false;
    const cJSON* jx = cJSON_GetObjectItemCaseSensitive(r, "x");
    const cJSON* jy = cJSON_GetObjectItemCaseSensitive(r, "y");
    const cJSON* jw = cJSON_GetObjectItemCaseSensitive(r, "w");
    const cJSON* jh = cJSON_GetObjectItemCaseSensitive(r, "h");
    if (!cJSON_IsNumber(jx) || !cJSON_IsNumber(jy) || !cJSON_IsNumber(jw) || !cJSON_IsNumber(jh))
        return false;
    out->x = (float)jx->valuedouble;
    out->y = (float)jy->valuedouble;
    out->w = (float)jw->valuedouble;
    out->h = (float)jh->valuedouble;
    return true;
}

/* Lit un {w,h} vers SDL_FPoint(x=w, y=h). */
static bool tp_read_wh_f(const cJSON* obj, const char* name, SDL_FPoint* out) {
    const cJSON* r = cJSON_GetObjectItemCaseSensitive(obj, name);
    if (!cJSON_IsObject(r)) return false;
    const cJSON* jw = cJSON_GetObjectItemCaseSensitive(r, "w");
    const cJSON* jh = cJSON_GetObjectItemCaseSensitive(r, "h");
    if (!cJSON_IsNumber(jw) || !cJSON_IsNumber(jh))
        return false;
    out->x = (float)jw->valuedouble;
    out->y = (float)jh->valuedouble;
    return true;
}

/* Parse une entrée de "frames" en RC2D_TP_Frame. */
static bool tp_parse_frame(const cJSON* fobj, RC2D_TP_Frame* out) {
    SDL_memset(out, 0, sizeof(*out));

    const cJSON* jname = cJSON_GetObjectItemCaseSensitive(fobj, "filename");
    if (!cJSON_IsString(jname)) return false;
    out->filename = tp_strdup(jname->valuestring);

    if (!tp_read_rect_f(fobj, "frame", &out->frame)) return false;

    /* spriteSourceSize = {x,y,w,h} */
    {
        const cJSON* sss = cJSON_GetObjectItemCaseSensitive(fobj, "spriteSourceSize");
        if (cJSON_IsObject(sss)) {
            const cJSON* jx = cJSON_GetObjectItemCaseSensitive(sss, "x");
            const cJSON* jy = cJSON_GetObjectItemCaseSensitive(sss, "y");
            const cJSON* jw = cJSON_GetObjectItemCaseSensitive(sss, "w");
            const cJSON* jh = cJSON_GetObjectItemCaseSensitive(sss, "h");
            if (cJSON_IsNumber(jx) && cJSON_IsNumber(jy) && cJSON_IsNumber(jw) && cJSON_IsNumber(jh)) {
                out->spriteSourceSize.x = (float)jx->valuedouble;
                out->spriteSourceSize.y = (float)jy->valuedouble;
                out->spriteSourceSize.w = (float)jw->valuedouble;
                out->spriteSourceSize.h = (float)jh->valuedouble;
            }
        }
    }

    /* sourceSize = {w,h} */
    tp_read_wh_f(fobj, "sourceSize", &out->sourceSize);

    return true;
}

/* Libère le contenu d'une frame (nom). */
static void tp_free_frame(RC2D_TP_Frame* f) {
    if (!f) return;
    if (f->filename) { RC2D_free(f->filename); f->filename = NULL; }
}

/* ========================================================================= */
/*                               API PUBLIQUE                                 */
/* ========================================================================= */

RC2D_TP_Atlas rc2d_tp_loadAtlasFromStorage(const char* json_path, RC2D_StorageKind storage_kind)
{
    RC2D_TP_Atlas atlas = (RC2D_TP_Atlas){0};

    if (!json_path || !*json_path) 
    {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: invalid json_path");
        return atlas;
    }

    /* 1) Charger le JSON depuis le storage RC2D */
    void* bytes = NULL; Uint64 len = 0;
    bool ok = false;
    if (storage_kind == RC2D_STORAGE_TITLE) {
        ok = rc2d_storage_titleReadFile(json_path, &bytes, &len);
    } else if (storage_kind == RC2D_STORAGE_USER) {
        ok = rc2d_storage_userReadFile(json_path, &bytes, &len);
    } else {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: invalid storage_kind");
        return atlas;
    }
    if (!ok || !bytes || len == 0) {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: failed to read '%s' from storage", json_path);
        return atlas;
    }

    /* 2) Parser le JSON */
    cJSON* root = cJSON_ParseWithLength((const char*)bytes, (size_t)len);
    RC2D_free(bytes); bytes = NULL;
    if (!root) {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: JSON parse failed for '%s'", json_path);
        return atlas;
    }

    const cJSON* jframes = cJSON_GetObjectItemCaseSensitive(root, "frames");
    const cJSON* jmeta   = cJSON_GetObjectItemCaseSensitive(root, "meta");
    if (!cJSON_IsArray(jframes) || !cJSON_IsObject(jmeta)) {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: invalid JSON (frames/meta)");
        cJSON_Delete(root);
        return atlas;
    }

    /* 3) Méta (image + size) */
    const cJSON* jimage = cJSON_GetObjectItemCaseSensitive(jmeta, "image");
    if (cJSON_IsString(jimage)) {
        atlas.atlas_image_name = tp_strdup(jimage->valuestring);
    }

    {
        const cJSON* jsize = cJSON_GetObjectItemCaseSensitive(jmeta, "size");
        if (cJSON_IsObject(jsize)) {
            const cJSON* jw = cJSON_GetObjectItemCaseSensitive(jsize, "w");
            const cJSON* jh = cJSON_GetObjectItemCaseSensitive(jsize, "h");
            if (cJSON_IsNumber(jw) && cJSON_IsNumber(jh)) {
                atlas.atlas_size.x = (float)jw->valuedouble;
                atlas.atlas_size.y = (float)jh->valuedouble;
            }
        }
    }

    /* 4) Charger l'image d'atlas (dans le même dossier que le JSON) */
    char dir[512]; dir[0] = '\0';
    tp_dirname(json_path, dir, sizeof(dir));

    if (atlas.atlas_image_name) {
        char img_path[1024];
        if (!tp_join2(dir, atlas.atlas_image_name, img_path, sizeof(img_path))) {
            RC2D_log(RC2D_LOG_ERROR, "TexturePacker: path join failed for atlas image");
            cJSON_Delete(root);
            rc2d_tp_freeAtlas(&atlas);
            return (RC2D_TP_Atlas){0};
        }
        atlas.atlas_image = rc2d_graphics_loadImageFromStorage(img_path, storage_kind);
        if (!atlas.atlas_image.sdl_texture) {
            RC2D_log(RC2D_LOG_ERROR, "TexturePacker: failed to load atlas image '%s'", img_path);
            cJSON_Delete(root);
            rc2d_tp_freeAtlas(&atlas);
            return (RC2D_TP_Atlas){0};
        }
    } else {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: meta.image missing");
        cJSON_Delete(root);
        rc2d_tp_freeAtlas(&atlas);
        return (RC2D_TP_Atlas){0};
    }

    /* 5) Allouer les frames et parser */
    const int n = cJSON_GetArraySize(jframes);
    if (n <= 0) {
        RC2D_log(RC2D_LOG_WARN, "TexturePacker: no frames in '%s'", json_path);
        cJSON_Delete(root);
        return atlas;
    }

    atlas.frames = (RC2D_TP_Frame*)RC2D_calloc((size_t)n, sizeof(RC2D_TP_Frame));
    if (!atlas.frames) {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: out of memory for frames");
        cJSON_Delete(root);
        rc2d_tp_freeAtlas(&atlas);
        return (RC2D_TP_Atlas){0};
    }
    atlas.frame_count = n;

    for (int i = 0; i < n; ++i) {
        const cJSON* fobj = cJSON_GetArrayItem(jframes, i);
        if (!cJSON_IsObject(fobj)) {
            RC2D_log(RC2D_LOG_WARN, "TexturePacker: skipping non-object frame at index %d", i);
            continue;
        }
        if (!tp_parse_frame(fobj, &atlas.frames[i])) {
            RC2D_log(RC2D_LOG_WARN, "TexturePacker: failed parsing frame at index %d", i);
        }
    }

    cJSON_Delete(root);
    return atlas;
}

void rc2d_tp_freeAtlas(RC2D_TP_Atlas* atlas)
{
    if (!atlas) return;

    /* Libérer frames */
    for (int i = 0; i < atlas->frame_count; ++i) {
        tp_free_frame(&atlas->frames[i]);
    }
    if (atlas->frames) { RC2D_free(atlas->frames); atlas->frames = NULL; }
    atlas->frame_count = 0;

    /* Libérer nom d'image */
    if (atlas->atlas_image_name) { RC2D_free(atlas->atlas_image_name); atlas->atlas_image_name = NULL; }

    /* Détruire texture d'atlas */
    if (atlas->atlas_image.sdl_texture) {
        rc2d_graphics_freeImage(&atlas->atlas_image);
        atlas->atlas_image.sdl_texture = NULL;
    }

    atlas->atlas_size.x = atlas->atlas_size.y = 0.0f;
}

const RC2D_TP_Frame* rc2d_tp_getFrame(const RC2D_TP_Atlas* atlas, const char* filename)
{
    if (!atlas || !filename || !*filename) return NULL;
    for (int i = 0; i < atlas->frame_count; ++i) {
        RC2D_TP_Frame* f = &atlas->frames[i];
        if (f->filename && SDL_strcmp(f->filename, filename) == 0)
            return f;
    }
    return NULL;
}

void rc2d_tp_drawFrameByName(const RC2D_TP_Atlas* atlas, const char* filename,
                             float x, float y,
                             double angle,
                             float scaleX, float scaleY,
                             float offsetX, float offsetY,
                             bool flipH, bool flipV)
{
    if (!atlas || !atlas->atlas_image.sdl_texture || !filename || !*filename) {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: invalid args in rc2d_tp_drawFrameByName");
        return;
    }

    const RC2D_TP_Frame* f = rc2d_tp_getFrame(atlas, filename);
    if (!f) {
        RC2D_log(RC2D_LOG_ERROR, "TexturePacker: frame '%s' not found", filename);
        return;
    }

    /* Dessin RAW: prend le sous-rect 'frame' tel quel dans l'atlas, à (x, y). */
    RC2D_Quad q;
    q.src = f->frame;

    rc2d_graphics_drawQuad((RC2D_Image*)&atlas->atlas_image, &q,
                           x, y,
                           angle,
                           scaleX, scaleY,
                           offsetX, offsetY,
                           flipH, flipV);
}