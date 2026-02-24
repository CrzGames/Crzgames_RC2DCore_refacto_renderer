get_filename_component(RC2D_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(VENDORED_DIR "${RC2D_ROOT}/dependencies")
set(DEPENDENCIES_FILE "${RC2D_ROOT}/dependencies.txt")

# Créer le répertoire s'il n'existe pas
file(MAKE_DIRECTORY ${VENDORED_DIR})

# Lire le contenu du fichier dependencies.txt
file(READ ${DEPENDENCIES_FILE} DEPENDENCIES_CONTENT)

# Normaliser les fins de lignes pour compatibilité multiplateforme
string(REPLACE "\r\n" "\n" DEPENDENCIES_CONTENT "${DEPENDENCIES_CONTENT}")
string(REPLACE "\r" "\n" DEPENDENCIES_CONTENT "${DEPENDENCIES_CONTENT}")
string(REPLACE "\n" ";" DEPENDENCIES_LINES "${DEPENDENCIES_CONTENT}")

# Parcourir chaque ligne de dépendance
foreach(LINE IN LISTS DEPENDENCIES_LINES)
  string(STRIP "${LINE}" LINE)

  # Ignorer les lignes vides ou commentaires
  if(LINE STREQUAL "" OR LINE MATCHES "^#.*")
    continue()
  endif()

  # Extraire le nom de la lib
  string(REGEX MATCH "^[^=]+" LIB_NAME "${LINE}")
  # Extraire le repository et le tag/commit
  string(REGEX REPLACE "^[^=]+=" "" VALUE_PART "${LINE}")
  string(REGEX MATCH "^(.*):([^:]*)$" _ "${VALUE_PART}")

  if(NOT _)
    message(WARNING "❌ Ligne de dépendance invalide: '${LINE}'")
    continue()
  endif()

  set(LIB_REPO "${CMAKE_MATCH_1}")
  set(LIB_REF "${CMAKE_MATCH_2}")
  set(CLONE_DIR "${VENDORED_DIR}/${LIB_NAME}")

  # Cloner le dépôt si non présent
  if(NOT EXISTS "${CLONE_DIR}/.git")
    message(STATUS "📥 Clonage ${LIB_NAME} depuis ${LIB_REPO} @ ${LIB_REF}")
    execute_process(COMMAND git clone ${LIB_REPO} ${CLONE_DIR})
  endif()

  # S'assurer d'avoir toutes les refs distantes
  execute_process(COMMAND git fetch origin
                  WORKING_DIRECTORY ${CLONE_DIR})
  execute_process(COMMAND git fetch --all
                  WORKING_DIRECTORY ${CLONE_DIR})

  # Revenir proprement à la version souhaitée (tag ou commit SHA)
  execute_process(COMMAND git reset --hard ${LIB_REF}
                  WORKING_DIRECTORY ${CLONE_DIR})

  # Mettre à jour les éventuels sous-modules
  execute_process(COMMAND git submodule update --init --recursive
                  WORKING_DIRECTORY ${CLONE_DIR})
endforeach()
