#ifndef RC2D_ONNX_H
#define RC2D_ONNX_H

#if RC2D_ONNX_MODULE_ENABLED

#include <onnxruntime_c_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Structure représentant un modèle ONNX chargé en mémoire.
 *
 * \since Disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_OnnxModel {
    /**
     * Chemin relatif vers le fichier ONNX à charger, par exemple : "models-onnx/my_model.onnx"
     * 
     * \note Le path doit être rempli par l'utilisateur avant d'appeler rc2d_onnx_loadModel().
     */
    const char* path;

    /**
     * Session ONNX Runtime créée à partir du modèle ONNX, spécifié par la propriété path.
     * 
     * Cette session peut être utilisée plusieurs fois pour effectuer des inférences sans avoir
     * à recharger le modèle depuis le disque.
     * 
     * \note La session sera remplie par la fonction rc2d_onnx_loadModel().
     */
    OrtSession* session;

    /**
     * Indique si le modèle utilise des formes dynamiques pour la dimension de batch.
     * 
     * Si true, la première dimension des tenseurs d'entrée et de sortie peut varier
     * (par exemple, [N, 3] où N est le nombre de PNJ pour lesquels on effectue une inférence).
     * Si false, la forme est statique (par exemple, [1, 3] pour une seule inférence).
     * 
     * \note Doit être défini par l'utilisateur avant d'appeler rc2d_onnx_loadModel().
     *       Utilisez true pour des inférences sur plusieurs PNJ simultanément, comme dans un batch
     *       de 150 PNJ avec des états [x, y, santé] pour optimiser les performances.
     */
    bool dynamic_batch;
} RC2D_OnnxModel;

/**
 * \brief Représente un tenseur d’entrée ou de sortie utilisé dans une inférence ONNX.
 *
 * Cette structure est utilisée à la fois pour :
 *   - fournir des **entrées** au modèle (inputs),
 *   - récupérer les **résultats** d’inférence (outputs).
 *
 * === Cas des ENTRÉES (inputs) ===
 * --------------------------------
 * Tous les champs de la structure doivent être renseignés **manuellement** :
 *
 * - `name`  : Nom exact de l’entrée attendue par le modèle ONNX (sensible à la casse).
 *             Il doit correspondre exactement à ce que le modèle a défini.
 *
 * - `data`  : Pointeur vers les données brutes à fournir au modèle.
 *             Cela peut être un tableau de `float`, `int32_t`, `bool`, ou un tableau de `char*` pour les chaînes.
 *             Pour un batch dynamique, le buffer doit être dimensionné pour N * M éléments
 *             (par exemple, N * 3 floats pour un tenseur [N, 3] représentant N PNJ avec [x, y, santé]).
 *
 * - `type`  : Type des éléments ONNX (ex : `ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT`, `..._STRING`, etc.).
 *
 * - `shape` : Tableau d'entiers 64 bits définissant la forme du tenseur (ex : `{N, 3}` pour un batch de N PNJ).
 *
 * - `dims`  : Nombre de dimensions (ex : `2` pour `{N, 3}`).
 *
 * ❗ En cas d’erreur de type, de taille, ou de shape, le modèle peut échouer à l'exécution.
 *
 *
 * === Cas des SORTIES (outputs) ===
 * ---------------------------------
 * Seuls les champs suivants doivent être spécifiés par l'utilisateur :
 *
 * - `name`  : **Nom logique** de la sortie, utilisé uniquement pour vous repérer dans votre tableau `outputs[]`.
 *             Il n’a pas besoin de correspondre exactement au nom du modèle, car le runtime utilise l’ordre.
 *             Il permet simplement d’identifier les éléments plus facilement dans le code (`outputs[0]`, `outputs[1]`, etc.).
 *
 * - `data`  : 
 *   - Si le type est scalaire (`FLOAT`, `INT32`, `BOOL`, etc.) :
 *     le buffer pointé par `data` doit être **pré-alloué** avec une taille suffisante
 *     (par exemple, N * M floats pour un tenseur [N, M], comme N * 3 pour les scores d'action de N PNJ).
 *   - Si le type est `STRING`, `data` doit être un tableau de `char*` initialisé,
 *     et chaque `char*` sera automatiquement alloué après l’inférence.
 *
 * **Les champs suivants sont automatiquement remplis après `rc2d_onnx_run()` :**
 *
 * - `type`  : Détecté dynamiquement à partir du modèle.
 * - `shape` : Déterminée automatiquement via `GetDimensions()`. Pour un batch dynamique,
 *             `shape[0]` sera égal à l'entrée `shape[0]` (par exemple, N pour [N, 3]).
 * - `dims`  : Rempli à partir du nombre de dimensions du modèle.
 *
 *
 * === Exemples ===
 *
 * ➤ **Entrée float pour un PNJ (statique) :**
 * ```c
 * float my_input[3] = {0.5f, 0.5f, 0.2f}; // x, y, santé pour un PNJ
 * RC2D_OnnxTensor input = {
 *     .name = "player_state",
 *     .data = my_input,
 *     .type = ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
 *     .shape = {1, 3},
 *     .dims = 2
 * };
 * ```
 *
 * ➤ **Entrée float pour 150 PNJ (dynamique) :**
 * ```c
 * float npc_states[150 * 3]; // 150 PNJ, chaque état = [x, y, santé]
 * // Remplir npc_states avec les états des PNJ, par exemple :
 * // npc_states[0] = 0.5f; npc_states[1] = 0.5f; npc_states[2] = 0.2f; // PNJ 1
 * // npc_states[3] = 0.1f; npc_states[4] = 0.9f; npc_states[5] = 0.8f; // PNJ 2
 * // ...
 * RC2D_OnnxTensor input = {
 *     .name = "player_state",
 *     .data = npc_states,
 *     .type = ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
 *     .shape = {150, 3}, // 150 PNJ, 3 valeurs par PNJ
 *     .dims = 2
 * };
 * ```
 *
 * ➤ **Entrée string :**
 * ```c
 * const char* my_strings[] = {"hello", "world"};
 * RC2D_OnnxTensor input = {
 *     .name = "input_tensor_string",
 *     .data = (void*)my_strings,
 *     .type = ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING,
 *     .shape = {1, 2},
 *     .dims = 2
 * };
 * ```
 *
 * ➤ **Sortie float pour un PNJ (statique) :**
 * ```c
 * float scores[3]; // Scores pour left, right, jump
 * RC2D_OnnxTensor output = {
 *     .name = "action_logits",
 *     .data = scores // Doit être alloué
 *     // type / shape / dims seront remplis par rc2d_onnx_run()
 * };
 * ```
 *
 * ➤ **Sortie float pour 150 PNJ (dynamique) :**
 * ```c
 * float npc_actions[150 * 3]; // Scores pour 150 PNJ, chaque PNJ a [left, right, jump]
 * RC2D_OnnxTensor output = {
 *     .name = "action_logits",
 *     .data = npc_actions // Doit être alloué
 *     // type / shape / dims seront remplis par rc2d_onnx_run()
 * };
 * // Après rc2d_onnx_run(), npc_actions contient :
 * // npc_actions[0..2] = scores pour PNJ 1
 * // npc_actions[3..5] = scores pour PNJ 2
 * // ...
 * ```
 *
 * ➤ **Sortie string :**
 * ```c
 * char* labels[2]; // Non alloués, mais initialisés (pointeurs nuls ok)
 * RC2D_OnnxTensor output = {
 *     .name = "output_labels",
 *     .data = labels
 *     // type / shape / dims seront remplis par rc2d_onnx_run()
 * };
 * // Après l’appel, chaque labels[i] est alloué avec RC2D_malloc
 * ```
 *
 * \since Disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_OnnxTensor {
    const char* name;                         ///< Nom de l’entrée ou de la sortie (obligatoire pour s’y retrouver dans le code)
    void* data;                               ///< Pointeur vers les données (entrée ou sortie)
    ONNXTensorElementDataType type;           ///< Type ONNX (FLOAT, INT32, BOOL, STRING, etc.)
    int64_t shape[8];                         ///< Shape du tensor (calculé automatiquement pour les sorties)
    size_t dims;                              ///< Nombre de dimensions (détecté automatiquement pour les sorties)
} RC2D_OnnxTensor;

/**
 * \brief Charge un modèle ONNX et initialise une session à partir d’un chemin.
 *
 * \param model Pointeur vers la structure à remplir.
 * \return true en cas de succès, false sinon.
 * 
 * \warning La session allouée dans `model->session` doit être libérée par l'appelant 
 * avec `rc2d_onnx_unloadModel` lorsque le modèle n'est plus nécessaire.
 *
 * \since Disponible depuis RC2D 1.0.0.
 */
bool rc2d_onnx_loadModel(RC2D_OnnxModel* model);

/**
 * \brief Libère la session ONNX associée à un modèle précédemment chargé.
 *
 * \param model Le modèle dont la session doit être libérée.
 *
 * \since Disponible depuis RC2D 1.0.0.
 */
void rc2d_onnx_unloadModel(RC2D_OnnxModel* model);

/**
 * \brief Lance une inférence avec un modèle ONNX et un tableau d’entrées/sorties.
 *
 * \param model Modèle ONNX chargé.
 * \param inputs Tableau d’entrées.
 * \param outputs Tableau de sorties.
 *
 * \return true si l’inférence a réussi, false sinon.
 *
 * \note Si model->dynamic_batch est true, la première dimension des tenseurs peut varier
 *       (par exemple, [N, 3] pour N PNJ, où chaque PNJ a un état [x, y, santé]).
 *       Les buffers d'entrée et de sortie doivent être dimensionnés pour N * M éléments
 *       (par exemple, N * 3 floats pour [N, 3]).
 * 
 * \warning Pour les tenseurs de sortie de type STRING, les chaînes allouées dans outputs[i].data 
 * doivent être libérées par l'appelant avec rc2d_onnx_freeTensors après utilisation.
 *
 * \since Disponible depuis RC2D 1.0.0.
 */
bool rc2d_onnx_run(RC2D_OnnxModel* model, RC2D_OnnxTensor* inputs, RC2D_OnnxTensor* outputs);

/**
 * \brief Libère les ressources associées à un ou plusieurs tenseurs ONNX.
 *
 * Cette fonction permet de libérer proprement les données associées à une ou plusieurs
 * structures `RC2D_OnnxTensor`, en particulier lorsqu’il s’agit de sorties de type `STRING`.
 *
 * Elle s’utilise typiquement après un appel à `rc2d_onnx_run()` afin de nettoyer les
 * chaînes allouées dynamiquement par le moteur ONNX Runtime dans les sorties de type string.
 *
 * ⚠️ Pour les types scalaires (FLOAT, INT, BOOL...), le buffer pointé par `tensor->data`
 * n’est **pas** libéré (il appartient à l’utilisateur).  
 * En revanche, pour les types `STRING`, chaque `char*` est libéré via `RC2D_safe_free()`.
 *
 * Tous les champs de la structure sont ensuite réinitialisés à zéro.
 *
 * \param tensors Tableau de `RC2D_OnnxTensor` à libérer (ou pointeur vers un seul tenseur)
 * \param count Nombre de tenseurs à libérer (1 pour une structure unique)
 *
 * === Exemple : sortie scalaire ===
 * ```c
 * float result[10];
 * RC2D_OnnxTensor out = {
 *     .name = "output",
 *     .data = result
 * };
 * rc2d_onnx_run(&model, inputs, &out);
 * rc2d_onnx_freeTensors(&out, 1); // OK, ne libère pas result
 * ```
 *
 * === Exemple : sortie string ===
 * ```c
 * char* labels[2];
 * RC2D_OnnxTensor out = {
 *     .name = "labels",
 *     .data = labels
 * };
 * rc2d_onnx_run(&model, inputs, &out);
 * rc2d_onnx_freeTensors(&out, 1); // Libère labels[0], labels[1], puis nettoie la structure
 * ```
 *
 * \since Disponible depuis RC2D 1.0.0.
 */
void rc2d_onnx_freeTensors(RC2D_OnnxTensor* tensors, size_t count);

#ifdef __cplusplus
}
#endif

#endif // RC2D_ONNX_MODULE_ENABLED
#endif // RC2D_ONNX_H