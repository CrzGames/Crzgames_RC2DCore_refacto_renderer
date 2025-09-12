#ifndef RC2D_GPU_H
#define RC2D_GPU_H

#include <RC2D/RC2D_storage.h>

#include <SDL3/SDL_gpu.h>

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Contexte GPU utilisé par RC2D.
 *
 * Ce type wrappe SDL_GPUDevice, le contexte GPU principal fourni par SDL3.
 *
 * \since Ce type est disponible depuis RC2D 1.0.0.
 */
typedef struct SDL_GPUDevice RC2D_GPUDevice;

/**
 * \brief Shader graphique utilisé par RC2D.
 * 
 * Ce type est un wrapper autour de SDL_GPUShader, utilisé pour les shaders graphiques.
 * 
 * \since Ce type est disponible depuis RC2D 1.0.0.
 */
typedef struct SDL_GPUShader RC2D_GPUShader;

/**
 * \brief Pilotes GPU disponibles pour RC2D.
 *
 * Ce type permet à l'utilisateur de forcer un backend GPU spécifique à l'initialisation.
 * RC2D se base sur les backends supportés par SDL3_GPU : `vulkan`, `metal`, `direct3d12`.
 * 
 * \note Concernant RC2D_GPU_DRIVER_PRIVATE, il s'agit d'un backend propriétaire par rapport à la plateforme de développement.
 * Tels que pour la Playstation 5, la Xbox Series X/S, Nintendo Switch 1/2, etc.
 * 
 * \since Cette enum est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_GPUDriver {
    /**
     * Laisser RC2D choisir automatiquement le meilleur backend GPU.
     * 
     * Cela permet sous le capot de RC2D à SDL de sélectionner le backend le plus adapté à la plateforme,
     * en tenant compte des capacités du matériel et des préférences de l'utilisateur.
     */
    RC2D_GPU_DRIVER_DEFAULT = 0,

    /**
     * Forcer l'utilisation de Vulkan comme backend GPU.
     * 
     * Cela peut être utile pour les développeurs qui souhaitent tirer parti des
     * fonctionnalités avancées de Vulkan ou qui ont besoin d'une compatibilité spécifique.
     */
    RC2D_GPU_DRIVER_VULKAN,

    /**
     * Forcer l'utilisation de Metal comme backend GPU.
     * 
     * Cela est particulièrement pertinent pour les applications macOS et iOS,
     * où Metal est le backend graphique natif recommandé.
     */
    RC2D_GPU_DRIVER_METAL,

    /**
     * Forcer l'utilisation de Direct3D12 comme backend GPU.
     * 
     * Cela est pertinent pour les applications Windows qui souhaitent tirer parti
     * des fonctionnalités avancées de Direct3D12.
     */
    RC2D_GPU_DRIVER_DIRECT3D12,

    /**
     * Forcer l'utilisation d'un backend GPU privée sous NDA (Non Disclosure Agreement).
     * 
     * Par rapport à la plateforme de développement, cela peut être un backend propriétaire ou un
     * backend expérimental qui n'est pas encore public.
     */
    RC2D_GPU_DRIVER_PRIVATE
} RC2D_GPUDriver;

/**
 * \brief Formats de shaders supportés par le GPU.
 *
 * Utilisé pour indiquer quels formats de shaders sont disponibles selon le backend.
 * 
 * \note Peut être combiné avec un bitmask.
 *
 * \since Cette enum est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_GPUShaderFormat {
    /**
     * Aucun format de shader n'est supporté.
     */
    RC2D_GPU_SHADERFORMAT_NONE = 0,

    /**
     * Pour les formats de shaders privés (NDA).
     * 
     * Utilisé pour les backends propriétaires ou expérimentaux.
     */
    RC2D_GPU_SHADERFORMAT_PRIVATE = 1 << 0,

    /**
     * SPIR-V (Vulkan) est supporté.
     * 
     * Utilisé pour les shaders Vulkan.
     */
    RC2D_GPU_SHADERFORMAT_SPIRV = 1 << 1,

    /**
     * DXBC (Direct3D11) est supporté.
     * 
     * Utilisé pour les shaders Direct3D11.
     */
    RC2D_GPU_SHADERFORMAT_DXBC = 1 << 2,

    /**
     * DXIL (Direct3D12) est supporté.
     * 
     * Utilisé pour les shaders Direct3D12.
     */
    RC2D_GPU_SHADERFORMAT_DXIL = 1 << 3,

    /**
     * MSL (Metal) est supporté.
     * 
     * Utilisé pour les shaders Metal sur macOS, iOS et tvOS.
     */
    RC2D_GPU_SHADERFORMAT_MSL = 1 << 4,

    /**
     * MetalLib (Metal) est supporté.
     * 
     * Utilisé pour les shaders Metal précompilés sur macOS, iOS et tvOS.
     */
    RC2D_GPU_SHADERFORMAT_METALLIB = 1 << 5
} RC2D_GPUShaderFormat;

/**
 * \brief Nombre maximal d’images autorisées en vol sur le GPU.
 *
 * Permet de choisir le comportement de la file d’attente GPU en ajustant le 
 * nombre maximal d’images « en vol » (c’est-à-dire que le CPU peut soumettre 
 * avant que le GPU ait fini de les afficher).
 * 
 * \note Cela influence directement la **latence visuelle**, le **débit de rendu** et la **compatibilité 
 * avec les petites configurations**. RC2D_GPU_FRAMES_BALANCED est le choix par défaut.
 * 
 * \since Cette enum est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_GPUFramesInFlight {
    /**
     * 1 image en vol : latence minimale, mais risque de baisse de framerate sur GPU lents.
     * - Idéal pour les jeux pixel art ou à forte précision de contrôle.
     * - Réduit la latence visuelle au maximum (le rendu est très réactif).
     * - Recommandé pour les petites configurations (PCs peu puissants, intégrés, etc.).
     * - Peut réduire le framerate si le GPU est souvent à la traîne.
     */
    RC2D_GPU_FRAMES_LOW_LATENCY = 1,

    /**
     * 2 images en vol : bon équilibre entre réactivité et performance.
     * - Bon équilibre entre réactivité et performance.
     * - Convient à la plupart des machines modernes.
     * - Recommandé pour la majorité des jeux 2D.
     */
    RC2D_GPU_FRAMES_BALANCED = 2,

    /**
     * 3 images en vol : latence légèrement augmentée, mais débit GPU maximisé.
     * - Maximise le débit GPU, utile pour les animations lourdes ou les effets complexes.
     * - Légère augmentation de la latence visuelle.
     * - Recommandé pour des jeux fluides, cinématiques, ou avec beaucoup de post-traitement.
     */
    RC2D_GPU_FRAMES_HIGH_THROUGHPUT = 3
} RC2D_GPUFramesInFlight;

/**
 * \brief Options avancées pour la création du contexte GPU.
 * 
 * Permet de configurer certains comportements internes lors de l’initialisation
 * du GPU via SDL3. Ces paramètres influencent la manière dont SDL configure
 * le périphérique graphique sous-jacent (Vulkan, Metal et Direct3D12).
 * 
 * \note RC2D active automatiquement tous les formats de shaders supportés par la plateforme cible
 * (SPIR-V, DXBC, DXIL, MSL, etc.), ce qui permet à l’utilisateur de ne fournir
 * que les formats dont il a besoin sans configuration supplémentaire.
 * 
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_GPUAdvancedOptions {
    /**
     * Active le mode debug GPU.
     * 
     * Cela permet à SDL d’activer des couches de validation supplémentaires 
     * pour capturer les erreurs GPU pendant le développement.
     * 
     * \note True par défaut dans RC2D.
     */
    bool debugMode;

    /**
     * Active les logs détaillés pendant la création du contexte GPU.
     * 
     * Cela permet de voir les capacités GPU détectées, les formats supportés, etc.
     * 
     * \note True par défaut dans RC2D.
     */
    bool verbose;

    /**
     * Demande à SDL de privilégier un GPU à faible consommation si disponible.
     * 
     * Cela peut être utile pour les appareils mobiles ou les ordinateurs portables
     * où l'économie d'énergie est une priorité.
     * 
     * \note False par défaut dans RC2D.
     */
    bool preferLowPower;

    /**
     * Permet de forcer un backend graphique spécifique (Vulkan, Metal, Direct3D12 ou un backend privé).
     * 
     * \note RC2D_GPU_DRIVER_DEFAULT par défaut dans RC2D, ce qui laisse SDL choisir automatiquement.
     */
    RC2D_GPUDriver driver;
} RC2D_GPUAdvancedOptions;

/**
 * \brief Remplit une structure RC2D_GPUInfo avec les métadonnées du GPU utilisé.
 * 
 * \param {RC2D_GPUDevice*} gpuDevice - Le périphérique GPU SDL initialisé par RC2D.
 * \param {RC2D_GPUInfo*} gpuInfo - Pointeur vers la structure à remplir.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
//void rc2d_gpu_getInfo(RC2D_GPUInfo* gpuInfo);

/**
 * \brief Récupère le périphérique GPU utilisé par RC2D.
 * 
 * \return {RC2D_GPUDevice*} Pointeur vers le périphérique GPU SDL utilisé par RC2D.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
RC2D_GPUDevice* rc2d_gpu_getDevice(void);

/**
 * \brief Récupère les formats de shaders supportés par le GPU actuel.
 *
 * \return {RC2D_GPUShaderFormat} Bitmask RC2D_GPUShaderFormat contenant les formats supportés.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
RC2D_GPUShaderFormat rc2d_gpu_getSupportedShaderFormats(void);

/**
 * \brief Charge un shader graphique à partir d'un fichier source HLSL ou d'un fichier binaire précompilé.
 * 
 * Le chemin passé à la fonction ne doit contenir que le "nom logique" du shader, avec son suffixe de stage
 * (exemple : "water.vertex", "water.fragment"). Il est inutile et incorrect d'inclure "shaders/src" ou 
 * "shaders/compiled" dans ce chemin : la fonction les ajoute automatiquement.
 * 
 * Selon la configuration :
 * - Si RC2D_GPU_SHADER_HOT_RELOAD_ENABLED = 1 :
 *   → le shader est compilé à la volée depuis le dossier `shaders/src/` 
 *     (fichiers attendus : `<nom>.<stage>.hlsl`).
 * - Sinon (mode compilation hors ligne) :
 *   → le shader est chargé depuis le dossier `shaders/compiled/<backend>/` 
 *     (fichiers attendus : `<nom>.<stage>.<ext>` selon le backend, par ex. `.spv`, `.dxil`, `.msl`, `.metallib`).
 *   → un fichier JSON de réflexion doit également être présent dans `shaders/reflection/` 
 *     (fichier attendu : `<nom>.<stage>.json`).
 * 
 * Exemple d’organisation attendue dans le storage (TITLE ou USER) :
 * ```
 * <racine_storage>/
 *   shaders/
 *     src/          # sources HLSL (*.vertex.hlsl, *.fragment.hlsl)
 *     compiled/     # binaires précompilés par backend
 *       spirv/      # *.spv
 *       msl/        # *.msl
 *       metallib/   # *.metallib
 *       dxil/       # *.dxil
 *     reflection/   # métadonnées JSON (*.json)
 * ```
 * 
 * \param {const char*} filename - Nom logique du shader à charger avec suffixe de stage
 *                                (ex. "water.vertex", "water.fragment").
 * \param {RC2D_StorageKind} storage_kind - Type de storage à utiliser (TITLE ou USER).
 * \return {RC2D_GPUShader*} Pointeur vers le shader chargé, ou NULL en cas d'erreur.
 * 
 * \warning Le pointeur retourné doit être libéré par l'appelant avec SDL_ReleaseGPUShader
 *          lorsque le shader n'est plus nécessaire.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
RC2D_GPUShader* rc2d_gpu_loadGraphicsShaderFromStorage(const char* filename,
                                                       RC2D_StorageKind storage_kind);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_GPU_H