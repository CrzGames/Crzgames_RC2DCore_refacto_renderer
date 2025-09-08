#ifndef RC2D_FILESYSTEM_H
#define RC2D_FILESYSTEM_H

#include <RC2D/RC2D_data.h> // Required for : RC2D_DataType

#include <SDL3/SDL_stdinc.h> // Required for : Sint64

#include <stdbool.h> // Required for : bool

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumération des résultats possibles des opérations du système de fichiers.
 * Fournit des codes de retour spécifiques pour différentes conditions d'erreur ou de succès.
 * @typedef {enum} RC2D_FileSystemResult
 * @property {number} RC2D_FS_SUCCESS - Opération réussie sans erreurs.
 * @property {number} RC2D_FS_ERROR_FILE_NOT_FOUND - Le fichier spécifié n'a pas été trouvé.
 * @property {number} RC2D_FS_ERROR_CANNOT_OPEN - Le fichier spécifié ne peut pas être ouvert.
 * @property {number} RC2D_FS_ERROR_CLOSE_FAIL - Échec de la fermeture du fichier spécifié.
 * @property {number} RC2D_FS_ERROR_WRITE_FAIL - Échec de l'écriture dans le fichier spécifié.
 * @property {number} RC2D_FS_ERROR_READ_FAIL - Échec de la lecture du fichier spécifié.
 * @property {number} RC2D_FS_ERROR_PARTIAL_READ - Seule une partie du fichier a été lue, lecture incomplète.
 * @property {number} RC2D_FS_ERROR_UNKNOWN - Une erreur inconnue s'est produite lors de l'opération.
 */
typedef enum RC2D_FileSystemResult {
    RC2D_FS_SUCCESS,               
    RC2D_FS_ERROR_FILE_NOT_FOUND,  
    RC2D_FS_ERROR_CANNOT_OPEN,
    RC2D_FS_ERROR_CLOSE_FAIL,     
    RC2D_FS_ERROR_WRITE_FAIL,     
    RC2D_FS_ERROR_READ_FAIL,       
    RC2D_FS_ERROR_PARTIAL_READ,    
    RC2D_FS_ERROR_UNKNOWN          
} RC2D_FileSystemResult;

/**
 * Obtenez le chemin absolu vers un répertoire spécifique à l'utilisateur et à l'application
 * où l'écriture de fichiers est autorisée. Ce chemin est idéal pour stocker les données 
 * persistantes telles que les configurations de l'application, les sauvegardes de jeu,
 * et tout autre fichier nécessaire au fonctionnement de l'application.
 * 
 * Ce chemin est le seul endroit recommandé pour écrire des données sur le disque afin
 * d'assurer la compatibilité entre différentes plateformes et de respecter les directives
 * d'accès aux fichiers des systèmes d'exploitation.
 * 
 * Android : /data/data/com.crzgames.rc2d/files/
 * Windows : C:\Users\CrzGames\AppData\Roaming\CrzGames\SeaTyrants\
 * macOS : /Users/CrzGames/Library/Application Support/CrzGames/SeaTyrants/
 * Linux : /home/debian/.local/share/CrzGames/SeaTyrants/
 * iOS : /var/mobile/Containers/Data/Application/5B7B1F4D-5F1D-4F1C-9F1D-5F1D4F1C9F1D/Library/Application Support/CrzGames/SeaTyrants/
 * HTML5 : /libsdl/CrzGames/SeaTyrants/
 * 
 * @param nameOrganisation Le nom de l'organisation sous laquelle classer les données de l'application.
 * @param nameApplication Le nom de l'application pour laquelle obtenir le chemin.
 * @return Un pointeur vers une chaîne contenant le chemin absolu recommandé pour l'écriture de données,
 *         ou NULL en cas d'erreur ou si la plateforme n'implémente pas cette fonctionnalité.
 */
char* rc2d_filesystem_getWritableAppDataPath(const char* nameOrganisation, const char* nameApplication);

/*
Obtenez le path absolu a partir duquel l'application a ete executee.

Android : pointer NULL (toute les ressources doit être dans le dossier app/src/main/assets du dossier android-project, le path commence a partir de la directement donc le path sera relative directement a partir de cette emplacement)
Windows (.exe) : C:\Users\Corentin\Documents\SeaTyrants\bin\
macOS (.app bundle) : /Applications/SeaTyrants/SeaTyrants.app/Contents/Resources/
iOS (.app bundle) : /private/var/containers/Bundle/Application/5E3A643E-4353-44CD-85A1-690E286A944B/SeaTyrants.app/
Linux : /tmp/.mount_mygame0q6J0j/ (une AppImage est un format d empaquetage qui, lors de son execution, monte le contenu de l image dans un systeme de fichiers temporaire. Cela explique pourquoi vous obtenez un chemin dans /tmp/.)
HTML5 : /
*/
char* rc2d_filesystem_getPathApp(void); 

/*
Les assets original doit être placer dans le dossier 'assets' du projet pour les plateformes Windows, Linux, macOS et iOS.
Pour Android, les assets original doit être placer dans le dossier app/src/main/assets du dossier android-project.

Build assets original in RRES file format (script), output example:
- RRES: CDIR: Entry id: 0xb90bce3a | Offset: 0x00334c26 | Filename: ./assets/images/test.png (len: 28)

On calculera le dossier 'assets' au préalable pour que ce sois relatif au dossier assets pour chaque plateforme.

Cela ne concerne que le module rc2d_rres.
*/
char* rc2d_filesystem_getPathAssetsInResourceRRES(void);

#ifdef __cplusplus
}
#endif

#endif // RC2D_FILESYSTEM_H