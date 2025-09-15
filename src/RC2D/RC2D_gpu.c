#include <RC2D/RC2D_gpu.h>
#include <RC2D/RC2D_assert.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_platform_defines.h>
#include <RC2D/RC2D_memory.h>

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>

#if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
#include <SDL3_shadercross/SDL_shadercross.h>
#endif

static bool json_read_uint(const char* s, const char* key, Uint32* out) 
{
    const char* p = SDL_strstr(s, key);       // ex: "\"uniform_buffers\""
    if (!p) return false;
    p = SDL_strchr(p, ':');                   // va au ':'
    if (!p) return false;
    p++;                                      // après ':'
    while (*p==' '||*p=='\t') p++;            // skip spaces
    char* end = NULL;
    long v = SDL_strtol(p, &end, 10);
    if (end==p || v<0) return false;
    *out = (Uint32)v;
    return true;
}

/**
 * Récupère le timestamp de la dernière modification d'un fichier.
 * 
 * @param {const char*} path - Le chemin du fichier dont on veut connaître la date de dernière modification.
 * @returns {SDL_Time} - Le timestamp de la dernière modification du fichier, ou 0 en cas d'erreur.
 */
static SDL_Time rc2d_gpu_getFileModificationTime(const char* path) 
{
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path, &info)) 
    {
        // Retourne le timestamp de la dernière modification du fichier
        return info.modify_time;
    }
    else
    {
        // Si l'appel échoue, on log l'erreur et on retourne 0
        RC2D_log(RC2D_LOG_ERROR, "Failed to get path info for %s, SDL_Error : %s", path, SDL_GetError());
        return 0;
    }
}

RC2D_GPUDevice* rc2d_gpu_getDevice(void)
{
    RC2D_assert_release(rc2d_engine_state.gpu_device != NULL, RC2D_LOG_CRITICAL, "GPU device is NULL.");
    return rc2d_engine_state.gpu_device;
}

/**
 * \brief Charge un shader graphique à partir d'un fichier source HLSL ou d'un fichier binaire précompilé.
 * 
 * Si RC2D_GPU_SHADER_HOT_RELOAD_ENABLED est défini à 1, cela compile le shader à la volée à
 * partir du fichier source HLSL. Sinon, cela charge le fichier binaire déjà précompilé.
 * 
 * L’organisation attendue dans le storage est la suivante :
 * ```
 * <racine_storage>/
 *   assets/
 *     shaders/
 *       src/          -> contient les fichiers HLSL sources   (*.vertex.hlsl, *.fragment.hlsl)
 *       compiled/     -> contient les binaires précompilés    (*.spv, *.msl, *.metallib, *.dxil)
 *       reflection/   -> contient les fichiers JSON de réflexion (*.json)
 * ```
 * 
 * \param {const char*} storage_path - Nom logique du shader à charger avec suffixe de stage
 *                                    (par ex. "assets/water.vertex" ou "assets/water.fragment").
 * \param {RC2D_StorageKind} storage_kind - Type de storage (TITLE ou USER).
 * \return {RC2D_GPUShader*} Pointeur vers le shader chargé, ou NULL en cas d'erreur.
 * 
 * \warning Le pointeur retourné doit être libéré par l'appelant avec SDL_ReleaseGPUShader
 *          lorsque le shader n'est plus nécessaire.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
RC2D_GPUShader* rc2d_gpu_loadGraphicsShaderFromStorage(const char* storage_path,
                                                       RC2D_StorageKind storage_kind)
{
    /**
     * Vérification des paramètres d'entrée
     */
    RC2D_assert_release(storage_path != NULL && *storage_path != '\0',
                        RC2D_LOG_CRITICAL,
                        "rc2d_gpu_loadGraphicsShaderFromStorage: storage_path is NULL or empty");

    // Vérifier que le stockage est prêt
    if (storage_kind == RC2D_STORAGE_TITLE && !rc2d_storage_titleReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Title storage not ready when loading '%s'", storage_path);
        return NULL;
    }
    else if (storage_kind == RC2D_STORAGE_USER && !rc2d_storage_userReady()) 
    {
        RC2D_log(RC2D_LOG_ERROR, "User storage not ready when loading '%s'", storage_path);
        return NULL;
    }

    /**
     * Locker le mutex pour éviter les accès concurrents au cache des shaders graphiques
     */
    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    /**
     * Vérifier si le shader graphique est déjà dans le cache (donc déjà chargé une fois)
     */
    for (int i = 0; i < rc2d_engine_state.gpu_graphics_shader_count; i++) 
    {
        if (SDL_strcmp(rc2d_engine_state.gpu_graphics_shaders_cache[i].filename, storage_path) == 0) 
        {
            RC2D_GPUShader* graphicsShader = rc2d_engine_state.gpu_graphics_shaders_cache[i].shader;
            SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

            RC2D_log(RC2D_LOG_INFO, "Graphics Shader already loaded from cache: %s", storage_path);
            return graphicsShader;
        }
    }

    /**
     * On unlock le mutex après avoir vérifié le cache des shaders graphiques.
     * Cela permet aux autres threads d'accéder au cache des shaders graphiques.
     */
    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    /**
     * Déterminer le stage en fonction du suffixe (vertex ou fragment)
     * On utilise uniquement le basename après le dernier '/' ou '\'
     * et on vérifie la présence de ".vertex" ou ".fragment" dans le nom du fichier
     */
    SDL_GPUShaderStage stage;
    {
        const char* base = storage_path;
        const char* s1 = SDL_strrchr(storage_path, '/');
        const char* s2 = SDL_strrchr(storage_path, '\\');
        if (s1 || s2) base = (s1 > s2 ? s1 : s2) + 1;

        if (SDL_strstr(base, ".vertex")) 
        {
            stage = SDL_GPU_SHADERSTAGE_VERTEX;
        } 
        else if (SDL_strstr(base, ".fragment")) 
        {
            stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        } 
        else 
        {
            RC2D_log(RC2D_LOG_CRITICAL, "Unknown shader stage suffix: expected .vertex or .fragment (in '%s').", base);
            return NULL;
        }
    }

    /**
     * fullPath : Chemin d'accès "logique storage" au fichier binaire du shader compilé
     * ou au fichier HLSL source. Ici on construit des chemins RELATIFS AU STORAGE,
     * pas au filesystem de l'OS (plus de SDL_GetBasePath()).
     *
     * Exemple : storage_path = "assets/water.fragment"
     *   -> offline:   "assets/shaders/compiled/spirv/water.fragment.spv"
     *   -> reflection:"assets/shaders/reflection/water.fragment.json"
     *   -> hot reload:"assets/shaders/src/water.fragment.hlsl"
     */
    char dirbuf[256], root_shaders[320], fullPath[512];

    // 1) Basename
    const char* base = storage_path;
    const char* s1 = SDL_strrchr(storage_path, '/');
    const char* s2 = SDL_strrchr(storage_path, '\\');
    if (s1 || s2) base = (s1 > s2 ? s1 : s2) + 1;

    // 2) Dirname (sans le basename)
    if (base != storage_path) 
    {
        size_t dirlen = (size_t)(base - storage_path); // inclut le slash juste avant base
        if (dirlen >= sizeof(dirbuf)) dirlen = sizeof(dirbuf) - 1;
        SDL_memcpy(dirbuf, storage_path, dirlen);
        dirbuf[dirlen] = '\0';
    } 
    else 
    {
        dirbuf[0] = '\0';
    }

    // 3) Normaliser les séparateurs (optionnel mais pratique pour les checks)
    for (char* p = dirbuf; *p; ++p) 
    {
        if (*p == '\\') *p = '/';
    }

    // 4) Trim des slashs finaux pour éviter "xxx//shaders"
    {
        size_t n = SDL_strlen(dirbuf);
        while (n > 0 && dirbuf[n-1] == '/') 
        {
            dirbuf[--n] = '\0';
        }
    }

    // 5) Construire la racine des shaders :
    //    - si dir contient déjà "shaders" comme SEGMENT final → on garde tel quel
    //    - sinon → on ajoute "/shaders"
    if (dirbuf[0] == '\0') 
    {
        SDL_strlcpy(root_shaders, "shaders", sizeof root_shaders);
    } 
    else 
    {
        bool ends_with_shaders = false;
        size_t dlen = SDL_strlen(dirbuf);
        if (dlen >= 7) 
        {
            // match fin de chaîne "/shaders" ou "shaders"
            const char* tail = dirbuf + (dlen - 7);
            if (SDL_strcmp(tail, "shaders") == 0) 
            {
                ends_with_shaders = true;
            } 
            else if (dlen >= 9 && SDL_strcmp(dirbuf + (dlen - 8), "/shaders") == 0) 
            {
                ends_with_shaders = true;
            }
        }

        if (ends_with_shaders) {
            SDL_strlcpy(root_shaders, dirbuf, sizeof root_shaders);
        } 
        else 
        {
            // jointure sûre (évite les doubles '/')
            if (dlen + 1 + 7 + 1 < sizeof root_shaders) 
            {
                SDL_strlcpy(root_shaders, dirbuf, sizeof root_shaders);
                SDL_strlcat(root_shaders, "/shaders", sizeof root_shaders);
            } 
            else 
            {
                RC2D_log(RC2D_LOG_ERROR, "Path too long when building shaders root from '%s'", dirbuf);
                return NULL;
            }
        }
    }

#if !RC2D_GPU_SHADER_HOT_RELOAD_ENABLED // Compilation hors ligne des shaders graphics en HLSL
    /**
     * entrypoint : Point d'entrée du shader (main pour SPIR-V, DXIL et main0 pour MSL).
     */
    const char* entrypoint = NULL;

    // Récupérer les formats supportés par le backend actuel
    SDL_GPUShaderFormat backendFormatsSupported = SDL_GetGPUShaderFormats(rc2d_gpu_getDevice());

    // Le format de shader à utiliser pour la compilation hors ligne
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;

    // Générer le chemin d'accès "storage" au fichier binaire du shader compilé en fonction du backend
    if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_SPIRV) 
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/spirv/%s.spv", root_shaders, base);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    }
    /**
     * Priorisé dans l'ordre des conditions le format : METALLIB avant MSL pour les appareils Apple.
     * METALLIB est le format précompilé pour Metal, tandis que MSL pour Metal est compilé à l'exécution.
     */
    else if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_METALLIB)
    {
    #ifdef RC2D_PLATFORM_IOS
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/metallib/ios/%s.metallib", root_shaders, base);
    #else
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/metallib/macos/%s.metallib", root_shaders, base);
    #endif
        format = SDL_GPU_SHADERFORMAT_METALLIB;
        entrypoint = "main";
    }
    else if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_MSL)
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/msl/%s.msl", root_shaders, base);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0"; // SDL_shadercross requiert "main0" pour MSL
    } 
    else if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_DXIL) 
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/dxil/%s.dxil", root_shaders, base);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    } 
    else 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "No compatible shader format for this backend");
        return NULL;
    }

    /**
     * Charger le fichier du shader compilé
     * On utilise l’API Storage (Title/User) pour lire le binaire en mémoire
     */
    void* codeShaderCompiled = NULL;
    Uint64 codeShaderCompiledSize = 0;
    if (!((storage_kind == RC2D_STORAGE_TITLE)
            ? rc2d_storage_titleReadFile(fullPath, &codeShaderCompiled, &codeShaderCompiledSize)
            : rc2d_storage_userReadFile (fullPath, &codeShaderCompiled, &codeShaderCompiledSize))
        || !codeShaderCompiled || codeShaderCompiledSize == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load compiled shader from storage: %s", fullPath);
        return NULL;
    }

    /**
     * En mode compilation hors ligne des shaders, on utilise un fichier JSON généré par le script de compilation des shaders.
     * On génère le chemin d'accès au fichier JSON de réflexion en fonction du nom du shader et de son stage.
     */
    char jsonPath[512];
    SDL_snprintf(jsonPath, sizeof(jsonPath), "%s/reflection/%s.json", root_shaders, base);

    /**
     * On ouvre le fichier JSON de réflexion pour récupérer les informations de réflexions sur le shader.
     * On utilise l’API Storage pour charger le fichier JSON de réflexion.
     * On utilise SDL_sscanf pour extraire les valeurs des champs : samplers, uniform_buffers, storage_buffers et storage_textures.
     * On libère le contenu JSON après la lecture.
     * 
     * Exemple de contenu JSON attendu :
     * ```json
     * {
     *   "samplers": 0,
     *   "uniform_buffers": 0,
     *   "storage_buffers": 0,
     *   "storage_textures": 0
     * }
     * ```
     */
    Uint32 numSamplers = 0;
    Uint32 numUniformBuffers = 0;
    Uint32 numStorageBuffers = 0;
    Uint32 numStorageTextures = 0;
    void* jsonContent = NULL;
    Uint64 jsonLen = 0;
    if (((storage_kind == RC2D_STORAGE_TITLE)
            ? rc2d_storage_titleReadFile(jsonPath, &jsonContent, &jsonLen)
            : rc2d_storage_userReadFile (jsonPath, &jsonContent, &jsonLen))
        && jsonContent && jsonLen > 0)
    {
        // Cast le contenu du fichier JSON en chaîne de caractères (NUL-terminée)
        char* content = (char*)RC2D_malloc(jsonLen+1);
        SDL_memcpy(content, jsonContent, jsonLen);
        content[jsonLen] = '\0';

        json_read_uint(content, "\"samplers\"",          &numSamplers);
        json_read_uint(content, "\"uniform_buffers\"",   &numUniformBuffers);
        json_read_uint(content, "\"storage_buffers\"",   &numStorageBuffers);
        json_read_uint(content, "\"storage_textures\"",  &numStorageTextures);

        // Libérer le contenu JSON après la lecture
        RC2D_safe_free(content);
        RC2D_safe_free(jsonContent);
    }
    else 
    {
        RC2D_log(RC2D_LOG_WARN, "Shader reflection file not found in storage: %s", jsonPath);
         if (jsonContent) RC2D_safe_free(jsonContent);
    }

    // Création du shader GPU avec les informations de réflexion récupérées depuis le fichier JSON
    SDL_GPUShaderCreateInfo info = {
        .code = codeShaderCompiled,
        .code_size = (size_t)codeShaderCompiledSize,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = numSamplers,
        .num_uniform_buffers = numUniformBuffers,
        .num_storage_buffers = numStorageBuffers,
        .num_storage_textures = numStorageTextures,
        .props = 0
    };

    // Créer le shader graphique à partir du code compilé du shader
    SDL_GPUShader* graphicsShader = SDL_CreateGPUShader(rc2d_gpu_getDevice(), &info);

    // Libérer le code du shader compilé après la création du shader
    RC2D_safe_free(codeShaderCompiled);

    // Vérifier si la création du shader graphique a réussi
    if (graphicsShader == NULL) 
    {
        // Si la création du shader graphique échoue, on log l'erreur et on retourne NULL
        RC2D_log(RC2D_LOG_ERROR, "Failed to create GPU graphics shader from storage file %s, SDL_Error: %s", fullPath, SDL_GetError());
        return NULL;
    }
#else
    /**
     * On génère le chemin d'accès au fichier HLSL source en fonction du nom du shader et de son stage pour la compilation en ligne des shaders.
     * On utilise SDL_snprintf pour formater le chemin d'accès "storage" au fichier HLSL source.
     */
    SDL_snprintf(fullPath, sizeof(fullPath), "%s/src/%s.hlsl", root_shaders, base);

    /**
     * Charger le fichier HLSL source
     * On utilise l’API Storage pour charger le fichier HLSL source.
     */
    void* codeHLSLSourceBytes = NULL;
    Uint64 codeHLSLSourceLen = 0;
    if (!((storage_kind == RC2D_STORAGE_TITLE)
            ? rc2d_storage_titleReadFile(fullPath, &codeHLSLSourceBytes, &codeHLSLSourceLen)
            : rc2d_storage_userReadFile (fullPath, &codeHLSLSourceBytes, &codeHLSLSourceLen))
        || !codeHLSLSourceBytes || codeHLSLSourceLen == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load HLSL shader source from storage: %s", fullPath); 
        return NULL;
    }

    // Préparer les informations pour la compilation HLSL vers SPIR-V
    // (on s’assure d’une chaîne NUL-terminée)
    char* codeHLSLSource = (char*)RC2D_malloc((size_t)codeHLSLSourceLen + 1);
    SDL_memcpy(codeHLSLSource, codeHLSLSourceBytes, (size_t)codeHLSLSourceLen);
    codeHLSLSource[codeHLSLSourceLen] = '\0';
    RC2D_safe_free(codeHLSLSourceBytes);

    SDL_ShaderCross_HLSL_Info hlslInfo = {
        .source = codeHLSLSource,
        .entrypoint = "main",
        .include_dir = NULL,
        .defines = NULL,
        .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
        .enable_debug = true,
        .name = storage_path,
        .props = 0
    };

    // Compiler HLSL vers SPIR-V
    size_t spirvByteCodeSize = 0;
    void* spirvByteCode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlslInfo, &spirvByteCodeSize);

    // Libérer le code HLSL source après la compilation
    RC2D_safe_free(codeHLSLSource);

    // Vérifier si la compilation HLSL vers SPIR-V a réussi
    if (spirvByteCode == NULL || spirvByteCodeSize == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to compile HLSL to SPIR-V: %s", storage_path);
        return NULL;
    }

    // Réfléchir les métadonnées du shader graphique
    SDL_ShaderCross_GraphicsShaderMetadata* metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(
        spirvByteCode, spirvByteCodeSize, 0
    );
    if (metadata == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to reflect graphics shader metadata: %s", storage_path);
        RC2D_safe_free(spirvByteCode);
        return NULL;
    }

    // Préparer les informations SPIR-V pour la compilation du shader
    SDL_ShaderCross_SPIRV_Info spirvInfo = {
        .bytecode      = spirvByteCode,
        .bytecode_size = spirvByteCodeSize,
        .entrypoint    = "main",
        .shader_stage  = (SDL_ShaderCross_ShaderStage)stage,
        .enable_debug  = true,
        .name          = storage_path,
        .props         = 0
    };

    // Compiler le shader graphique
    SDL_GPUShader* graphicsShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        rc2d_gpu_getDevice(),
        &spirvInfo,
        metadata,
        0
    );

    // Libérer les ressources allouées pour les métadonnées et le code SPIR-V
    RC2D_safe_free(metadata);
    RC2D_safe_free(spirvByteCode);

    // Vérifier si la compilation du shader graphique a réussi
    if (graphicsShader == NULL) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to create GPU graphics shader from SPIR-V: %s", storage_path);
        return NULL;
    }

    /**
     * On lock le temps d'ajouter le shader graphique au cache des shaders graphiques,
     * et pour éviter les accès concurrents.
     * 
     * On utilise un mutex pour protéger l'accès au cache des shaders graphiques.
     */
    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    // On réalloue le cache des shaders graphiques pour ajouter le nouveau shader graphique (on dois augmenter la taille du cache)
    RC2D_GraphicsShaderEntry* newShaders = RC2D_realloc(
        rc2d_engine_state.gpu_graphics_shaders_cache,
        (rc2d_engine_state.gpu_graphics_shader_count + 1) * sizeof(RC2D_GraphicsShaderEntry)
    );

    // Vérifier si la réallocation a réussi
    RC2D_assert_release(newShaders != NULL, RC2D_LOG_CRITICAL, "Failed to realloc shader cache");

    // Mettre à jour le cache des shaders graphiques avec le nouveau shader graphique
    rc2d_engine_state.gpu_graphics_shaders_cache = newShaders;
    RC2D_GraphicsShaderEntry* entry = &rc2d_engine_state.gpu_graphics_shaders_cache[rc2d_engine_state.gpu_graphics_shader_count++];
    entry->filename = RC2D_strdup(storage_path);
    entry->shader = graphicsShader;
    entry->lastModified = 0; // Pas de mtime via Storage : laisser 0 ou TODO si tu exposes une API.

    /**
     * On unlock le mutex après avoir ajouté le shader graphique au cache.
     * Cela permet aux autres threads d'accéder au cache des shaders graphiques.
     */
    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
#endif // RC2D_GPU_SHADER_HOT_RELOAD_ENABLED

#if !RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
    // Branche "offline" : on n’a pas encore inséré dans le cache, on le fait maintenant.

    /**
     * On lock le temps d'ajouter le shader graphique au cache des shaders graphiques,
     * et pour éviter les accès concurrents.
     */
    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    // On réalloue le cache des shaders graphiques pour ajouter le nouveau shader graphique (on dois augmenter la taille du cache)
    RC2D_GraphicsShaderEntry* newShaders2 = RC2D_realloc(
        rc2d_engine_state.gpu_graphics_shaders_cache,
        (rc2d_engine_state.gpu_graphics_shader_count + 1) * sizeof(RC2D_GraphicsShaderEntry)
    );

    // Vérifier si la réallocation a réussi
    RC2D_assert_release(newShaders2 != NULL, RC2D_LOG_CRITICAL, "Failed to realloc shader cache");

    // Mettre à jour le cache des shaders graphiques avec le nouveau shader graphique
    rc2d_engine_state.gpu_graphics_shaders_cache = newShaders2;
    RC2D_GraphicsShaderEntry* entry2 = &rc2d_engine_state.gpu_graphics_shaders_cache[rc2d_engine_state.gpu_graphics_shader_count++];
    entry2->filename = RC2D_strdup(storage_path);
    entry2->shader = graphicsShader;
    entry2->lastModified = 0; // TODO: si tu ajoutes rc2d_storage_getFileMTime(), renseigne-le ici.

    /**
     * On unlock le mutex après avoir ajouté le shader graphique au cache.
     * Cela permet aux autres threads d'accéder au cache des shaders graphiques.
     */
    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
#endif // !RC2D_GPU_SHADER_HOT_RELOAD_ENABLED

    RC2D_log(RC2D_LOG_INFO, "Graphics Shader loaded and cached from storage: %s", storage_path);
    return graphicsShader;
}