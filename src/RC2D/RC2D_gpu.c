#include <RC2D/RC2D_gpu.h>

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>

#include <RC2D/RC2D_assert.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_platform_defines.h>
#include <RC2D/RC2D_memory.h>

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

static SDL_Time rc2d_gpu_getStorageFileModificationTime(const char* storage_path, RC2D_StorageKind storage_kind)
{
    if (storage_path == NULL || *storage_path == '\0')
    {
        return 0;
    }

    if (storage_kind == RC2D_STORAGE_TITLE)
    {
        const char* basePath = SDL_GetBasePath();
        if (basePath != NULL)
        {
            char absolutePath[1024];
            SDL_snprintf(absolutePath, sizeof(absolutePath), "%s%s", basePath, storage_path);
            return rc2d_gpu_getFileModificationTime(absolutePath);
        }
    }
    else if (storage_kind == RC2D_STORAGE_USER &&
             rc2d_engine_state.config != NULL &&
             rc2d_engine_state.config->appInfo != NULL)
    {
        const char* org = rc2d_engine_state.config->appInfo->organization;
        const char* app = rc2d_engine_state.config->appInfo->name;
        if (org != NULL && app != NULL)
        {
            char* prefPath = SDL_GetPrefPath(org, app);
            if (prefPath != NULL)
            {
                char absolutePath[1024];
                SDL_snprintf(absolutePath, sizeof(absolutePath), "%s%s", prefPath, storage_path);
                SDL_free(prefPath);
                return rc2d_gpu_getFileModificationTime(absolutePath);
            }
        }
    }

    // Fallback: chemin relatif au CWD (utile en développement local).
    return rc2d_gpu_getFileModificationTime(storage_path);
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
     *   offline:   
     *   -> binaire compilé:"assets/shaders/compiled/spirv/water.fragment.spv" (ou .metallib, .msl, .dxil selon le backend)
     *   -> reflection:"assets/shaders/reflection/water.fragment.json"
     *
     *   online:
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
    #if defined(RC2D_PLATFORM_IOS)
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/metallib/ios/%s.metallib", root_shaders, base);
    #elif defined(RC2D_PLATFORM_MACOS)
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
    // D3D (renderer GPU SDL): les shaders internes sont DXIL60.
    // On priorise donc DXIL pour eviter un mix de shader model dans un meme PSO.
    else if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_DXIL) 
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/dxil/%s.dxil", root_shaders, base);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    }
    else if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_DXBC)
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%s/compiled/dxbc/%s.dxbc", root_shaders, base);
        format = SDL_GPU_SHADERFORMAT_DXBC;
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

    RC2D_log(RC2D_LOG_INFO, "Loading graphics shader from storage: %s (format=%d, samplers=%u, uniform_buffers=%u, storage_buffers=%u, storage_textures=%u)",
             fullPath, format, numSamplers, numUniformBuffers, numStorageBuffers, numStorageTextures);

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

    SDL_PropertiesID shaderProps = SDL_CreateProperties();
    SDL_SetBooleanProperty(shaderProps, SDL_SHADERCROSS_PROP_SHADER_DEBUG_ENABLE_BOOLEAN, true);
    SDL_SetStringProperty(shaderProps, SDL_SHADERCROSS_PROP_SHADER_DEBUG_NAME_STRING, storage_path);
    SDL_SetBooleanProperty(shaderProps, SDL_SHADERCROSS_PROP_SHADER_CULL_UNUSED_BINDINGS_BOOLEAN, true);
#if defined(RC2D_PLATFORM_APPLE)
    SDL_SetStringProperty(shaderProps, SDL_SHADERCROSS_PROP_SPIRV_MSL_VERSION_STRING, "3.2.0");
#endif

    SDL_ShaderCross_HLSL_Info hlslInfo = {
        .source = codeHLSLSource,
        .entrypoint = "main",
        .include_dir = NULL,
        .defines = NULL,
        .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
        .props = shaderProps
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
        (const Uint8*)spirvByteCode, 
        spirvByteCodeSize,
        shaderProps
    );
    if (metadata == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to reflect graphics shader metadata: %s", storage_path);
        RC2D_safe_free(spirvByteCode);
        return NULL;
    }

    // Préparer les informations SPIR-V pour la compilation du shader
    SDL_ShaderCross_SPIRV_Info spirvInfo = {
        .bytecode      = (const Uint8*)spirvByteCode,
        .bytecode_size = spirvByteCodeSize,
        .entrypoint    = "main",
        .shader_stage  = (SDL_ShaderCross_ShaderStage)stage,
        .props         = shaderProps
    };

    // Compiler le shader graphique.
    // IMPORTANT (Windows / D3D12 + SDL GPU Renderer):
    // les shaders internes du renderer SDL sont DXIL60, donc on priorise DXIL ici.
    SDL_GPUShader* graphicsShader = NULL;

    SDL_GPUShaderFormat backendFormatsSupportedHotReload = SDL_GetGPUShaderFormats(rc2d_gpu_getDevice());
    const bool hasDxbcHotReload = (backendFormatsSupportedHotReload & SDL_GPU_SHADERFORMAT_DXBC) != 0;
    const bool hasDxilHotReload = (backendFormatsSupportedHotReload & SDL_GPU_SHADERFORMAT_DXIL) != 0;
    const bool d3dBackendHotReload = (backendFormatsSupportedHotReload & SDL_GPU_SHADERFORMAT_DXBC) ||
                                     (backendFormatsSupportedHotReload & SDL_GPU_SHADERFORMAT_DXIL);

    if (hasDxilHotReload)
    {
        size_t dxilByteCodeSize = 0;
        void* dxilByteCode = SDL_ShaderCross_CompileDXILFromSPIRV(&spirvInfo, &dxilByteCodeSize);
        if (dxilByteCode != NULL && dxilByteCodeSize > 0)
        {
            SDL_GPUShaderCreateInfo dxilShaderInfo = {
                .code = dxilByteCode,
                .code_size = dxilByteCodeSize,
                .entrypoint = "main",
                .format = SDL_GPU_SHADERFORMAT_DXIL,
                .stage = stage,
                .num_samplers = metadata->resource_info.num_samplers,
                .num_uniform_buffers = metadata->resource_info.num_uniform_buffers,
                .num_storage_buffers = metadata->resource_info.num_storage_buffers,
                .num_storage_textures = metadata->resource_info.num_storage_textures,
                .props = 0
            };

            graphicsShader = SDL_CreateGPUShader(rc2d_gpu_getDevice(), &dxilShaderInfo);
            if (graphicsShader != NULL)
            {
                RC2D_log(RC2D_LOG_INFO, "Hot reload graphics shader created as DXIL: %s", storage_path);
            }
        }
        RC2D_safe_free(dxilByteCode);
    }

    // Fallback DXBC uniquement si DXIL n'est pas supporte par le device.
    if (graphicsShader == NULL && !hasDxilHotReload && hasDxbcHotReload)
    {
        size_t dxbcByteCodeSize = 0;
        void* dxbcByteCode = SDL_ShaderCross_CompileDXBCFromSPIRV(&spirvInfo, &dxbcByteCodeSize);
        if (dxbcByteCode != NULL && dxbcByteCodeSize > 0)
        {
            SDL_GPUShaderCreateInfo dxbcShaderInfo = {
                .code = dxbcByteCode,
                .code_size = dxbcByteCodeSize,
                .entrypoint = "main",
                .format = SDL_GPU_SHADERFORMAT_DXBC,
                .stage = stage,
                .num_samplers = metadata->resource_info.num_samplers,
                .num_uniform_buffers = metadata->resource_info.num_uniform_buffers,
                .num_storage_buffers = metadata->resource_info.num_storage_buffers,
                .num_storage_textures = metadata->resource_info.num_storage_textures,
                .props = 0
            };

            graphicsShader = SDL_CreateGPUShader(rc2d_gpu_getDevice(), &dxbcShaderInfo);
            if (graphicsShader != NULL)
            {
                RC2D_log(RC2D_LOG_INFO, "Hot reload graphics shader created as DXBC: %s", storage_path);
            }
        }
        RC2D_safe_free(dxbcByteCode);
    }

    // Backends non-D3D: conserver le chemin cross-platform habituel.
    if (graphicsShader == NULL && !d3dBackendHotReload)
    {
        graphicsShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
            rc2d_gpu_getDevice(),
            &spirvInfo,
            &metadata->resource_info,
            shaderProps
        );
    }

    // Libérer les ressources allouées pour les métadonnées et le code SPIR-V
    RC2D_safe_free(metadata);
    RC2D_safe_free(spirvByteCode);
    SDL_DestroyProperties(shaderProps);

    // Vérifier si la compilation du shader graphique a réussi
    if (graphicsShader == NULL) 
    {
        if (d3dBackendHotReload)
        {
            RC2D_log(RC2D_LOG_ERROR, "D3D hot reload shader creation failed for both DXIL and DXBC: %s", storage_path);
        }
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
    entry->gpu_render_state = NULL;
    entry->storage_kind = storage_kind;
    entry->gpu_render_states = NULL;
    entry->gpu_render_state_count = 0;
    entry->lastModified = rc2d_gpu_getStorageFileModificationTime(fullPath, storage_kind);

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
    entry2->gpu_render_state = NULL;
    entry2->storage_kind = storage_kind;
    entry2->gpu_render_states = NULL;
    entry2->gpu_render_state_count = 0;
    entry2->lastModified = rc2d_gpu_getStorageFileModificationTime(fullPath, storage_kind);

    /**
     * On unlock le mutex après avoir ajouté le shader graphique au cache.
     * Cela permet aux autres threads d'accéder au cache des shaders graphiques.
     */
    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
#endif // !RC2D_GPU_SHADER_HOT_RELOAD_ENABLED

    RC2D_log(RC2D_LOG_INFO, "Graphics Shader loaded and cached from storage: %s", storage_path);
    return graphicsShader;
}

#if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
static bool rc2d_gpu_extractStageAndPaths(const char* storage_path,
                                          SDL_GPUShaderStage* outStage,
                                          char* outRootShaders,
                                          size_t outRootShadersSize,
                                          char* outBase,
                                          size_t outBaseSize,
                                          char* outSourcePath,
                                          size_t outSourcePathSize)
{
    RC2D_assert_release(storage_path != NULL && *storage_path != '\0',
                        RC2D_LOG_CRITICAL,
                        "rc2d_gpu_extractStageAndPaths: storage_path is NULL or empty");

    const char* base = storage_path;
    const char* s1 = SDL_strrchr(storage_path, '/');
    const char* s2 = SDL_strrchr(storage_path, '\\');
    if (s1 || s2)
    {
        base = (s1 > s2 ? s1 : s2) + 1;
    }

    if (SDL_strstr(base, ".vertex"))
    {
        *outStage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (SDL_strstr(base, ".fragment"))
    {
        *outStage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        RC2D_log(RC2D_LOG_ERROR, "Unknown shader stage suffix in '%s'", storage_path);
        return false;
    }

    SDL_strlcpy(outBase, base, outBaseSize);

    char dirbuf[256];
    if (base != storage_path)
    {
        size_t dirlen = (size_t)(base - storage_path);
        if (dirlen >= sizeof(dirbuf))
        {
            dirlen = sizeof(dirbuf) - 1;
        }
        SDL_memcpy(dirbuf, storage_path, dirlen);
        dirbuf[dirlen] = '\0';
    }
    else
    {
        dirbuf[0] = '\0';
    }

    for (char* p = dirbuf; *p; ++p)
    {
        if (*p == '\\')
        {
            *p = '/';
        }
    }

    size_t n = SDL_strlen(dirbuf);
    while (n > 0 && dirbuf[n - 1] == '/')
    {
        dirbuf[--n] = '\0';
    }

    if (dirbuf[0] == '\0')
    {
        SDL_strlcpy(outRootShaders, "shaders", outRootShadersSize);
    }
    else
    {
        bool endsWithShaders = false;
        size_t dlen = SDL_strlen(dirbuf);
        if (dlen >= 7)
        {
            const char* tail = dirbuf + (dlen - 7);
            if (SDL_strcmp(tail, "shaders") == 0)
            {
                endsWithShaders = true;
            }
            else if (dlen >= 9 && SDL_strcmp(dirbuf + (dlen - 8), "/shaders") == 0)
            {
                endsWithShaders = true;
            }
        }

        if (endsWithShaders)
        {
            SDL_strlcpy(outRootShaders, dirbuf, outRootShadersSize);
        }
        else
        {
            if (dlen + 1 + 7 + 1 >= outRootShadersSize)
            {
                RC2D_log(RC2D_LOG_ERROR, "Path too long when building shaders root from '%s'", dirbuf);
                return false;
            }
            SDL_strlcpy(outRootShaders, dirbuf, outRootShadersSize);
            SDL_strlcat(outRootShaders, "/shaders", outRootShadersSize);
        }
    }

    SDL_snprintf(outSourcePath, outSourcePathSize, "%s/src/%s.hlsl", outRootShaders, outBase);
    return true;
}

static RC2D_GPUShader* rc2d_gpu_compileGraphicsShaderHotReload(const char* storage_path,
                                                                RC2D_StorageKind storage_kind,
                                                                SDL_GPUShaderStage stage,
                                                                const char* sourcePath)
{
    void* codeHLSLSourceBytes = NULL;
    Uint64 codeHLSLSourceLen = 0;
    if (!((storage_kind == RC2D_STORAGE_TITLE)
            ? rc2d_storage_titleReadFile(sourcePath, &codeHLSLSourceBytes, &codeHLSLSourceLen)
            : rc2d_storage_userReadFile(sourcePath, &codeHLSLSourceBytes, &codeHLSLSourceLen))
        || !codeHLSLSourceBytes || codeHLSLSourceLen == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "Hot reload: failed to load HLSL source from storage: %s", sourcePath);
        return NULL;
    }

    char* codeHLSLSource = (char*)RC2D_malloc((size_t)codeHLSLSourceLen + 1);
    SDL_memcpy(codeHLSLSource, codeHLSLSourceBytes, (size_t)codeHLSLSourceLen);
    codeHLSLSource[codeHLSLSourceLen] = '\0';
    RC2D_safe_free(codeHLSLSourceBytes);

    SDL_PropertiesID shaderProps = SDL_CreateProperties();
    SDL_SetBooleanProperty(shaderProps, SDL_SHADERCROSS_PROP_SHADER_DEBUG_ENABLE_BOOLEAN, true);
    SDL_SetStringProperty(shaderProps, SDL_SHADERCROSS_PROP_SHADER_DEBUG_NAME_STRING, storage_path);
    SDL_SetBooleanProperty(shaderProps, SDL_SHADERCROSS_PROP_SHADER_CULL_UNUSED_BINDINGS_BOOLEAN, true);
#if defined(RC2D_PLATFORM_APPLE)
    SDL_SetStringProperty(shaderProps, SDL_SHADERCROSS_PROP_SPIRV_MSL_VERSION_STRING, "3.2.0");
#endif

    SDL_ShaderCross_HLSL_Info hlslInfo = {
        .source = codeHLSLSource,
        .entrypoint = "main",
        .include_dir = NULL,
        .defines = NULL,
        .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
        .props = shaderProps
    };

    size_t spirvByteCodeSize = 0;
    void* spirvByteCode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlslInfo, &spirvByteCodeSize);
    RC2D_safe_free(codeHLSLSource);

    if (spirvByteCode == NULL || spirvByteCodeSize == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "Hot reload: failed to compile HLSL to SPIR-V: %s", storage_path);
        SDL_DestroyProperties(shaderProps);
        return NULL;
    }

    SDL_ShaderCross_GraphicsShaderMetadata* metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(
        (const Uint8*)spirvByteCode,
        spirvByteCodeSize,
        shaderProps
    );
    if (metadata == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Hot reload: failed to reflect shader metadata: %s", storage_path);
        RC2D_safe_free(spirvByteCode);
        SDL_DestroyProperties(shaderProps);
        return NULL;
    }

    SDL_ShaderCross_SPIRV_Info spirvInfo = {
        .bytecode = (const Uint8*)spirvByteCode,
        .bytecode_size = spirvByteCodeSize,
        .entrypoint = "main",
        .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
        .props = shaderProps
    };

    SDL_GPUShader* graphicsShader = NULL;
    SDL_GPUShaderFormat backendFormatsSupported = SDL_GetGPUShaderFormats(rc2d_gpu_getDevice());
    const bool hasDxbc = (backendFormatsSupported & SDL_GPU_SHADERFORMAT_DXBC) != 0;
    const bool hasDxil = (backendFormatsSupported & SDL_GPU_SHADERFORMAT_DXIL) != 0;
    const bool d3dBackend = hasDxbc || hasDxil;

    if (hasDxil)
    {
        size_t dxilByteCodeSize = 0;
        void* dxilByteCode = SDL_ShaderCross_CompileDXILFromSPIRV(&spirvInfo, &dxilByteCodeSize);
        if (dxilByteCode != NULL && dxilByteCodeSize > 0)
        {
            SDL_GPUShaderCreateInfo info = {
                .code = dxilByteCode,
                .code_size = dxilByteCodeSize,
                .entrypoint = "main",
                .format = SDL_GPU_SHADERFORMAT_DXIL,
                .stage = stage,
                .num_samplers = metadata->resource_info.num_samplers,
                .num_uniform_buffers = metadata->resource_info.num_uniform_buffers,
                .num_storage_buffers = metadata->resource_info.num_storage_buffers,
                .num_storage_textures = metadata->resource_info.num_storage_textures,
                .props = 0
            };
            graphicsShader = SDL_CreateGPUShader(rc2d_gpu_getDevice(), &info);
        }
        RC2D_safe_free(dxilByteCode);
    }

    if (graphicsShader == NULL && !hasDxil && hasDxbc)
    {
        size_t dxbcByteCodeSize = 0;
        void* dxbcByteCode = SDL_ShaderCross_CompileDXBCFromSPIRV(&spirvInfo, &dxbcByteCodeSize);
        if (dxbcByteCode != NULL && dxbcByteCodeSize > 0)
        {
            SDL_GPUShaderCreateInfo info = {
                .code = dxbcByteCode,
                .code_size = dxbcByteCodeSize,
                .entrypoint = "main",
                .format = SDL_GPU_SHADERFORMAT_DXBC,
                .stage = stage,
                .num_samplers = metadata->resource_info.num_samplers,
                .num_uniform_buffers = metadata->resource_info.num_uniform_buffers,
                .num_storage_buffers = metadata->resource_info.num_storage_buffers,
                .num_storage_textures = metadata->resource_info.num_storage_textures,
                .props = 0
            };
            graphicsShader = SDL_CreateGPUShader(rc2d_gpu_getDevice(), &info);
        }
        RC2D_safe_free(dxbcByteCode);
    }

    if (graphicsShader == NULL && !d3dBackend)
    {
        graphicsShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
            rc2d_gpu_getDevice(),
            &spirvInfo,
            &metadata->resource_info,
            shaderProps
        );
    }

    RC2D_safe_free(metadata);
    RC2D_safe_free(spirvByteCode);
    SDL_DestroyProperties(shaderProps);

    return graphicsShader;
}

bool rc2d_gpu_trackGraphicsRenderState(const char* shader_storage_path,
                                       SDL_GPURenderState** state_handle,
                                       int num_sampler_bindings,
                                       const SDL_GPUTextureSamplerBinding* sampler_bindings)
{
    if (shader_storage_path == NULL || *shader_storage_path == '\0' || state_handle == NULL || *state_handle == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "trackGraphicsRenderState: invalid parameters");
        return false;
    }

    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    RC2D_GraphicsShaderEntry* shaderEntry = NULL;
    for (int i = 0; i < rc2d_engine_state.gpu_graphics_shader_count; ++i)
    {
        if (SDL_strcmp(rc2d_engine_state.gpu_graphics_shaders_cache[i].filename, shader_storage_path) == 0)
        {
            shaderEntry = &rc2d_engine_state.gpu_graphics_shaders_cache[i];
            break;
        }
    }

    if (shaderEntry == NULL)
    {
        SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
        RC2D_log(RC2D_LOG_ERROR, "trackGraphicsRenderState: shader not found in cache: %s", shader_storage_path);
        return false;
    }

    for (int i = 0; i < shaderEntry->gpu_render_state_count; ++i)
    {
        if (shaderEntry->gpu_render_states[i].state_handle == state_handle)
        {
            SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
            return true;
        }
    }

    RC2D_GPURenderStateBindingEntry* newEntries = (RC2D_GPURenderStateBindingEntry*)RC2D_realloc(
        shaderEntry->gpu_render_states,
        (shaderEntry->gpu_render_state_count + 1) * sizeof(RC2D_GPURenderStateBindingEntry)
    );

    RC2D_assert_release(newEntries != NULL, RC2D_LOG_CRITICAL, "trackGraphicsRenderState: realloc failed");
    shaderEntry->gpu_render_states = newEntries;

    RC2D_GPURenderStateBindingEntry* binding = &shaderEntry->gpu_render_states[shaderEntry->gpu_render_state_count++];
    binding->state_handle = state_handle;
    binding->num_sampler_bindings = num_sampler_bindings;
    binding->sampler_bindings = NULL;

    if (num_sampler_bindings > 0 && sampler_bindings != NULL)
    {
        binding->sampler_bindings = (SDL_GPUTextureSamplerBinding*)RC2D_malloc(
            (size_t)num_sampler_bindings * sizeof(SDL_GPUTextureSamplerBinding)
        );
        RC2D_assert_release(binding->sampler_bindings != NULL, RC2D_LOG_CRITICAL, "trackGraphicsRenderState: malloc failed");
        SDL_memcpy(binding->sampler_bindings,
                   sampler_bindings,
                   (size_t)num_sampler_bindings * sizeof(SDL_GPUTextureSamplerBinding));
    }

    shaderEntry->gpu_render_state = *state_handle;

    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
    RC2D_log(RC2D_LOG_INFO, "Tracked GPURenderState for shader: %s", shader_storage_path);
    return true;
}

void rc2d_gpu_untrackGraphicsRenderState(SDL_GPURenderState** state_handle)
{
    if (state_handle == NULL)
    {
        return;
    }

    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    for (int i = 0; i < rc2d_engine_state.gpu_graphics_shader_count; ++i)
    {
        RC2D_GraphicsShaderEntry* shaderEntry = &rc2d_engine_state.gpu_graphics_shaders_cache[i];
        for (int j = 0; j < shaderEntry->gpu_render_state_count; ++j)
        {
            if (shaderEntry->gpu_render_states[j].state_handle == state_handle)
            {
                RC2D_safe_free(shaderEntry->gpu_render_states[j].sampler_bindings);

                const int remaining = shaderEntry->gpu_render_state_count - (j + 1);
                if (remaining > 0)
                {
                    SDL_memmove(&shaderEntry->gpu_render_states[j],
                                &shaderEntry->gpu_render_states[j + 1],
                                (size_t)remaining * sizeof(RC2D_GPURenderStateBindingEntry));
                }
                shaderEntry->gpu_render_state_count--;
                j--;
            }
        }

        if (shaderEntry->gpu_render_state_count == 0)
        {
            RC2D_safe_free(shaderEntry->gpu_render_states);
            shaderEntry->gpu_render_states = NULL;
            shaderEntry->gpu_render_state = NULL;
        }
    }

    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
}

void rc2d_gpu_hotReloadGraphicsShaders(void)
{
    if (!rc2d_engine_state.gpu_graphics_shader_mutex || rc2d_engine_state.renderer == NULL)
    {
        return;
    }

    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    for (int i = 0; i < rc2d_engine_state.gpu_graphics_shader_count; ++i)
    {
        RC2D_GraphicsShaderEntry* entry = &rc2d_engine_state.gpu_graphics_shaders_cache[i];
        if (entry->filename == NULL || entry->shader == NULL)
        {
            continue;
        }

        SDL_GPUShaderStage stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        char rootShaders[320];
        char base[256];
        char sourcePath[512];
        if (!rc2d_gpu_extractStageAndPaths(entry->filename,
                                           &stage,
                                           rootShaders,
                                           sizeof(rootShaders),
                                           base,
                                           sizeof(base),
                                           sourcePath,
                                           sizeof(sourcePath)))
        {
            continue;
        }
        (void)rootShaders;
        (void)base;

        SDL_Time currentModified = rc2d_gpu_getStorageFileModificationTime(sourcePath, entry->storage_kind);
        if (currentModified == 0 || currentModified <= entry->lastModified)
        {
            continue;
        }

        RC2D_GPUShader* newShader = rc2d_gpu_compileGraphicsShaderHotReload(
            entry->filename,
            entry->storage_kind,
            stage,
            sourcePath
        );
        if (newShader == NULL)
        {
            continue;
        }

        bool rebuildOk = true;
        for (int j = 0; j < entry->gpu_render_state_count; ++j)
        {
            RC2D_GPURenderStateBindingEntry* binding = &entry->gpu_render_states[j];
            if (binding->state_handle == NULL || *binding->state_handle == NULL)
            {
                continue;
            }

            SDL_GPURenderStateCreateInfo createInfo = {
                .fragment_shader = newShader,
                .num_sampler_bindings = binding->num_sampler_bindings,
                .sampler_bindings = binding->sampler_bindings
            };

            SDL_GPURenderState* newState = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &createInfo);
            if (newState == NULL)
            {
                RC2D_log(RC2D_LOG_ERROR,
                         "Hot reload: failed to recreate GPURenderState for shader %s: %s",
                         entry->filename,
                         SDL_GetError());
                rebuildOk = false;
                break;
            }

            SDL_GPURenderState* oldState = *binding->state_handle;
            *binding->state_handle = newState;
            SDL_DestroyGPURenderState(oldState);
        }

        if (!rebuildOk)
        {
            SDL_ReleaseGPUShader(rc2d_gpu_getDevice(), newShader);
            continue;
        }

        // IMPORTANT:
        // On ne libère pas automatiquement l'ancien shader ici pour éviter de casser les
        // pointeurs conservés côté application en mode hot reload Renderer.
        entry->shader = newShader;
        entry->lastModified = currentModified;
        entry->gpu_render_state = (entry->gpu_render_state_count > 0 && entry->gpu_render_states[0].state_handle != NULL)
                                      ? *entry->gpu_render_states[0].state_handle
                                      : NULL;

        RC2D_log(RC2D_LOG_INFO, "Hot reload graphics shader success: %s", entry->filename);
    }

    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
}
#endif // RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
