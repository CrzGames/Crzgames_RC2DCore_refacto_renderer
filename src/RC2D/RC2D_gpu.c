#include <RC2D/RC2D_gpu.h>
#include <RC2D/RC2D_assert.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_platform_defines.h>
#include <RC2D/RC2D_memory.h>

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_filesystem.h>

#if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
#include <SDL3_shadercross/SDL_shadercross.h>
#endif

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

RC2D_GPUShader* rc2d_gpu_loadGraphicsShader(const char* filename) 
{
    /**
     * Vérification des paramètres d'entrée
     */
    RC2D_assert_release(filename != NULL, RC2D_LOG_CRITICAL, "Graphics Shader filename is NULL");

    /**
     * Locker le mutex pour éviter les accès concurrents au cache des shaders graphiques
     */
    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    /**
     * Vérifier si le shader graphique est déjà dans le cache (donc déjà chargé une fois)
     */
    for (int i = 0; i < rc2d_engine_state.gpu_graphics_shader_count; i++) 
    {
        if (SDL_strcmp(rc2d_engine_state.gpu_graphics_shaders_cache[i].filename, filename) == 0) 
        {
            RC2D_GPUShader* graphicsShader = rc2d_engine_state.gpu_graphics_shaders_cache[i].shader;
            SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

            RC2D_log(RC2D_LOG_INFO, "Graphics Shader already loaded from cache: %s", filename);
            return graphicsShader;
        }
    }

    /**
     * On unlock le mutex après avoir vérifié le cache des shaders graphiques.
     * Cela permet aux autres threads d'accéder au cache des shaders graphiques.
     */
    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    // Récupérer le chemin de base de l'application (où est exécuté l'exécutable)
    const char* basePath = SDL_GetBasePath();
    RC2D_assert_release(basePath != NULL, RC2D_LOG_CRITICAL, "SDL_GetBasePath() failed, SDL_Error: %s", SDL_GetError());

    /**
     * Déterminer le stage en fonction du suffixe (vertex ou fragment)
     * On utilise SDL_strstr pour vérifier la présence de ".vertex" ou ".fragment" dans le nom du fichier
     */
    SDL_GPUShaderStage stage;
    if (SDL_strstr(filename, ".vertex")) 
    {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } 
    else if (SDL_strstr(filename, ".fragment")) 
    {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } 
    else 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Unknown shader stage suffix: expected .vertex or .fragment.");
        return NULL;
    }

    /**
     * fullPath : Chemin d'accès complet au fichier binaire du shader compilé ou au fichier HLSL source.
     */
    char fullPath[512];

#if !RC2D_GPU_SHADER_HOT_RELOAD_ENABLED // Compilation hors ligne des shaders graphics en HLSL
    /**
     * entrypoint : Point d'entrée du shader (main pour SPIR-V, DXIL et main0 pour MSL).
     */
    const char* entrypoint = NULL;

    // Récupérer les formats supportés par le backend actuel
    SDL_GPUShaderFormat backendFormatsSupported = SDL_GetGPUSupportedShaderFormats(rc2d_gpu_getDevice());

    // Le format de shader à utiliser pour la compilation hors ligne
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;

    // Générer le chemin d'accès au fichier binaire du shader compilé en fonction du backend
    if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_SPIRV) 
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/spirv/%s.spv", basePath, filename);
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
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/metallib/ios/%s.metallib", basePath, filename);
#else
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/metallib/macos/%s.metallib", basePath, filename);
#endif // RC2D_PLATFORM_IOS
        format = SDL_GPU_SHADERFORMAT_METALLIB;
        entrypoint = "main";
    }
    else if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_MSL)
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/msl/%s.msl", basePath, filename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0"; // SDL_shadercross requiert "main0" pour MSL
    } 
    else if (backendFormatsSupported & SDL_GPU_SHADERFORMAT_DXIL) 
    {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/dxil/%s.dxil", basePath, filename);
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
     * On utilise SDL_LoadFile pour charger le fichier binaire du shader compilé
     */
    size_t codeShaderCompiledSize = 0;
    void* codeShaderCompiled = SDL_LoadFile(fullPath, &codeShaderCompiledSize);
    if (codeShaderCompiled == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load compiled shader: %s, SDL_Error: %s", fullPath, SDL_GetError());
        return NULL;
    }

    /**
     * En mode compilation hors ligne des shaders, on utilise un fichier JSON généré par le script de compilation des shaders.
     * On génère le chemin d'accès au fichier JSON de réflexion en fonction du nom du shader et de son stage.
     */
    char jsonPath[512];
    SDL_snprintf(jsonPath, sizeof(jsonPath), "%sshaders/reflection/%s.json", basePath, filename);

    /**
     * On ouvre le fichier JSON de réflexion pour récupérer les informations de réflexions sur le shader.
     * On utilise SDL_LoadFile pour charger le fichier JSON de réflexion.
     * On utilise SDL_sscanf pour extraire les valeurs des champs : samplers, uniform_buffers, storage_buffers et storage_textures.
     * On ferme le fichier JSON après la lecture.
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
    void* jsonContent = SDL_LoadFile(jsonPath, NULL);
    if (jsonContent) 
    {
        // Cast le contenu du fichier JSON en chaîne de caractères
        const char* content = (const char*)jsonContent;
    
        // Essayer les différentes clés connues, y compris alternatives
        if (SDL_strstr(content, "\"samplers\""))
            SDL_sscanf(content, "%*[^\"samplers\"]\"samplers\"%*[: ]%u", &numSamplers);
        if (SDL_strstr(content, "\"uniform_buffers\""))
            SDL_sscanf(content, "%*[^\"uniform_buffers\"]\"uniform_buffers\"%*[: ]%u", &numUniformBuffers);
        else if (SDL_strstr(content, "\"uniformBuffers\""))
            SDL_sscanf(content, "%*[^\"uniformBuffers\"]\"uniformBuffers\"%*[: ]%u", &numUniformBuffers);
        if (SDL_strstr(content, "\"storage_buffers\""))
            SDL_sscanf(content, "%*[^\"storage_buffers\"]\"storage_buffers\"%*[: ]%u", &numStorageBuffers);
        else if (SDL_strstr(content, "\"readOnlyStorageBuffers\""))
            SDL_sscanf(content, "%*[^\"readOnlyStorageBuffers\"]\"readOnlyStorageBuffers\"%*[: ]%u", &numStorageBuffers);
        if (SDL_strstr(content, "\"storage_textures\""))
            SDL_sscanf(content, "%*[^\"storage_textures\"]\"storage_textures\"%*[: ]%u", &numStorageTextures);
        else if (SDL_strstr(content, "\"readOnlyStorageTextures\""))
            SDL_sscanf(content, "%*[^\"readOnlyStorageTextures\"]\"readOnlyStorageTextures\"%*[: ]%u", &numStorageTextures);
    
        // Libérer le contenu JSON après la lecture
        RC2D_safe_free(jsonContent);
    }
    else 
    {
        RC2D_log(RC2D_LOG_ERROR, "Shader reflection file not found: %s, SDL_Error: %s", jsonPath, SDL_GetError());
    }
    
    // Création du shader GPU avec les informations de réflexion récupérées depuis le fichier JSON
    SDL_GPUShaderCreateInfo info = {
        .code = codeShaderCompiled,
        .code_size = codeShaderCompiledSize,
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
        RC2D_log(RC2D_LOG_ERROR, "Failed to create GPU graphics shader from file %s, SDL_Error: %s", filename, SDL_GetError());
        return NULL;
    }

#else // Compilation en ligne des shaders graphics en HLSL
    /**
     * On génère le chemin d'accès au fichier HLSL source en fonction du nom du shader et de son stage pour la compilation en ligne des shaders.
     * On utilise SDL_snprintf pour formater le chemin d'accès au fichier HLSL source.
     */
    SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/src/%s.hlsl", basePath, filename);

    /**
     * Charger le fichier HLSL source
     * On utilise SDL_LoadFile pour charger le fichier HLSL source.
     */
    char* codeHLSLSource = SDL_LoadFile(fullPath, NULL);
    if (codeHLSLSource == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load HLSL shader source: %s, SDL_Error: %s", fullPath, SDL_GetError()); 
        return NULL;
    }

    // Préparer les informations pour la compilation HLSL vers SPIR-V
    SDL_ShaderCross_HLSL_Info hlslInfo = {
        .source = codeHLSLSource,
        .entrypoint = "main",
        .include_dir = NULL,
        .defines = NULL,
        .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
        .enable_debug = true,
        .name = filename,
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
        RC2D_log(RC2D_LOG_ERROR, "Failed to compile HLSL to SPIR-V: %s", filename);
        return NULL;
    }

    // Réfléchir les métadonnées du shader graphique
    SDL_ShaderCross_GraphicsShaderMetadata* metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(
        spirvByteCode, spirvByteCodeSize, 0
    );
    if (metadata == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to reflect graphics shader metadata: %s", filename);
        RC2D_safe_free(spirvByteCode);
        return NULL;
    }

    // Préparer les informations SPIR-V pour la compilation du shader
    SDL_ShaderCross_SPIRV_Info spirvInfo = {
        .bytecode = spirvByteCode,
        .bytecode_size = spirvByteCodeSize,
        .entrypoint = "main",
        .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
        .enable_debug = true,
        .name = filename,
        .props = 0
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
        RC2D_log(RC2D_LOG_ERROR, "Failed to create GPU graphics shader from SPIR-V: %s", filename);
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
    entry->filename = RC2D_strdup(filename);
    entry->shader = graphicsShader;
    entry->lastModified = rc2d_gpu_getFileModificationTime(fullPath);

    /**
     * On unlock le mutex après avoir ajouté le shader graphique au cache.
     * Cela permet aux autres threads d'accéder au cache des shaders graphiques.
     */
    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
#endif

    RC2D_log(RC2D_LOG_INFO, "Graphics Shader loaded and cached: %s", filename);
    return graphicsShader;
}

void rc2d_gpu_hotReloadGraphicsShaders(void)
{
#if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
    /**
     * Vérifier si les mutex pour les shaders graphiques sont initialisés
     */
    if (!rc2d_engine_state.gpu_graphics_shader_mutex)
    {
        RC2D_assert_release(false, RC2D_LOG_CRITICAL, "gpu_graphics_shader_mutex is NULL");
    }

    /**
     * Récupérer le chemin de base de l'application (où est exécuté l'exécutable)
     */
    const char* basePath = SDL_GetBasePath();
    if (basePath == NULL) 
    {
        RC2D_assert_release(false, RC2D_LOG_CRITICAL, "SDL_GetBasePath() failed, SDL_Error: %s", SDL_GetError());
        return;
    }

    /**
     * On verrouille les mutex pour éviter les conflits d'accès concurrents
     * lors de la mise à jour des shaders graphiques
     */
    SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);

    /**
     * Parcourir tous les shaders graphiques dans le cache
     * et vérifier s'ils ont été modifiés depuis leur dernière compilation.
     */
    for (int i = 0; i < rc2d_engine_state.gpu_graphics_shader_count; i++) 
    {
        // Récupérer le shader graphique à partir du cache
        RC2D_GraphicsShaderEntry* entry = &rc2d_engine_state.gpu_graphics_shaders_cache[i];

        /**
         * Générer le chemin d'accès complet au fichier HLSL source
         * 
         * En utilisant le nom du fichier du shader graphique récupéré du cache,
         * via `entry->filename`.
         * 
         * On utilise SDL_snprintf pour formater le chemin d'accès au fichier HLSL source.
         */
        char fullPath[512];
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/src/%s.hlsl", basePath, entry->filename);

        /**
         * Vérifier si le fichier HLSL source a été modifié depuis la dernière compilation
         * 
         * On utilise `rc2d_gpu_getFileModificationTime` pour obtenir le temps de modification du fichier.
         */
        SDL_Time currentModified = rc2d_gpu_getFileModificationTime(fullPath);

        // Si le fichier HLSL source a été modifié depuis la dernière compilation
        if (currentModified > entry->lastModified) 
        {            
            // Déterminer le stage
            SDL_GPUShaderStage stage;
            if (SDL_strstr(entry->filename, ".vertex")) 
            {
                stage = SDL_GPU_SHADERSTAGE_VERTEX;
            } 
            else if (SDL_strstr(entry->filename, ".fragment")) 
            {
                stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
            } 
            else 
            {
                RC2D_log(RC2D_LOG_ERROR, "Unknown shader stage for %s during reload", entry->filename);
                continue;
            }

            /**
             * Charger le fichier HLSL
             * 
             * Attend un peu que l'os ou l'ide est le temps d'écrire le 
             * fichier avant de le lire
             */
            char* codeHLSLSource = NULL;
            for (int attempt = 0; attempt < 3; attempt++) {
                codeHLSLSource = SDL_LoadFile(fullPath, NULL);
                if (codeHLSLSource != NULL) break;
                SDL_Delay(20);
            }

            /**
             * Si le code HLSL n'a pas pu être chargé après plusieurs tentatives, log l'erreur
             * et continue avec le prochain shader graphique.
             */
            if (!codeHLSLSource) 
            {
                RC2D_log(RC2D_LOG_ERROR, "Failed to load HLSL shader source after retries: %s", fullPath);
                continue;
            }

            // Préparer les informations HLSL
            SDL_ShaderCross_HLSL_Info hlslInfo = {
                .source = codeHLSLSource,
                .entrypoint = "main",
                .include_dir = NULL,
                .defines = NULL,
                .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
                .enable_debug = true,
                .name = entry->filename,
                .props = 0
            };

            // Temps de début pour mesurer la compilation
            Uint64 t0 = SDL_GetPerformanceCounter();

            // Compiler HLSL vers SPIR-V
            size_t spirvByteCodeSize = 0;
            void* spirvByteCode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlslInfo, &spirvByteCodeSize);

            // Libérer le code HLSL source après la compilation
            RC2D_safe_free(codeHLSLSource);

            // Vérifier si la compilation HLSL vers SPIR-V a réussi
            if (spirvByteCode == NULL || spirvByteCodeSize == 0)
            {
                RC2D_log(RC2D_LOG_ERROR, "Failed to compile HLSL to SPIR-V during reload: %s", entry->filename);
                continue;
            }

            // Réfléchir les métadonnées
            SDL_ShaderCross_GraphicsShaderMetadata* metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(
                spirvByteCode, 
                spirvByteCodeSize, 
                0
            );
            // Vérifier si la réflexion des métadonnées a réussi
            if (!metadata) 
            {
                RC2D_log(RC2D_LOG_ERROR, "Failed to reflect graphics shader metadata during reload: %s", entry->filename);
                RC2D_safe_free(spirvByteCode);
                continue;
            }

            // Préparer les informations SPIR-V
            SDL_ShaderCross_SPIRV_Info spirvInfo = {
                .bytecode = spirvByteCode,
                .bytecode_size = spirvByteCodeSize,
                .entrypoint = "main",
                .shader_stage = (SDL_ShaderCross_ShaderStage)stage,
                .enable_debug = true,
                .name = entry->filename,
                .props = 0
            };

            // Compiler le shader
            SDL_GPUShader* newShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
                rc2d_gpu_getDevice(),
                &spirvInfo,
                metadata,
                0
            );

            // Temps de fin pour mesurer la compilation
            Uint64 t1 = SDL_GetPerformanceCounter();
            double compileTimeMs = (double)(t1 - t0) * 1000.0 / SDL_GetPerformanceFrequency();

            // Libérer les ressources allouées pour les métadonnées et le code SPIR-V
            RC2D_safe_free(metadata);
            RC2D_safe_free(spirvByteCode);

            if (newShader) 
            {
                /**
                 * Créer un nouvel état de rendu GPU pour le shader
                 */
                SDL_GPURenderStateCreateInfo renderStateInfo = {
                    .fragment_shader = (stage == SDL_GPU_SHADERSTAGE_FRAGMENT) ? newShader : NULL,
                    .num_sampler_bindings = metadata->num_samplers,
                    .sampler_bindings = NULL, // À adapter si vous utilisez des samplers
                    .num_storage_textures = metadata->num_storage_textures,
                    .storage_textures = NULL, // À adapter si nécessaire
                    .num_storage_buffers = metadata->num_storage_buffers,
                    .storage_buffers = NULL, // À adapter si nécessaire
                    .props = 0
                };

                SDL_GPURenderState* newRenderState = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &renderStateInfo);
                if (!newRenderState)
                {
                    RC2D_log(RC2D_LOG_ERROR, "Failed to create GPU render state for shader: %s, SDL_Error: %s", entry->filename, SDL_GetError());
                    SDL_ReleaseGPUShader(rc2d_gpu_getDevice(), newShader);
                    continue;
                }

                /**
                 * Synchronisation : Attendre la fin du rendu en cours
                 * L'API Renderer GPU gère la synchronisation via SDL_RenderPresent
                 */
                //SDL_RenderPresent(rc2d_engine_state.renderer);

                // Libérer l'ancien shader et l'ancien état de rendu
                if (entry->shader) 
                {
                    SDL_ReleaseGPUShader(rc2d_gpu_getDevice(), entry->shader);
                }
                if (entry->gpu_render_state)
                {
                    SDL_DestroyGPURenderState(entry->gpu_render_state);
                }

                // Mettre à jour le cache avec le nouveau shader et l'état de rendu
                entry->shader = newShader;
                entry->gpu_render_state = newRenderState;
                entry->lastModified = currentModified;

                RC2D_log(RC2D_LOG_INFO, "Successfully Shader %s reloaded in %.2f ms", entry->filename, compileTimeMs);
            } 
            else 
            {
                // Log l'erreur si la compilation du shader graphique a échoué
                RC2D_log(RC2D_LOG_ERROR, "Failed to reload shader: %s", entry->filename);
            }
        }
    }

    /**
     * Unlock les mutex pour permettre l'accès concurrent aux shaders graphiques
     */
    SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
#endif
}