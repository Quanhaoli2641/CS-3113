//
//  SpriteSheet.h
//  NYUCodebase
//
//  Created by Quanhao Li on 5/5/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#ifndef SpriteSheet_h
#define SpriteSheet_h

#include "ShaderProgram.h"

class SpriteSheet {
public:
    SpriteSheet(){};
    SpriteSheet(unsigned int textureID, float u, float v, float width, float height, float
                size) : textureID (textureID), u (u), v(v), width (width), height (height), size (size){};
    
    void Draw(ShaderProgram* program);
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
};

#endif /* SpriteSheet_h */
