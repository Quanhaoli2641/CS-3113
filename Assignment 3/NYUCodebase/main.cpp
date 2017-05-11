#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include "ShaderProgram.h"
#include "Matrix.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "entity.h"
#include <vector>
#include <SDL_mixer.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL};
int state;
Mix_Chunk* someSound = nullptr;
Mix_Music* music = nullptr;

using namespace std;


void DrawText(ShaderProgram *program, GLuint fontTexture, string text, float size, float spacing) {
    float texture_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    for(int i=0; i < text.size(); i++) {
        float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
        float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size * 2,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size *2,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size *2,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size*2,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size*2,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size*2,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        });
    }
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);

    
}

GLuint LoadTexture(const char *filePath) {
    
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return retTexture;
}


// Checks collision between two Entity objects
bool checkCollision (Entity& a, Entity& b) {
    if ((a.Pos.y - a.size/2) > (b.Pos.y + b.size/2)) {
        return true;
    }
    if ((a.Pos.y + a.size/2) < (b.Pos.y - b.size/2)) {
        return true;
    }
    if ((a.Pos.x - a.size/2) > (b.Pos.x + b.size/2)) {
        return true;
    }
    if ((a.Pos.x + a.size/2) < (b.Pos.x - b.size/2)) {
        return true;
    }
    
    return false;
}

// Stores an enemy entity into a vector data structure
void storeEnemy (vector<Entity*>* e, SpriteSheet* enemySprite) {
    Vector3 v (-0.02f, 0.0f, 0.0f);
    Entity* enemy = new Entity(enemySprite, &v);
    e->push_back(enemy);
}

bool checkAllDeleted (vector<vector<Entity*>>& enemies) {
    for (int i = 0; i < enemies.size(); i++) {
        for (int j = 0; j < enemies[i].size(); j++) {
            if (enemies[i][j] != nullptr) return false;
        }
    }
    return true;
}

// Stores all enemies into a vector data structure
// Calls store enemy to complete action
void storeEnemyComplete (GLuint spriteSheetTexture, vector<vector<Entity*>>& enemies) {
    
    SpriteSheet* enemyBlack1 = new SpriteSheet(spriteSheetTexture, 423.0f/1024.0f, 728.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlack2 = new SpriteSheet(spriteSheetTexture, 120.0f/1024.0f, 604.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlack3 = new SpriteSheet(spriteSheetTexture, 144.0f/1024.0f, 156.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlack4 = new SpriteSheet(spriteSheetTexture, 518.0f/1024.0f, 325.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlack5 = new SpriteSheet(spriteSheetTexture, 346.0f/1024.0f, 150.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlue1 = new SpriteSheet(spriteSheetTexture, 425.0f/1024.0f, 468.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlue2 = new SpriteSheet(spriteSheetTexture, 143.0f/1024.0f, 293.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlue3 = new SpriteSheet(spriteSheetTexture, 222.0f/1024.0f, 0.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlue4 = new SpriteSheet(spriteSheetTexture, 518.0f/1024.0f, 409.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyBlue5 = new SpriteSheet(spriteSheetTexture, 421.0f/1024.0f, 814.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyGreen1 = new SpriteSheet(spriteSheetTexture, 425.0f/1024.0f, 552.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyGreen2 = new SpriteSheet(spriteSheetTexture, 133.0f/1024.0f, 412.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyGreen3 = new SpriteSheet(spriteSheetTexture, 224.0f/1024.0f, 496.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyGreen4 = new SpriteSheet(spriteSheetTexture, 518.0f/1024.0f, 493.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyGreen5 = new SpriteSheet(spriteSheetTexture, 408.0f/1024.0f, 907.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyRed1 = new SpriteSheet(spriteSheetTexture, 425.0f/1024.0f, 384.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyRed2 = new SpriteSheet(spriteSheetTexture, 120.0f/1024.0f, 520.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyRed3 = new SpriteSheet(spriteSheetTexture, 224.0f/1024.0f, 580.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyRed4 = new SpriteSheet(spriteSheetTexture, 520.0f/1024.0f, 577.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    SpriteSheet* enemyRed5 = new SpriteSheet(spriteSheetTexture, 423.0f/1024.0f, 644.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 0.3f);
    
    storeEnemy(&enemies[1], enemyBlack1);
    storeEnemy(&enemies[1], enemyBlack2);
    storeEnemy(&enemies[1], enemyBlack3);
    storeEnemy(&enemies[1], enemyBlack4);
    storeEnemy(&enemies[1], enemyBlack5);
    storeEnemy(&enemies[2], enemyBlue1);
    storeEnemy(&enemies[2], enemyBlue2);
    storeEnemy(&enemies[2], enemyBlue3);
    storeEnemy(&enemies[2], enemyBlue4);
    storeEnemy(&enemies[2], enemyBlue5);
    storeEnemy(&enemies[3], enemyGreen1);
    storeEnemy(&enemies[3], enemyGreen2);
    storeEnemy(&enemies[3], enemyGreen3);
    storeEnemy(&enemies[3], enemyGreen4);
    storeEnemy(&enemies[3], enemyGreen5);
    storeEnemy(&enemies[4], enemyRed1);
    storeEnemy(&enemies[4], enemyRed2);
    storeEnemy(&enemies[4], enemyRed3);
    storeEnemy(&enemies[4], enemyRed4);
    storeEnemy(&enemies[4], enemyRed5);
    
}

// Fires a bullet from the location of an entity
int shootBullet (Entity** bullets, int currBullet, float gap, float y_velocity, Entity* other, Matrix& modelMatrix, ShaderProgram* program) {
    bullets[currBullet]->Pos.x = other->Pos.x;
    bullets[currBullet]->Pos.y = other->Pos.y+gap;
    modelMatrix.identity();
    modelMatrix.Translate(bullets[currBullet]->Pos.x - bullets[currBullet]->init_Pos.x, bullets[currBullet]->Pos.y - bullets[currBullet]->init_Pos.y, 0);
    program->setModelMatrix(modelMatrix);
    bullets[currBullet]->sprite.Draw(program);
    bullets[currBullet]->velocity.y = y_velocity;
    if (currBullet == 29) currBullet = 0;
    else currBullet+=1;

    return currBullet;
}

// Randomly fires from all bottom enemies
int enemyFire (float elapsed, vector<vector<Entity*>>& enemies, Entity** bullets, int currBullet, Matrix& modelMatrix, ShaderProgram* program) {
        
        for (int i = 0; i < 5; i++) {
            int ran = rand() % 100;
            int n = 4;
            while (n > 0 && enemies[n][i] == nullptr) {
                n--;
            }
            if (n <= 0) continue;
            if (ran <= 10){
                currBullet = shootBullet(bullets, currBullet, -0.3, -4.0f, enemies[n][i], modelMatrix, program);
                
                someSound = Mix_LoadWAV(RESOURCE_FOLDER"Enemy_Shoot.wav");
                Mix_PlayChannel(-1, someSound, 0);
            }
        }
    return currBullet;
    
}

// Setup of Game
void setUpGame (GLuint spriteSheetTexture, Matrix& modelMatrix, ShaderProgram* program, Entity* p,Entity** bullets, vector<vector<Entity*>>& enemies, SpriteSheet* bullet, float init_x, float curr_y, float gap) {
    
    // sets up storage of enemies in a container
    storeEnemyComplete(spriteSheetTexture, enemies);
    
    // sets up player location
    modelMatrix.identity();
    modelMatrix.Translate(0.0f, -1.6f, 0.0f);
    p->init_Pos.x = modelMatrix.m[3][0];
    p->init_Pos.y = modelMatrix.m[3][1];
    p->init_Pos.z = modelMatrix.m[3][2];
    p->Pos.x = modelMatrix.m[3][0];
    p->Pos.y = modelMatrix.m[3][1];
    p->Pos.z = modelMatrix.m[3][2];
    program->setModelMatrix(modelMatrix);
    p->sprite.Draw(program);

    // sets up location of bullets
    for (int i = 0; i < 30; i++) {
        Entity* b = new Entity(bullet, new Vector3(0.0f,0.0f,0.0f), new Vector3 (5.0f, 0.0f, 0.0f));
        modelMatrix.identity();
        modelMatrix.Translate(20.0f, 0.0f, 0.0f);
        program->setModelMatrix(modelMatrix);
        b->sprite.Draw(program);
        bullets[i] = b;
    }
    
    // sets up location of enemies
    for (int i = 0; i < enemies.size(); i++) {
        for (int j = 0; j < enemies[i].size(); j++) {
            modelMatrix.identity();
            if (j == 0) {
                modelMatrix.Translate(init_x, curr_y - enemies[i][j]->sprite.height/2, 0);
            }
            else {
                modelMatrix.Translate(enemies[i][j-1]->Pos.x + gap, curr_y, 0);
            }
            enemies[i][j]->Pos.x = modelMatrix.m[3][0];
            enemies[i][j]->Pos.y = modelMatrix.m[3][1];
            program->setModelMatrix(modelMatrix);
            enemies[i][j]->sprite.Draw(program);
        }
        curr_y -= 0.5f;
    }
}

void update (SDL_Event* event, ShaderProgram* program, Matrix& modelMatrix, Matrix& viewMatrix, Matrix& projectionMatrix, bool done);

void updateMenuScreen (SDL_Event* event, ShaderProgram* program, Matrix& modelMatrix, Matrix& viewMatrix, Matrix& projectionMatrix, bool done, GLuint fonts) {
    while (!done) {
        while (SDL_PollEvent(event)) {
            if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
            else if (event->type == SDL_KEYUP) {
                if (event->key.keysym.scancode == SDL_SCANCODE_S) {
                    state = STATE_GAME_LEVEL;
                    done = true;
                    update(event, program, modelMatrix, viewMatrix, projectionMatrix, false);
                }
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(program->programID);
        
        program->setViewMatrix(viewMatrix);
        program->setProjectionMatrix(projectionMatrix);
        modelMatrix.identity();
        modelMatrix.Translate(-1.65, 0.5, 0);
        program->setModelMatrix(modelMatrix);
        DrawText(program, fonts, "Hello World!" , 0.3f, 0);
        
        program->setViewMatrix(viewMatrix);
        program->setProjectionMatrix(projectionMatrix);
        modelMatrix.identity();
        modelMatrix.Translate(-2, -0.5, 0);
        program->setModelMatrix(modelMatrix);
        DrawText(program, fonts, "Press 's' to Start!" , 0.2f, 0);
        
        SDL_GL_SwapWindow(displayWindow);
        
    }
}

void updateGame (SDL_Event* event, ShaderProgram* program, Matrix& modelMatrix, Matrix& viewMatrix, Matrix& projectionMatrix, bool done, Entity* p, Entity** bullets, vector<vector<Entity*>>& enemies, float lastFrameTicks, float curr_y, float init_y, GLuint fontTexture) {
    
    int currBullet = 0;
    float amtTime = 0;
    int score = 0;
    
    
    Mix_Chunk *someSound = nullptr;
    
    music = Mix_LoadMUS(RESOURCE_FOLDER"My Heart Will Go On - Recorder By Candlelight by Matt Mulholland.mp3");
    Mix_PlayMusic(music, -1);
    
    while (!done) {
        while (SDL_PollEvent(event)) {
            if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
            else if (event->type == SDL_KEYUP) {
                if (p != nullptr && event->key.keysym.scancode == SDL_SCANCODE_SPACE) {
                    currBullet = shootBullet(bullets, currBullet, 0.3f, 2.0f, p, modelMatrix, program);
                    someSound = Mix_LoadWAV(RESOURCE_FOLDER"Laser_Shoot.wav");
                    Mix_PlayChannel(-1, someSound, 0);
                }
            }
        }
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks; 
        lastFrameTicks = ticks;
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(program->programID);
        
        program->setViewMatrix(viewMatrix);
        program->setProjectionMatrix(projectionMatrix);
        
        // moving of enemy planes
        
        for (int i = 0; i < enemies.size(); i++) {
            for (int j = 0; j < enemies[i].size(); j++) {
                if (enemies[i][j] == nullptr) continue;
                if (enemies[i][j]->Pos.x < -3.2f) {
                    for (int k = 0; k < enemies.size(); k++) {
                        for (int l = 0; l < enemies[k].size(); l++) {
                            if (enemies[k][l]!= nullptr) enemies[k][l]->velocity.x = 0.02f;
                        }
                    }
                }
                else if (enemies[i][j]->Pos.x > 3.2f) {
                    for (int k = 0; k < enemies.size(); k++) {
                        for (int l = 0; l < enemies[k].size(); l++) {
                            if (enemies[k][l]!= nullptr) enemies[k][l]->velocity.x = -0.02f;
                        }
                    }
                }
                
                enemies[i][j]->Pos.x += enemies[i][j]->velocity.x;
                modelMatrix.identity();
                modelMatrix.Translate(enemies[i][j]->Pos.x, curr_y, 0);
                program->setModelMatrix(modelMatrix);
                enemies[i][j]->sprite.Draw(program);
            }
            curr_y -= 0.5f;
        }
        curr_y = init_y;
        
        if (checkAllDeleted(enemies)) {
            delete p;
            p = nullptr;
            state = STATE_MAIN_MENU;
            done = true;
            update(event, program, modelMatrix, viewMatrix, projectionMatrix, false);
        }
        
        // Enemy Fire
        
        amtTime += elapsed;
        if (amtTime > 1) {
            currBullet = enemyFire(elapsed, enemies, bullets, currBullet, modelMatrix, program);
            amtTime = 0;
        }
        
        // player movement
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if(p != nullptr && keys[SDL_SCANCODE_LEFT] && p->Pos.x > -3.2f) {
            p->Pos.x -= p->velocity.x * elapsed;
        } else if(p != nullptr &&  keys[SDL_SCANCODE_RIGHT] && p->Pos.x < 3.2f) {
            p->Pos.x += p->velocity.x * elapsed;
        }
        
        if (p != nullptr) {
            modelMatrix.identity();
            modelMatrix.Translate(p->Pos.x-p->init_Pos.x, -1.6f, 0.0f);
            program->setModelMatrix(modelMatrix);
            p->sprite.Draw(program);
        }
        
        // bullet movement
        
        for (int i =0; i<30; i++) {
            modelMatrix.identity();
            if (bullets[i]->velocity.y != 0.0f) {
                bullets[i]->Pos.y += bullets[i]->velocity.y*elapsed;
                modelMatrix.Translate(bullets[i]->Pos.x, bullets[i]->Pos.y - bullets[i]->init_Pos.y, 0.0f);
                program->setModelMatrix(modelMatrix);
                bullets[i]->sprite.Draw(program);
                
                // checking for collision with enemy space ship
                for (int j = 0; j < enemies.size(); j++) {
                    for (int k = 0; k < enemies[j].size(); k++) {
                        if (enemies[j][k] == nullptr) continue;
                        if (!checkCollision(*bullets[i], *enemies[j][k])) {
                            delete enemies[j][k];
                            enemies[j][k] = nullptr;
                            bullets[i]->Pos.x = 5.0f;
                            modelMatrix.identity();
                            modelMatrix.Translate(5.0f, 0.0f, 0.0f);
                            program->setModelMatrix(modelMatrix);
                            bullets[i]->sprite.Draw(program);
                            score+=1;
                        }
                    }
                }
                
                // checking for collision with player
                if (p != nullptr && !checkCollision(*bullets[i], *p)) {
                    delete p;
                    p = nullptr;
                    state = STATE_MAIN_MENU;
                    done = true;
                    update(event, program, modelMatrix, viewMatrix, projectionMatrix, false);
                }
                
                // checking if bullet leaves boundaries of screen
                if ((bullets[i]->Pos.y > 2.0f || bullets[i]->Pos.y < -2.0f)) {
                    bullets[i]->velocity.y = 0.0f;
                    modelMatrix.identity();
                    modelMatrix.Translate(50, 0, 0);
                    bullets[i]->Pos.x += 50;
                    program->setModelMatrix(modelMatrix);
                    bullets[i]->sprite.Draw(program);
                }
            }
        }
        
        modelMatrix.identity();
        modelMatrix.Translate(0, 1.9, 0);
        program->setModelMatrix(modelMatrix);
        DrawText(program, fontTexture, "Score: " + to_string(score), 0.1, 0.025);
        
        SDL_GL_SwapWindow(displayWindow);
    }
}

void update (SDL_Event* event, ShaderProgram* program, Matrix& modelMatrix, Matrix& viewMatrix, Matrix& projectionMatrix, bool done) {
    
    GLuint fonts = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
    
    GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");
    vector<vector<Entity*>> enemies(6);
    
    SpriteSheet player = SpriteSheet(spriteSheetTexture, 112.0f/1024.0f, 791.0f/1024.0f, 112.0f/1024.0f, 75.0f/1024.0f, 0.3f);
    Entity* p = new Entity(&player, new Vector3 (1.0f, 0.0f, 0.0f));
    
    Entity* bullets[30];
    
    SpriteSheet bullet = SpriteSheet(spriteSheetTexture, 858.0f/1024.0f, 475.0f/1024.0f, 9.0f/1024.0f, 37.0f/1024.0f, 0.2f);
    
    float init_x = -1.1;
    float init_y = 2.0;
    float curr_y = init_y;
    float gap = 0.5;
    
    float lastFrameTicks = 0.0f;
    
    switch (state) {
            
        case STATE_MAIN_MENU: {
            
            updateMenuScreen(event, program, modelMatrix, viewMatrix, projectionMatrix, done, fonts);
            break;
        }
            
        case STATE_GAME_LEVEL: {
            
            setUpGame(spriteSheetTexture, modelMatrix, program, p, bullets, enemies, &bullet, init_x,curr_y, gap);
            
            updateGame(event, program, modelMatrix, viewMatrix, projectionMatrix, done, p, bullets, enemies, lastFrameTicks, curr_y, init_y, fonts);
            
            /*Mix_Music *music;
            music = Mix_LoadMUS(RESOURCE_FOLDER"My Heart Will Go On - Recorder By Candlelight by Matt Mulholland.mp3");
            Mix_PlayMusic(music, -1);*/
            
            break;
        }
            
        default:
            break;
    };
}

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
    
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );

    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    Matrix projectionMatrix;
    Matrix viewMatrix;
    Matrix modelMatrix;
    
    projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
    
    state = STATE_MAIN_MENU;
    srand((unsigned int) time (NULL));
    
    update(&event, &program, modelMatrix, viewMatrix, projectionMatrix, done);
    
    if (someSound != nullptr) {
        Mix_FreeChunk(someSound);
        someSound = nullptr;
    }
    if (music != nullptr) {
        Mix_FreeMusic(music);
        music = nullptr;
    }
    
    SDL_Quit();
    return 0;
}
