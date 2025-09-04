#include <RC2D/RC2D_logger.h>

#include <SDL3/SDL_log.h>

/**
 * Définities le niveau de log par défaut à RC2D_LOG_DEBUG
 * Cela peut être modifié par l'utilisateur via la fonction : rc2d_log_set_priority()
 */
static RC2D_LogLevel currentLogLevel = RC2D_LOG_DEBUG;

/**
 * Convertit le niveau de log RC2D en chaîne de caractères.
 *
 * @param {RC2D_LogLevel} level - Le niveau de log à convertir.
 * @return {const char*} - La chaîne de caractères représentant le niveau de log.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static const char* rc2d_logger_log_level_to_string(RC2D_LogLevel level) 
{
    switch (level) 
    {
        case RC2D_LOG_TRACE:     return "trace";
        case RC2D_LOG_VERBOSE:   return "verbose";
        case RC2D_LOG_DEBUG:     return "debug";
        case RC2D_LOG_INFO:      return "info";
        case RC2D_LOG_WARN:      return "warn";
        case RC2D_LOG_ERROR:     return "error";
        case RC2D_LOG_CRITICAL:  return "critical";
        default:                 return "unknown";
    }
}

RC2D_LogLevel rc2d_logger_get_priority(void)
{
    return currentLogLevel;
}

void rc2d_logger_set_priority(const RC2D_LogLevel logLevel) 
{
    // Enregistre le niveau de log actuel
    currentLogLevel = logLevel;

    // Convertit le niveau de log de RC2D en priorite SDL correspondante.
    SDL_LogPriority logPriority;
    switch(logLevel) 
    {
        case RC2D_LOG_TRACE:     logPriority = SDL_LOG_PRIORITY_TRACE;     break;
        case RC2D_LOG_VERBOSE:   logPriority = SDL_LOG_PRIORITY_VERBOSE;   break;
        case RC2D_LOG_DEBUG:     logPriority = SDL_LOG_PRIORITY_DEBUG;     break;
        case RC2D_LOG_INFO:      logPriority = SDL_LOG_PRIORITY_INFO;      break;
        case RC2D_LOG_WARN:      logPriority = SDL_LOG_PRIORITY_WARN;      break;
        case RC2D_LOG_ERROR:     logPriority = SDL_LOG_PRIORITY_ERROR;     break;
        case RC2D_LOG_CRITICAL:  logPriority = SDL_LOG_PRIORITY_CRITICAL;  break;
        default:                 logPriority = SDL_LOG_PRIORITY_INFO;       break;
    }

    // Definit la priorite de log pour toutes les categories
    SDL_SetLogPriorities(logPriority);
}

void rc2d_logger_log(const RC2D_LogLevel logLevel, const char* file, int line, const char* function, const char* format, ...)
{
    /**
     * Vérifie si le niveau de log est inférieur au niveau actuel
     * Si oui, on ne fait rien
     */
    if (logLevel < currentLogLevel) return;

    // Convertit le niveau de log de RC2D en priorite SDL correspondante.
    SDL_LogPriority sdlPriority;
    switch (logLevel) 
    {
        case RC2D_LOG_TRACE:     sdlPriority = SDL_LOG_PRIORITY_TRACE;     break;
        case RC2D_LOG_VERBOSE:   sdlPriority = SDL_LOG_PRIORITY_VERBOSE;   break;
        case RC2D_LOG_DEBUG:     sdlPriority = SDL_LOG_PRIORITY_DEBUG;     break;
        case RC2D_LOG_INFO:      sdlPriority = SDL_LOG_PRIORITY_INFO;      break;
        case RC2D_LOG_WARN:      sdlPriority = SDL_LOG_PRIORITY_WARN;      break;
        case RC2D_LOG_ERROR:     sdlPriority = SDL_LOG_PRIORITY_ERROR;     break;
        case RC2D_LOG_CRITICAL:  sdlPriority = SDL_LOG_PRIORITY_CRITICAL;  break;
        default:                 sdlPriority = SDL_LOG_PRIORITY_INFO;       break;
    }

    // Convertit le niveau de log de RC2D en chaîne de caractères
    const char* levelStr = rc2d_logger_log_level_to_string(logLevel);

    // Préparer le message de log
    const char* filename = SDL_strrchr(file, '/');
    if (!filename) filename = SDL_strrchr(file, '\\');
    filename = filename ? filename + 1 : file;

    char prefix[256];
    SDL_snprintf(prefix, sizeof(prefix), "[%s:%s:%d:%s] ", levelStr, filename, line, function);

    char message[1024];
    va_list args;
    va_start(args, format);
    SDL_vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    char fullMessage[1280];
    SDL_snprintf(fullMessage, sizeof(fullMessage), "%s%s", prefix, message);

    // Utiliser SDL_LogMessageV pour les plateformes natives
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, sdlPriority, "%s", fullMessage);
}