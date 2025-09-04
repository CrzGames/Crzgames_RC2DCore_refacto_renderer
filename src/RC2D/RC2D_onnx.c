#if RC2D_ONNX_MODULE_ENABLED

#include <RC2D/RC2D_onnx.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_platform_defines.h>
#include <RC2D/RC2D_memory.h>

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_filesystem.h>

#ifdef RC2D_PLATFORM_WIN32
#include <windows.h>
#endif 

/**
 * Contexte global ONNX Runtime (logger, thread pool partagé, etc.)
 */
static OrtEnv* g_ort_env = NULL;

/**
 * Configuration de la session (optimisation, EPs, threads, etc.)
 */
static OrtSessionOptions* g_session_options = NULL;

/**
 * Calcule la taille d'un type ONNX en octets.
 * 
 * @param type Type ONNX à évaluer.
 * @return Taille en octets du type ONNX.
 */
static size_t rc2d_onnx_getTypeSize(ONNXTensorElementDataType type)
{
    switch (type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:   return sizeof(float);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:   return sizeof(uint8_t);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8:    return sizeof(int8_t);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16:  return sizeof(uint16_t);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16:   return sizeof(int16_t);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:   return sizeof(int32_t);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:   return sizeof(int64_t);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:  return sizeof(double);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL:    return sizeof(bool);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32:  return sizeof(uint32_t);
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64:  return sizeof(uint64_t);
        default: return 0; // STRING / autres types = non supportés ici
    }
}

/**
 * Calcule le nombre total d'éléments dans un tenseur ONNX à partir de sa forme.
 * 
 * @param shape Tableau contenant la forme du tenseur (dimensions).
 * @param dims Nombre de dimensions du tenseur.
 * @return Nombre total d'éléments dans le tenseur.
 */
static size_t rc2d_onnx_computeElementCount(const int64_t* shape, size_t dims)
{
    size_t count = 1;
    for (size_t i = 0; i < dims; ++i) count *= (size_t)shape[i];
    return count;
}

bool rc2d_onnx_init(void) 
{
    OrtStatus* status;

    // Récupère l’API ONNX Runtime (interface principale vers les fonctions)
    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);

    // Crée un environnement ONNX Runtime avec un niveau de log "warning"
    status = ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "rc2d", &g_ort_env);
    if (status != NULL) 
    {
        // Si la création échoue, log l’erreur et retourne false
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to create ONNX Runtime environment");
        return false;
    }

    // Crée les options de session (permet de configurer les EPs, optimisations, etc.)
    status = ort->CreateSessionOptions(&g_session_options);
    if (status != NULL) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to create session options");

        // Libère l’environnement ONNX Runtime si la création des options échoue
        ort->ReleaseEnv(g_ort_env);
        g_ort_env = NULL;

        return false;
    }

    /**
     * Active la gestion de la mémoire CPU pour améliorer les performances
     * 
     * Au lieu de faire plein de petits malloc()/free() pendant l’exécution d’un modèle, 
     * ONNX Runtime utilise une "arena allocator" :
     * 
     * Une grande zone mémoire préallouée (l'arène) qui est réutilisée efficacement pendant l’inférence.
     */
    ort->EnableCpuMemArena(g_session_options);

    // Définit le niveau d’optimisation graphique au maximum (fusion, simplification, etc.)
    ort->SetSessionGraphOptimizationLevel(g_session_options, ORT_ENABLE_ALL);

    /**
     * Sélection automatique des Execution Providers (EPs) disponibles, 
     * du meilleur au pire donc : GPU/NPU -> CPU SIMD -> CPU
     */
    status = ort->SessionOptionsSetEpSelectionPolicy(g_session_options, OrtExecutionProviderDevicePolicy_MAX_PERFORMANCE);
    if (status != NULL) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to set EP selection policy");

        // Libère les options de session et l’environnement ONNX Runtime
        ort->ReleaseSessionOptions(g_session_options);
        g_session_options = NULL;
        ort->ReleaseEnv(g_ort_env);
        g_ort_env = NULL;

        return false;
    }

    // Liste les Execution Providers disponibles et les log
    const char** available_providers = NULL;
    int num_providers = 0;
    status = ort->GetAvailableProviders(&available_providers, &num_providers);
    if (status == NULL) 
    {
        RC2D_log(RC2D_LOG_INFO, "Available ONNX Execution Providers:");

        for (int i = 0; i < num_providers; ++i) 
        {
            RC2D_log(RC2D_LOG_INFO, "  - %s", available_providers[i]);
        }

        ort->ReleaseAvailableProviders(available_providers, num_providers);
    } 
    else 
    {
        RC2D_log(RC2D_LOG_WARN, "Failed to get available EPs");
    }

    // Tout s'est bien passé
    return true;
}

void rc2d_onnx_cleanup(void) 
{
    // Récupère l’API ONNX Runtime
    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);

    // Libère les options de session si elles ont été allouées
    if (g_session_options) {
        ort->ReleaseSessionOptions(g_session_options);
        g_session_options = NULL;
    }

    // Libère l’environnement ONNX Runtime si alloué
    if (g_ort_env) 
    {
        ort->ReleaseEnv(g_ort_env);
        g_ort_env = NULL;
    }
}

bool rc2d_onnx_loadModel(RC2D_OnnxModel* model)
{
    /**
     * Vérifie que le modèle est valide
     * 
     * Le modèle doit avoir un chemin valide (non NULL)
     */
    if (model == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Model pointer is NULL");
        return false;
    }
    if (model->path == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Model path is NULL");
        return false;
    }

    /**
     * Récupère l’API ONNX Runtime (interface principale vers les fonctions)
     */
    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);

    /**
     * Récupère le chemin de base de l’application
     * SDL_GetBasePath() renvoie le chemin du répertoire de l’exécutable
     */
    const char* base_path = SDL_GetBasePath();
    if (base_path == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "SDL_GetBasePath() failed to get base path %s", SDL_GetError());
        return false;
    }

    // Construit le chemin complet vers le modèle ONNX
    char full_path[1024];
    SDL_snprintf(full_path, sizeof(full_path), "%s%s",
                 base_path,
                 model->path);

    /**
     * IMPORTANT:
     * Pour Windows, on doit convertir le chemin en UTF-16
     * Le type de chemin attendu par ONNX Runtime est wchar_t* (UTF-16)
     * Le chemin est converti avec MultiByteToWideChar()
     */
#ifdef RC2D_PLATFORM_WIN32
    wchar_t full_path_w[1024];
    MultiByteToWideChar(CP_UTF8, 0, full_path, -1, full_path_w, 1024);
    // Crée la session avec le modèle ONNX
    OrtStatus* status = ort->CreateSession(g_ort_env, full_path_w, g_session_options, &model->session);
#else
    // Crée la session avec le modèle ONNX
    OrtStatus* status = ort->CreateSession(g_ort_env, full_path, g_session_options, &model->session);
#endif
    if (status != NULL) 
    {
        OrtErrorCode err_code = ort->GetErrorCode(status);
        const char* msg = ort->GetErrorMessage(status);
        RC2D_log(RC2D_LOG_CRITICAL, "Message from ONNX Runtime: Code=%d, Message=%s", err_code, msg);
        ort->ReleaseStatus(status);
        return false;
    }

    RC2D_log(RC2D_LOG_INFO, "ONNX model loaded: %s", full_path);
    return true;
}

void rc2d_onnx_unloadModel(RC2D_OnnxModel* model)
{
    if (!model || !model->session)
        return;

    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    ort->ReleaseSession(model->session);
    model->session = NULL;

    RC2D_log(RC2D_LOG_INFO, "ONNX model session unloaded.");
}

bool rc2d_onnx_run(RC2D_OnnxModel* model, RC2D_OnnxTensor* inputs, RC2D_OnnxTensor* outputs)
{   
    // Vérifier les paramètres d'entrée
    if (model == NULL || model->session == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Model is not loaded");
        return false;
    }

    if (inputs == NULL || outputs == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Inputs or outputs are NULL");
        return false;
    }

    if (inputs->data == NULL || outputs->data == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Input or output data is NULL");
        return false;
    }

    if (inputs->type == ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Input type is undefined");
        return false;
    }

    /**
     * Récupère l’API ONNX Runtime (interface principale vers les fonctions)
     */
    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);

    /**
     * Récupère l'allocateur par défaut d'ONNX Runtime
     * 
     * L'allocateur est utilisé ici pour allouer les noms d’entrées/sorties
     * via `SessionGetInputName()` / `SessionGetOutputName()`, qui renvoient
     * une chaîne allouée dynamiquement que l’on doit libérer avec `allocator->Free()`.
     */
    OrtAllocator* allocator = NULL;
    ort->GetAllocatorWithDefaultOptions(&allocator);

    /**
     * Récupère dynamiquement le nombre d’entrées et de sorties attendus par le modèle.
     *
     * Cela permet de s’adapter à n’importe quel modèle ONNX sans coder en dur
     * le nombre d’inputs/outputs.
     */
    size_t input_count = 0;
    ort->SessionGetInputCount(model->session, &input_count);
    size_t output_count = 0;
    ort->SessionGetOutputCount(model->session, &output_count);

    /**
     * Crée une description de la mémoire CPU utilisée pour les tenseurs.
     * 
     * Cette info est requise par `CreateTensorWithDataAsOrtValue` pour indiquer
     * que les données sont en mémoire CPU standard (non GPU).
     */
    OrtMemoryInfo* memory_info = NULL;
    ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info);

    /**
     * Alloue les buffers nécessaires pour stocker :
     * - les noms des entrées (`input_names`)
     * - les OrtValue* représentant les entrées (`input_values`)
     * 
     * Chaque `OrtValue*` représente un tenseur à passer au moteur d’inférence.
     */
    OrtValue** input_values = RC2D_malloc(sizeof(OrtValue*) * input_count);
    if (!input_values) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to allocate input_values");
        ort->ReleaseMemoryInfo(memory_info);
        return false;
    }

    const char** input_names = RC2D_malloc(sizeof(const char*) * input_count);
    if (!input_names) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to allocate input_names");
        RC2D_safe_free(input_values);
        ort->ReleaseMemoryInfo(memory_info);
        return false;
    }

    /**
     * Prépare les OrtValue* pour chaque entrée (input) :
     * - Récupère le nom de l’entrée depuis le modèle ONNX
     * - Vérifie la forme en fonction de model->dynamic_batch
     * - Crée un tenseur avec les données utilisateur
     * 
     * Si model->dynamic_batch est true, accepte une forme [N, M] où N > 0
     * (par exemple, [150, 3] pour 150 PNJ avec [x, y, santé]).
     * Sinon, impose une forme statique [1, M] (par exemple, [1, 3] pour un PNJ).
     */
    for (size_t i = 0; i < input_count; ++i)
    {
        // Récupère le nom de l’entrée (input) i depuis le modèle ONNX
        char* name = NULL;
        ort->SessionGetInputName(model->session, i, allocator, &name);
        input_names[i] = name;

        // Vérifie la correspondance du nom de l’entrée
        if (SDL_strcmp(name, inputs[i].name) != 0) {
            RC2D_log(RC2D_LOG_CRITICAL, "Input name mismatch: expected %s, got %s", inputs[i].name, name);
            for (size_t j = 0; j <= i; ++j) {
                if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
            }
            RC2D_safe_free(input_values);
            RC2D_safe_free(input_names);
            ort->ReleaseMemoryInfo(memory_info);
            return false;
        }

        // Valide la forme du tenseur d’entrée
        if (model->dynamic_batch) {
            if (inputs[i].shape[0] <= 0) {
                RC2D_log(RC2D_LOG_CRITICAL, "Invalid input batch size %lld for dynamic batch", inputs[i].shape[0]);
                for (size_t j = 0; j <= i; ++j) {
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
        } else {
            if (inputs[i].shape[0] != 1) {
                RC2D_log(RC2D_LOG_CRITICAL, "Expected static input batch size 1, got %lld", inputs[i].shape[0]);
                for (size_t j = 0; j <= i; ++j) {
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
        }

        // Calcule la taille mémoire du tenseur en bytes
        size_t total_size = rc2d_onnx_getTypeSize(inputs[i].type) * rc2d_onnx_computeElementCount(inputs[i].shape, inputs[i].dims);

        if (inputs[i].type == ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING)
        {
            // Pour les chaînes de caractères, on crée un tenseur vide, puis on le remplit
            OrtStatus* status = ort->CreateTensorAsOrtValue(allocator, inputs[i].shape, inputs[i].dims, inputs[i].type, &input_values[i]);
            if (status != NULL || input_values[i] == NULL) {
                RC2D_log(RC2D_LOG_CRITICAL, "Failed to create string input tensor %zu: %s", i,
                         status ? ort->GetErrorMessage(status) : "Null OrtValue");
                if (status) ort->ReleaseStatus(status);
                for (size_t j = 0; j < i; ++j) {
                    if (input_values[j]) ort->ReleaseValue(input_values[j]);
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
            size_t string_count = rc2d_onnx_computeElementCount(inputs[i].shape, inputs[i].dims);
            ort->FillStringTensor(input_values[i], (const char* const*)inputs[i].data, string_count);
        }
        else
        {
            /**
             * Pour les types scalaires (float, int, bool, etc.), on crée directement 
             * le tenseur avec les données utilisateur
             */
            OrtStatus* status = ort->CreateTensorWithDataAsOrtValue(memory_info,
                                                                    inputs[i].data, total_size,
                                                                    inputs[i].shape, inputs[i].dims,
                                                                    inputs[i].type, &input_values[i]);
            if (status != NULL || input_values[i] == NULL) {
                RC2D_log(RC2D_LOG_CRITICAL, "Failed to create scalar input tensor %zu: %s", i,
                         status ? ort->GetErrorMessage(status) : "Null OrtValue");
                if (status) ort->ReleaseStatus(status);
                for (size_t j = 0; j < i; ++j) {
                    if (input_values[j]) ort->ReleaseValue(input_values[j]);
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
        }
    }

    /**
     * Même logique que pour les entrées, mais pour les sorties :
     * - Alloue les noms des sorties (`output_names`)
     * - Prépare les `OrtValue*` pour recevoir les résultats de l’inférence
     * - Vérifie la forme en fonction de model->dynamic_batch
     *
     * Les buffers de sorties doivent être pré-alloués par l'utilisateur
     * et associés dans `outputs[i].data`. Pour un batch dynamique, la sortie
     * doit avoir une forme [N, M] où N correspond à l’entrée (par exemple, [150, 3]
     * pour 150 PNJ avec scores [left, right, jump]).
     */
    OrtValue** output_values = RC2D_malloc(sizeof(OrtValue*) * output_count);
    if (!output_values) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to allocate output_values");
        for (size_t i = 0; i < input_count; ++i) {
            if (input_values[i]) ort->ReleaseValue(input_values[i]);
            if (input_names[i]) allocator->Free(allocator, (void*)input_names[i]);
        }
        RC2D_safe_free(input_values);
        RC2D_safe_free(input_names);
        ort->ReleaseMemoryInfo(memory_info);
        return false;
    }

    const char** output_names = RC2D_malloc(sizeof(const char*) * output_count);
    if (!output_names) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to allocate output_names");
        for (size_t i = 0; i < input_count; ++i) {
            if (input_values[i]) ort->ReleaseValue(input_values[i]);
            if (input_names[i]) allocator->Free(allocator, (void*)input_names[i]);
        }
        RC2D_safe_free(input_values);
        RC2D_safe_free(input_names);
        RC2D_safe_free(output_values);
        ort->ReleaseMemoryInfo(memory_info);
        return false;
    }

    /**
     * Prépare les OrtValue* pour chaque sortie (output) :
     * - Récupère le nom de la sortie depuis le modèle ONNX
     * - Valide la forme ([N, M] pour dynamique, [1, M] pour statique)
     * - Crée un tenseur avec les données utilisateur
     */
    for (size_t i = 0; i < output_count; ++i)
    {
        // Récupère le nom de la sortie (output) i depuis le modèle ONNX
        char* name = NULL;
        ort->SessionGetOutputName(model->session, i, allocator, &name);
        output_names[i] = name;

        // Récupère dynamiquement type, shape et dims de la sortie
        OrtTypeInfo* type_info = NULL;
        ort->SessionGetOutputTypeInfo(model->session, i, &type_info);

        const OrtTensorTypeAndShapeInfo* tensor_info = NULL;
        ort->CastTypeInfoToTensorInfo(type_info, &tensor_info);

        ONNXTensorElementDataType type;
        ort->GetTensorElementType(tensor_info, &type);
        outputs[i].type = type;

        size_t num_dims = 0;
        ort->GetDimensionsCount(tensor_info, &num_dims);
        outputs[i].dims = num_dims;

        ort->GetDimensions(tensor_info, outputs[i].shape, num_dims);
        ort->ReleaseTypeInfo(type_info);

        // Valide la forme du tenseur de sortie
        if (model->dynamic_batch) {
            // Accepte une forme dynamique où shape[0] peut être -1 (indéfini) ou correspondre à l'entrée
            if (outputs[i].shape[0] != -1 && outputs[i].shape[0] != inputs[0].shape[0]) {
                RC2D_log(RC2D_LOG_CRITICAL, "Dynamic batch: Output batch size %lld does not match input batch size %lld",
                         outputs[i].shape[0], inputs[0].shape[0]);
                for (size_t j = 0; j < input_count; ++j) {
                    if (input_values[j]) ort->ReleaseValue(input_values[j]);
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                for (size_t j = 0; j <= i; ++j) {
                    if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                RC2D_safe_free(output_values);
                RC2D_safe_free(output_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
            // Normalise la forme de sortie pour correspondre à l’entrée
            outputs[i].shape[0] = inputs[0].shape[0];
        } else {
            // Impose une forme statique avec batch size = 1
            if (outputs[i].shape[0] != 1) {
                RC2D_log(RC2D_LOG_CRITICAL, "Static batch: Expected output batch size 1, got %lld", outputs[i].shape[0]);
                for (size_t j = 0; j < input_count; ++j) {
                    if (input_values[j]) ort->ReleaseValue(input_values[j]);
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                for (size_t j = 0; j <= i; ++j) {
                    if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                RC2D_safe_free(output_values);
                RC2D_safe_free(output_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
        }

        if (outputs[i].type == ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING)
        {
            // Crée un tenseur vide : ONNX Runtime va le remplir après l’inférence
            OrtStatus* status = ort->CreateTensorAsOrtValue(allocator, outputs[i].shape, outputs[i].dims, outputs[i].type, &output_values[i]);
            if (status != NULL || output_values[i] == NULL) {
                RC2D_log(RC2D_LOG_CRITICAL, "Failed to create string output tensor %zu: %s", i,
                         status ? ort->GetErrorMessage(status) : "Null OrtValue");
                if (status) ort->ReleaseStatus(status);
                for (size_t j = 0; j < input_count; ++j) {
                    if (input_values[j]) ort->ReleaseValue(input_values[j]);
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                for (size_t j = 0; j < i; ++j) {
                    if (output_values[j]) ort->ReleaseValue(output_values[j]);
                    if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                RC2D_safe_free(output_values);
                RC2D_safe_free(output_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
        }
        else
        {
            // Calcule la taille nécessaire pour les données de sortie
            size_t total_size = rc2d_onnx_getTypeSize(outputs[i].type) * rc2d_onnx_computeElementCount(outputs[i].shape, outputs[i].dims);
    
            // Cas classique pour float, int, etc.
            OrtStatus* status = ort->CreateTensorWithDataAsOrtValue(
                memory_info,
                outputs[i].data, total_size,
                outputs[i].shape, outputs[i].dims,
                outputs[i].type, &output_values[i]);
            if (status != NULL || output_values[i] == NULL) {
                RC2D_log(RC2D_LOG_CRITICAL, "Failed to create scalar output tensor %zu: %s", i,
                         status ? ort->GetErrorMessage(status) : "Null OrtValue");
                if (status) ort->ReleaseStatus(status);
                for (size_t j = 0; j < input_count; ++j) {
                    if (input_values[j]) ort->ReleaseValue(input_values[j]);
                    if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
                }
                for (size_t j = 0; j < i; ++j) {
                    if (output_values[j]) ort->ReleaseValue(output_values[j]);
                    if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
                }
                RC2D_safe_free(input_values);
                RC2D_safe_free(input_names);
                RC2D_safe_free(output_values);
                RC2D_safe_free(output_names);
                ort->ReleaseMemoryInfo(memory_info);
                return false;
            }
        }
    }

    // Vérifie si les OrtValue* ont été créées correctement (input/output values)
    for (size_t i = 0; i < input_count; ++i)
    {
        if (input_values[i] == NULL)
        {
            RC2D_log(RC2D_LOG_CRITICAL, "Failed to create OrtValue for input %zu", i);
            for (size_t j = 0; j < input_count; ++j) {
                if (input_values[j]) ort->ReleaseValue(input_values[j]);
                if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
            }
            for (size_t j = 0; j < output_count; ++j) {
                if (output_values[j]) ort->ReleaseValue(output_values[j]);
                if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
            }
            RC2D_safe_free(input_values);
            RC2D_safe_free(input_names);
            RC2D_safe_free(output_values);
            RC2D_safe_free(output_names);
            ort->ReleaseMemoryInfo(memory_info);
            return false;
        }
    }
    for (size_t i = 0; i < output_count; ++i)
    {
        if (output_values[i] == NULL)
        {
            RC2D_log(RC2D_LOG_CRITICAL, "Failed to create OrtValue for output %zu", i);
            for (size_t j = 0; j < input_count; ++j) {
                if (input_values[j]) ort->ReleaseValue(input_values[j]);
                if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
            }
            for (size_t j = 0; j < output_count; ++j) {
                if (output_values[j]) ort->ReleaseValue(output_values[j]);
                if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
            }
            RC2D_safe_free(input_values);
            RC2D_safe_free(input_names);
            RC2D_safe_free(output_values);
            RC2D_safe_free(output_names);
            ort->ReleaseMemoryInfo(memory_info);
            return false;
        }
    }

    // Vérifie si les noms des OrtValue* ont été créés correctement (input/output names)
    for (size_t i = 0; i < input_count; ++i)
    {
        if (input_names[i] == NULL)
        {
            RC2D_log(RC2D_LOG_CRITICAL, "Failed to create name for input %zu", i);
            for (size_t j = 0; j < input_count; ++j) {
                if (input_values[j]) ort->ReleaseValue(input_values[j]);
                if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
            }
            for (size_t j = 0; j < output_count; ++j) {
                if (output_values[j]) ort->ReleaseValue(output_values[j]);
                if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
            }
            RC2D_safe_free(input_values);
            RC2D_safe_free(input_names);
            RC2D_safe_free(output_values);
            RC2D_safe_free(output_names);
            ort->ReleaseMemoryInfo(memory_info);
            return false;
        }
    }
    for (size_t i = 0; i < output_count; ++i)
    {
        if (output_names[i] == NULL)
        {
            RC2D_log(RC2D_LOG_CRITICAL, "Failed to create name for output %zu", i);
            for (size_t j = 0; j < input_count; ++j) {
                if (input_values[j]) ort->ReleaseValue(input_values[j]);
                if (input_names[j]) allocator->Free(allocator, (void*)input_names[j]);
            }
            for (size_t j = 0; j < output_count; ++j) {
                if (output_values[j]) ort->ReleaseValue(output_values[j]);
                if (output_names[j]) allocator->Free(allocator, (void*)output_names[j]);
            }
            RC2D_safe_free(input_values);
            RC2D_safe_free(input_names);
            RC2D_safe_free(output_values);
            RC2D_safe_free(output_names);
            ort->ReleaseMemoryInfo(memory_info);
            return false;
        }
    }

    /**
     * Lance l’inférence avec :
     * - la session modèle (`model->session`)
     * - les entrées (noms + OrtValue*)
     * - les sorties attendues (noms + OrtValue* à remplir)
     * 
     * Pour un batch dynamique, traite N inférences simultanément
     * (par exemple, [150, 3] pour 150 PNJ avec [x, y, santé]).
     */
    OrtStatus* status = ort->Run(model->session, NULL, input_names, input_values, input_count,
                                 output_names, output_count, output_values);

    /**
     * Vérifie si une erreur est survenue pendant l’inférence
     */
    if (status != NULL)
    {
        OrtErrorCode err_code = ort->GetErrorCode(status);
        const char* err_msg = ort->GetErrorMessage(status);
        RC2D_log(RC2D_LOG_CRITICAL, "ONNX inference failed: Code=%d, Message=%s", err_code, err_msg);
        ort->ReleaseStatus(status);
        for (size_t i = 0; i < input_count; ++i) {
            if (input_values[i]) ort->ReleaseValue(input_values[i]);
            if (input_names[i]) allocator->Free(allocator, (void*)input_names[i]);
        }
        for (size_t i = 0; i < output_count; ++i) {
            if (output_values[i]) ort->ReleaseValue(output_values[i]);
            if (output_names[i]) allocator->Free(allocator, (void*)output_names[i]);
        }
        RC2D_safe_free(input_values);
        RC2D_safe_free(input_names);
        RC2D_safe_free(output_values);
        RC2D_safe_free(output_names);
        ort->ReleaseMemoryInfo(memory_info);
        return false;
    }
    else
    {
        RC2D_log(RC2D_LOG_INFO, "ONNX inference succeeded");
        ort->ReleaseStatus(status);
    }

    /**
     * Récupère les résultats de l’inférence dans les OrtValue* de sortie
     * et les copie dans les buffers utilisateurs (outputs[i].data).
     *
     * Pour les types scalaires (float, int, bool, etc.), on copie directement
     * les données. Pour un batch dynamique, le buffer contient N * M éléments
     * (par exemple, 150 * 3 floats pour [150, 3] avec scores [left, right, jump] pour 150 PNJ).
     *
     * Pour les chaînes de caractères, on doit d’abord récupérer la longueur
     * de chaque chaîne, puis allouer un buffer pour la stocker.
     */
    for (size_t i = 0; i < output_count; ++i)
    {
        if (outputs[i].type == ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING)
        {
            size_t count = rc2d_onnx_computeElementCount(outputs[i].shape, outputs[i].dims);
            char** dest = (char**)outputs[i].data;

            for (size_t j = 0; j < count; ++j)
            {
                size_t len = 0;
                ort->GetStringTensorElementLength(output_values[i], j, &len);

                dest[j] = RC2D_malloc(len + 1); // Pour ajouter un \0 manuellement si besoin
                ort->GetStringTensorElement(output_values[i], len, j, dest[j]);

                // Ajoute un nul terminal pour compatibilité C si nécessaire
                dest[j][len] = '\0';
            }
        }
        else
        {
            // Copie les données scalaires depuis le tenseur vers le buffer utilisateur
            void* src = NULL;
            ort->GetTensorMutableData(output_values[i], &src);

            size_t total_size = rc2d_onnx_getTypeSize(outputs[i].type) *
                                rc2d_onnx_computeElementCount(outputs[i].shape, outputs[i].dims);

            SDL_memcpy(outputs[i].data, src, total_size);
        }
    }

    /**
     * Libère toutes les ressources utilisées :
     * - les OrtValue* (inputs / outputs)
     * - les noms alloués dynamiquement
     * - les tableaux RC2D_malloc
     * - l'objet memory_info
     */
    for (size_t i = 0; i < input_count; ++i)
    {
        ort->ReleaseValue(input_values[i]);
        allocator->Free(allocator, (void*)input_names[i]);
    }
    for (size_t i = 0; i < output_count; ++i)
    {
        ort->ReleaseValue(output_values[i]);
        allocator->Free(allocator, (void*)output_names[i]);
    }
    RC2D_safe_free(input_values);
    RC2D_safe_free(input_names);
    RC2D_safe_free(output_values);
    RC2D_safe_free(output_names);
    ort->ReleaseMemoryInfo(memory_info);

    // Succès
    return true;
}

void rc2d_onnx_freeTensors(RC2D_OnnxTensor* tensors, size_t count)
{
    if (!tensors) return;

    for (size_t i = 0; i < count; ++i)
    {
        RC2D_OnnxTensor* tensor = &tensors[i];

        if (!tensor->data) continue;

        if (tensor->type == ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING)
        {
            size_t element_count = rc2d_onnx_computeElementCount(tensor->shape, tensor->dims);
            char** strings = (char**)tensor->data;

            for (size_t j = 0; j < element_count; ++j)
            {
                if (strings[j]) {
                    RC2D_safe_free(strings[j]);
                    strings[j] = NULL;
                }
            }
        }

        // Nettoie complètement la structure pour éviter toute réutilisation incorrecte
        tensor->name = NULL;
        tensor->data = NULL;
        tensor->type = ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED;
        tensor->dims = 0;
        SDL_memset(tensor->shape, 0, sizeof(tensor->shape));
    }
}

#endif // RC2D_ONNX_MODULE_ENABLED