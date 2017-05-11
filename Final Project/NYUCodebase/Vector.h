//
//  Vector.h
//  NYUCodebase
//
//  Created by Quanhao Li on 5/3/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#ifndef Vector_h
#define Vector_h

class Vector {
public:
    float x;
    float y;
    float z;
    
    Vector():x(0), y(0), z(0){};
    Vector(float x, float y):x(x), y(y), z(0){};
    float length();
    void normalize();
};


#endif /* Vector_h */
