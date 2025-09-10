#ifndef RC2D_STORAGE_H
#define RC2D_STORAGE_H

#include <SDL3/SDL_stdinc.h> // Required for : Uint64

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Ouvre le conteneur de stockage "Title" (lecture seule, assets packagés).
 *
 * \details Cette fonction wrappe SDL_OpenTitleStorage(). Si 'override_path' est NULL,
 * l’implémentation générique utilise SDL_GetBasePath() comme racine. Les chemins à
 * utiliser avec ce storage doivent **toujours** être en séparateurs Unix ('/'), sans
 * segments relatifs ("." ou ".."), conformément à l’API Storage SDL.
 *
 * \param override_path Chemin de substitution pour la racine du titre, ou NULL.
 * \return true si le conteneur a été ouvert.
 *
 * \threadsafety À appeler depuis le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_openTitle(const char *override_path);

/**
 * \brief Ouvre le conteneur de stockage "User" (lecture/écriture unique d’un utilisateur).
 *
 * \details Wrap de SDL_OpenUserStorage(org, app, props=0). Par bonnes pratiques SDL,
 * ouvrez ce storage uniquement au moment de lire/écrire des données utilisateur (saves,
 * options, etc.), puis fermez-le après les opérations pour permettre au backend de
 * batcher et de flusher proprement.
 *
 * \param org Nom de l’organisation (ex: "CrzGames").
 * \param app Nom de l’application (ex: "SeaTyrants").
 * \return true si le conteneur a été ouvert, false sinon.
 *
 * \threadsafety À appeler depuis le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_openUser(const char *org, const char *app);

/**
 * \brief Indique si le storage "Title" est prêt.
 *
 * \details Wrap de SDL_StorageReady() sur le handle interne "Title".
 *
 * \return true si prêt, false sinon ou si non ouvert.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_titleReady(void);

/**
 * \brief Indique si le storage "User" est prêt.
 *
 * \details Wrap de SDL_StorageReady() sur le handle interne "User".
 *
 * \return true si prêt, false sinon ou si non ouvert.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_userReady(void);

/**
 * \brief Crée un répertoire dans le storage "User".
 *
 * \details Wrap de SDL_CreateStorageDirectory(user, path). Le 'path' utilise
 * des séparateurs '/' et ne doit pas contenir de segments relatifs. La création
 * récursive n’est pas garantie selon backend.
 *
 * \param path Chemin (style Unix) du dossier à créer dans le storage user.
 * \return true en cas de succès, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_userMkdir(const char *path);

/**
 * \brief Lit entièrement un fichier depuis le storage "Title" dans un buffer alloué.
 *
 * \details Cette fonction interroge d’abord la taille via SDL_GetStorageFileSize(),
 * alloue via SDL_malloc() puis appelle SDL_ReadStorageFile(). À la réussite, \p *out_data
 * contient un buffer à libérer avec SDL_free() et \p *out_len la taille lue.
 *
 * \param path Chemin (style Unix) du fichier à lire depuis le storage title.
 * \param out_data [out] Reçoit un pointeur alloué via RC2D_malloc (à libérer par l’appelant).
 * \param out_len  [out] Reçoit la taille du fichier.
 * \return true si lecture réussie, false sinon
 *
 * \warning Le pointeur out_data doit être libéré par l’appelant avec `RC2D_safe_free`
 * lorsque les données ne sont plus nécessaires.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_titleReadFile(const char *path, void **out_data, Uint64 *out_len);

/**
 * \brief Lit entièrement un fichier depuis le storage "User" dans un buffer alloué.
 *
 * \details Même contrat que rc2d_storage_titleReadFile(), mais sur le storage user.
 *
 * \param path Chemin (style Unix) du fichier à lire.
 * \param out_data [out] Buffer alloué via RC2D_malloc (à libérer par l’appelant).
 * \param out_len  [out] Taille lue.
 * \return true si lecture réussie, false sinon.
 *
 * \warning Le pointeur out_data doit être libéré par l’appelant avec `RC2D_safe_free` 
 * lorsque les données ne sont plus nécessaires.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_userReadFile(const char *path, void **out_data, Uint64 *out_len);

/**
 * \brief Écrit un buffer en entier dans un fichier du storage "User".
 *
 * \details Wrap direct de SDL_WriteStorageFile(user, path, src, len). Le backend
 * garantit la cohérence à la fermeture du storage (pensez à close après vos batchs).
 *
 * \param path Chemin (style Unix) de destination dans le storage user.
 * \param src  Pointeur vers les données sources en mémoire.
 * \param len  Longueur des données.
 * \return true si écriture réussie, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_storage_userWriteFile(const char *path, const void *src, Uint64 len);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif /* RC2D_STORAGE_H */
