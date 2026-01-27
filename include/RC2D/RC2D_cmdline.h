#ifndef RC2D_CMDLINE_H
#define RC2D_CMDLINE_H

/* Configuration pour les définitions de fonctions C, même en C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct RC2D_CommandLine
{
    int argc;
    char** argv;        // copie profonde (N strings)
} RC2D_CommandLine;

/**
 * \brief Retourne le nombre d'arguments de la ligne de commande.
 */
int rc2d_cmdline_getArgc(void);

/**
 * \brief Retourne un argument de la ligne de commande.
 *
 * \param i Index de l'argument (0 = nom de l'exécutable)
 * \return Pointeur constant vers l'argument, ou NULL si index invalide
 */
const char* rc2d_cmdline_getArgv(int i);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_CMDLINE_H