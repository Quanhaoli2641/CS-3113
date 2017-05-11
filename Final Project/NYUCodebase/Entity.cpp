//
//  Entity.cpp
//  NYUCodebase
//
//  Created by Quanhao Li on 5/3/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#include <stdio.h>
#include "Entity.h"
#include <math.h>


float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

void Entity::Render(ShaderProgram *p, Matrix& m, float bounceVal) {
    m.identity();
    m.Translate(this->position.x, this->position.y, 0);
    if (entityType == ENTITY_ENEMY) {
        m.Scale(fabs(0.25*sin(bounceVal*4)) + 0.75, fabs(0.25*sin(bounceVal*4)) + 0.75, 1);
    }
    
    if(name == "player1") m.Scale(1, 2, 1);
    if(name == "player2") m.Scale(1, 2, 1);
    if (name == "bullet") m.Scale(0.75, 1.5, 1);
    p->setModelMatrix(m);
    if (name!= "player1" && name!= "player2") this->sprite.Draw(p);
    else {
        DrawSpriteSheetSprite(p, state, 4, 4);
    }
}

void Entity::Update(float elapsed) {
    if (!this->isStatic) {
        
        this->velocity.x = lerp(this->velocity.x, 0.0f, elapsed * this->friction.x);
        this->velocity.y = lerp(this->velocity.y, 0.0f, elapsed * this->friction.y);
        
        this->velocity.x += this->acceleration.x * elapsed;
        this->velocity.y += this->acceleration.y * elapsed;
        
        this->position.x += this->velocity.x * elapsed;
        this->position.y += this->velocity.y * elapsed;
        
        if (!isStatic && life <= 0) {
            if(entityType == ENTITY_PLAYER){
                position.x = 1000;
                position.y = 1000;
            }
            if(entityType == ENTITY_ENEMY){
                position.x = 500;
                position.y = 500;
            }
            
        }
        
    }
}

void Entity::DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY) {
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    GLfloat texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f,
        -0.5f, 0.5f, -0.5f};
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, *spriteSheet);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

bool Entity::collidesWith(Entity *entity) {
    if ((position.y - sprite.size/2) > (entity->position.y + entity->sprite.size/2)) {
        return false;
    }
    else if ((position.y + sprite.size/2) < (entity->position.y - entity->sprite.size/2)) {
        return false;
    }
    else if ((position.x - sprite.size/2) > (entity->position.x + entity->sprite.size/2)) {
        return false;
    }
    else if ((position.x + sprite.size/2) < (entity->position.x - entity->sprite.size/2)) {
        return false;
    }
    return true;
}

void Entity::handleCollision(Entity *entity){
    float xDiff = position.x - entity->position.x;
    float yDiff = position.y - entity->position.y;
    if((entityType == ENTITY_PLAYER && entity->entityType == ENTITY_COLLIDE) ||
       (entityType == ENTITY_ENEMY && entity->entityType == ENTITY_ENEMY) ||
       (entityType == ENTITY_PLAYER && entity->entityType == ENTITY_PLAYER)){
        
        if(xDiff > 0){
            if (entityType == ENTITY_PLAYER) {
                velocity.x = 0;
                velocity.y = 0;
            }
            position.x += xDiff * 0.2f;
        }
        if(yDiff > 0){
            if (entityType == ENTITY_PLAYER) {
                velocity.x = 0;
                velocity.y = 0;
            }
            position.y += yDiff* 0.2f;
        }
        if(xDiff < 0){
            if (entityType == ENTITY_PLAYER) {
                velocity.x = 0;
                velocity.y = 0;
            }
            position.x += xDiff* 0.2f;
        }
        if(yDiff < 0){
            if (entityType == ENTITY_PLAYER) {
                velocity.x = 0;
                velocity.y = 0;
            }
            position.y += yDiff* 0.2f;
        }
    }
    if(entityType == ENTITY_PLAYER && entity->entityType == ENTITY_ENEMY){
        life--;
        entity->position.x = 500;
        entity->position.y = 500;
    }
    if(entityType == ENTITY_BULLET && entity->entityType == ENTITY_ENEMY){
        position.x = 600;
        position.y = 600;
        velocity.x = 0;
        velocity.y = 0;
        acceleration.x = 0;
        acceleration.y = 0;
        entity->life -= 1;
    }
    
    
}







