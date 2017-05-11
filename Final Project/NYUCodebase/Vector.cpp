//
//  Vector.cpp
//  NYUCodebase
//
//  Created by Quanhao Li on 5/3/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#include <stdio.h>
#include "Vector.h"
#include "math.h"

float Vector::length() {
    
    return sqrtf(x*x + y*y + z*z);
    
}

void Vector::normalize() {
    
    x /= length();
    y /= length();
    z /= length();
    
}
