#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include "Matrix.h"
#include "ShaderProgram.h"
#include "Vector.h"
#include "sat.h"
#include "Entity.h"
#include <vector>

SDL_Window* displayWindow;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

using namespace std;

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    SDL_Event event;
    bool done = false;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glViewport(0, 0, 640, 360);

    ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    
    Matrix projectionMatrix;
    Matrix viewMatrix;
    Matrix modelMatrix;
    
    projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
    
    float lastFrameTicks = 0.0f;
    
    float vertex[] = {0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f};
    
    Entity e1(modelMatrix, Vector(0,0), Vector(1,1), 0);
    Entity e2(modelMatrix, Vector(0,0), Vector(1,1), 0);
    Entity e3(modelMatrix, Vector(0,0), Vector(1,1), 0);
    
    glUseProgram(program.programID);
    program.setViewMatrix(viewMatrix);
    program.setProjectionMatrix(projectionMatrix);
    glClear(GL_COLOR_BUFFER_BIT);
    
    modelMatrix.identity();
    modelMatrix.Rotate(45*M_PI/180);
    modelMatrix.Translate(1.0f, 1.0f, 0);
    modelMatrix.Scale(0.5f, 2.0f, 0);
    program.setModelMatrix(modelMatrix);
    e1.Draw(program, vertex);
    e1.matrix = modelMatrix;
    e1.scale.x = 0.5f;
    e1.scale.y = 2.0f;
    e1.position.x += 1;
    e1.position.y += 1;
    e1.rotation += 45;
    e1.acceleration_x = -0.5f;
    e1.acceleration_y = -0.5f;
    e1.vertices.push_back(Vector(-0.5, -0.5));
    e1.vertices.push_back(Vector(0.5, -0.5));
    e1.vertices.push_back(Vector(0.5, 0.5));
    e1.vertices.push_back(Vector(-0.5, 0.5));
    vector<Vector> e1WCoord (4);
    
    modelMatrix.identity();
    modelMatrix.Translate(-1.0f, -1.0f, 0);
    modelMatrix.Rotate(60*M_PI/180);
    program.setModelMatrix(modelMatrix);
    modelMatrix.Scale(1.5f, 1.5f, 0);
    e2.Draw(program, vertex);
    e2.matrix = modelMatrix;
    e2.scale.x = 1.5f;
    e2.scale.y = 1.5f;
    e2.position.x -= 1;
    e2.position.y -= 1;
    e2.rotation += 60;
    e2.acceleration_x = 2.0f;
    e2.acceleration_y = 0.5f;
    e2.vertices.push_back(Vector(-0.5, -0.5));
    e2.vertices.push_back(Vector(0.5, -0.5));
    e2.vertices.push_back(Vector(0.5, 0.5));
    e2.vertices.push_back(Vector(-0.5, 0.5));
    vector<Vector> e2WCoord (4);
    
    modelMatrix.identity();
    modelMatrix.Scale(2.0f, 0.5f, 0);
    modelMatrix.Translate(1, -1, 0);
    modelMatrix.Rotate(30*M_PI/180);
    program.setModelMatrix(modelMatrix);
    e3.Draw(program, vertex);
    e3.matrix = modelMatrix;
    e3.position.x += 1;
    e3.position.y -= 1;
    e3.rotation += 30;
    e3.scale.x = 2.0f;
    e3.scale.y = 0.5f;
    e3.acceleration_y = 2.0f;
    e3.vertices.push_back(Vector(-0.5, -0.5));
    e3.vertices.push_back(Vector(0.5, -0.5));
    e3.vertices.push_back(Vector(0.5, 0.5));
    e3.vertices.push_back(Vector(-0.5, 0.5));
    vector<Vector> e3WCoord (4);
    
    SAT s;
    Vector pen;

    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glUseProgram(program.programID);
        program.setViewMatrix(viewMatrix);
        program.setProjectionMatrix(projectionMatrix);
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        for (int i = 0; i < 4; i++) {
            e1WCoord[i] = e1.matrix*e1.vertices[i];
            e2WCoord[i] = e2.matrix*e2.vertices[i];
            e3WCoord[i] = e3.matrix*e3.vertices[i];
        }
        
        
        float fixedElapsed = elapsed;
        if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
            fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
        }
        while (fixedElapsed >= FIXED_TIMESTEP ) {
            fixedElapsed -= FIXED_TIMESTEP;
            e1.Update(FIXED_TIMESTEP);
            e2.Update(FIXED_TIMESTEP);
            e3.Update(FIXED_TIMESTEP);
        }
        
        e1.Update(FIXED_TIMESTEP);
        e2.Update(FIXED_TIMESTEP);
        e3.Update(FIXED_TIMESTEP);
        
        Vector pos1, pos2;
        
        if (s.checkSATCollision(e1WCoord, e2WCoord, pen)) {
            pos1 = e1.matrix*Vector(0,0);
            pos2 = e2.matrix*Vector(0,0);
            if (pos1.x < pos2.x) {
                e1.position.x -= pen.x;
                e2.position.x += pen.x;
            }
            else {
                e1.position.x += pen.x;
                e2.position.x -= pen.x;
            }
            if (pos1.y < pos2.y) {
                e1.position.y -= pen.y;
                e2.position.y += pen.y;
            }
            else {
                e1.position.y += pen.y;
                e2.position.y -= pen.y;
            }
            e1.acceleration_x = 0.0f;
            e1.acceleration_y = 0.0f;
            e1.velocity_x = 0;
            e1.velocity_y = 0;
            e2.acceleration_x = -0;
            e2.acceleration_y = -0.0f;
            e2.velocity_x = 0;
            e2.velocity_y = 0;
            
        }
        if (s.checkSATCollision(e1WCoord, e3WCoord, pen)) {
            pos1 = e1.matrix*Vector(0,0);
            pos2 = e3.matrix*Vector(0,0);
            if (pos1.x < pos2.x) {
                e1.position.x -= pen.x;
                e3.position.x += pen.x;
            }
            else {
                e1.position.x += pen.x;
                e3.position.x -= pen.x;
            }
            if (pos1.y < pos2.y) {
                e1.position.y -= pen.y;
                e3.position.y += pen.y;
            }
            else {
                e1.position.y += pen.y;
                e3.position.y -= pen.y;
            }
            e1.acceleration_x = 0.0f;
            e1.acceleration_y = 0.0f;
            e1.velocity_x = 0;
            e1.velocity_y = 0;
            e3.acceleration_y =0;
            e3.velocity_y = 0;
        }
        if (s.checkSATCollision(e2WCoord, e3WCoord, pen)) {
            pos1 = e2.matrix*Vector(0,0);
            pos2 = e3.matrix*Vector(0,0);
            if (pos1.x < pos2.x) {
                e2.position.x -= pen.x;
                e3.position.x += pen.x;
            }
            else {
                e2.position.x += pen.x;
                e3.position.x -= pen.x;
            }
            if (pos1.y < pos2.y) {
                e2.position.y -= pen.y;
                e3.position.y += pen.y;
            }
            else {
                e2.position.y += pen.y;
                e3.position.y -= pen.y;
            }
            e2.acceleration_x = 0;
            e2.acceleration_y = 0;
            e2.velocity_x = 0;
            e2.velocity_y = 0;
            e3.acceleration_y = 0;
            e3.velocity_y = 0;
        }
        
        e1.Render(program, vertex);
        e2.Render(program, vertex);
        e3.Render(program, vertex);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
