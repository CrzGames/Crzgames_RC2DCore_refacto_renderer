#ifndef ENTITY_H
#define ENTITY_H

typedef enum Direction {
    DIRECTION_NORTH_EAST = 0, /**< Vers le haut-droit.  */
    DIRECTION_NORTH_WEST,     /**< Vers le haut-gauche. */
    DIRECTION_SOUTH_EAST,     /**< Vers le bas-droit.   */
    DIRECTION_SOUTH_WEST      /**< Vers le bas-gauche.  */
} Direction;

typedef enum EntityType {
    ENTITY_PLAYER = 0, /**< Joueur. */
    ENTITY_NPC,        /**< Personnage non-joueur. */
    ENTITY_SCINTILLE,  /**< Scintillement. */
    ENTITY_MONSTER,    /**< Monstre. */
    ENTITY_TOWER       /**< Tour. */
} EntityType;

typedef struct Entity {
    int id;              /**< Identifiant unique (ID du joueur, ID du NPC, etc.). */
    EntityType type;     /**< Type de l'entité. */
    Direction direction; /**< Direction de l'entité. */
    float x;             /**< Position en pixels dans la carte. */
    float y;             /**< Position en pixels dans la carte. */
} Entity;

#endif // ENTITY_H