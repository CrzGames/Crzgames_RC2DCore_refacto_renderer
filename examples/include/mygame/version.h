#ifndef GAME_H
#define GAME_H

/**
 * \brief Version majeure de la bibliothèque RC2D.
 * 
 * \threadsafety Il est possible d'appeler cette macro de préprocesseur en toute sécurité à partir de n'importe quel thread.
 * 
 * \since Cette macro est disponible depuis RC2D 1.0.0.
 * 
 * \see GAME_VERSION_MINOR
 * \see GAME_VERSION_PATCH
 */
#define GAME_VERSION_MAJOR 1

/**
 * \brief Version mineure de la bibliothèque RC2D.
 * 
 * \threadsafety Il est possible d'appeler cette macro de préprocesseur en toute sécurité à partir de n'importe quel thread.
 *
 * \since Cette macro est disponible depuis RC2D 1.0.0.
 * 
 * \see GAME_VERSION_MAJOR
 * \see GAME_VERSION_PATCH
 */
#define GAME_VERSION_MINOR 0

/**
 * \brief Version de correction de la bibliothèque RC2D.
 * 
 * \threadsafety Il est possible d'appeler cette macro de préprocesseur en toute sécurité à partir de n'importe quel thread.
 *
 * \since Cette macro est disponible depuis RC2D 1.0.0.
 * 
 * \see GAME_VERSION_MAJOR
 * \see GAME_VERSION_MINOR
 */
#define GAME_VERSION_PATCH 0

#endif // GAME_H