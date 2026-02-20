#include <RC2D/RC2D_cmdline.h>

#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_memory.h>

void rc2d_cmdline_store(int argc, char* argv[])
{
    // Initialisation des valeurs par défaut (vides)
    rc2d_engine_state.cmdline.argc = 0;
    rc2d_engine_state.cmdline.argv = NULL;

    // Check si il y a au moins un argument SINON on ne fait rien
    if (argc <= 0) return;

    // Alloue la mémoire pour les arguments
    rc2d_engine_state.cmdline.argv = RC2D_malloc(sizeof(char*) * argc);

    // Si l'allocation a échoué, on quitte
    if (!rc2d_engine_state.cmdline.argv) return;

    // Copie profonde des arguments dans la structure
    for (int i = 0; i < argc; i++)
    {
        rc2d_engine_state.cmdline.argv[i] = RC2D_strdup(argv[i] ? argv[i] : "");
    }

    // Stocke le nombre d'arguments dans la structure
    rc2d_engine_state.cmdline.argc = argc;
}

void rc2d_cmdline_free(void)
{
    // Libère chaque argument individuellement
    for (int i = 0; i < rc2d_engine_state.cmdline.argc; i++)
    {
        RC2D_free(rc2d_engine_state.cmdline.argv[i]);
    }

    // Libère le tableau des arguments
    RC2D_free(rc2d_engine_state.cmdline.argv);

    // Réinitialise les valeurs à NULL/0
    rc2d_engine_state.cmdline.argv = NULL;
    rc2d_engine_state.cmdline.argc = 0;
}

// ============================================================
// API publique lecture seule
// ============================================================

int rc2d_cmdline_getArgc(void)
{
    return rc2d_engine_state.cmdline.argc;
}

const char* rc2d_cmdline_getArgv(int i)
{
    if (i < 0 || i >= rc2d_engine_state.cmdline.argc)
        return NULL;

    return rc2d_engine_state.cmdline.argv[i];
}