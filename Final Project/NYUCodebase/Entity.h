//
//  Entity.h
//  NYUCodebase
//
//  Created by Quanhao Li on 5/3/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#ifndef Entity_h
#define Entity_h

#include "ShaderProgram.h"
#include "Matrix.h"
#include "SpriteSheet.h"
#include "Vector.h"

enum EntityType {ENTITY_PLAYER, ENTITY_ENEMY,
    ENTITY_COLLIDE, ENTITY_BULLET};

class Entity {
public:
    
    Entity():state(0), friction(Vector(15.0f,15.0f)) {}
    void Update(float elapsed);
    void Render(ShaderProgram *program, Matrix& modelMatrix, float bounceVal = 0);
    bool collidesWith(Entity *entity);
    void handleCollision(Entity *entity);
    void DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX,
                               int spriteCountY);
    
    
    SpriteSheet sprite;
    GLuint* spriteSheet;
    
    Vector position;
    Vector size;
    Vector velocity;
    Vector acceleration;
    Vector friction;
    
    bool isStatic;
    
    EntityType entityType;
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
    
    std::string name;
    int state;
    int life;
    
};

#endif /* Entity_h */
