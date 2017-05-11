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
#include "Entity.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <SDL_mixer.h>

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

SDL_Window* displayWindow;

using namespace std;

int mapWidth;
int mapHeight;
unsigned char** levelData;
float TILE_SIZE = 1;
int SPRITE_COUNT_X = 20;
int SPRITE_COUNT_Y = 13;
std::vector<Entity*> v;
std::vector<Entity*> enemies;
Entity* player1;
Entity* player2;
std::vector<Entity*> bullets;
int currBulletBeingFired  = 0;
int currSpaceShipRendered = 0;
std::vector<SpriteSheet*> spaceShipSprites;

enum gameState {TITLE_SCREEN, GAME_OVER, WIN, S1, S2, S3, LOADING};
int currState = TITLE_SCREEN;
int prevState = GAME_OVER;

GLuint LoadTexture(const char *image_path) {
    SDL_Surface *surface = IMG_Load(image_path);
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    SDL_FreeSurface(surface);
    
    return textureID;
}

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

void placeEntity (std::string& type, std::string& name, float placeX, float placeY) {
    Entity* ent = new Entity;
    if (type == "player") {
        ent->entityType = ENTITY_PLAYER;
        ent->isStatic = false;
        ent->name = name;
        ent->life = 3;
        if (name == "player1") {
            player1 = ent;
        }
        if (name == "player2") {
            player2 = ent;
        }
    }
    if (type == "collision") {
        ent->entityType = ENTITY_COLLIDE;
        ent->isStatic = true;
        ent->name = name;
    }
    ent->position.x = placeX;
    ent->position.y = placeY;
    v.push_back(ent);
}

bool readHeader(std::ifstream &stream){
    string line;
    mapWidth = -1;
    mapHeight = -1;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "width") {
            mapWidth = atoi(value.c_str());
        } else if(key == "height"){
            mapHeight = atoi(value.c_str());
        } }
    if(mapWidth == -1 || mapHeight == -1) {
        return false;
    } else {
        levelData = new unsigned char*[mapHeight];
        for(int i = 0; i < mapHeight; ++i) {
            levelData[i] = new unsigned char[mapWidth];
        }
        return true;
    }
}

bool readLayerData(std::ifstream &stream){
    string line;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "data") {
            for(int y=0; y < mapHeight; y++) {
                getline(stream, line);
                istringstream lineStream(line);
                string tile;
                for(int x=0; x < mapWidth; x++) {
                    getline(lineStream, tile, ',');
                    unsigned char val =  (unsigned char)atoi(tile.c_str());
                    if(val > 0) {
                        levelData[y][x] = val-1;
                    } else {
                        levelData[y][x] = 0;
                    }
                }
            }
        }
    }
    return true;
}

bool readEntityData(std::ifstream &stream){
    string line;
    string type;
    string name;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if (key == "#") {
            name = value;
        }
        else if(key == "type") {
            type = value;
        }
        else if(key == "location") {
            istringstream lineStream(value);
            string xPosition, yPosition;
            getline(lineStream, xPosition, ',');
            getline(lineStream, yPosition, ',');
            float placeX = atoi(xPosition.c_str())*TILE_SIZE;
            float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
            placeEntity(type, name, placeX+0.5, placeY+0.5);
        }
    }
    return true;
}

void DrawTiles (ShaderProgram *program, GLuint spriteSheetTexture){
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    int amtToDraw = 0;
    
    for(int y=0; y < mapHeight; y++) {
        for(int x=0; x < mapWidth; x++) {
            amtToDraw += 1;
            float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
            float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
            float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
            float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;
            vertexData.insert(vertexData.end(), {
                TILE_SIZE * x, -TILE_SIZE * y,
                TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                TILE_SIZE * x, -TILE_SIZE * y,
                (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
            });
            texCoordData.insert(texCoordData.end(), {
                u, v,
                u, v+(spriteHeight),
                u+spriteWidth, v+(spriteHeight),
                u, v,
                u+spriteWidth, v+(spriteHeight),
                u+spriteWidth, v
            });
            
        }
    }
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
    glDrawArrays(GL_TRIANGLES, 0, amtToDraw*6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void fillBulletVec(ShaderProgram* p, Matrix& modelMatrix, SpriteSheet bullet) {
    
    for (int i = 0; i< 50; i++) {
        Entity* b = new Entity();
        b->sprite = bullet;
        b->position.x = 600;
        b->position.y = 600;
        b->Render(p, modelMatrix);
        b->isStatic = false;
        b->entityType = ENTITY_BULLET;
        b->name = "bullet";
        bullets.push_back(b);
    }
}

int shootBullet (float gap, float x_acc, float y_acc, Entity* other, Matrix& modelMatrix, ShaderProgram* program) {
    if (x_acc == 0) {
        bullets[currBulletBeingFired]->position.x = other->position.x;
        bullets[currBulletBeingFired]->position.y = other->position.y+gap;
    }
    else if (y_acc == 0) {
        bullets[currBulletBeingFired]->position.x = other->position.x+gap;
        bullets[currBulletBeingFired]->position.y = other->position.y;
    }
    modelMatrix.identity();
    modelMatrix.Translate(bullets[currBulletBeingFired]->position.x - bullets[currBulletBeingFired]->position.x, bullets[currBulletBeingFired]->position.y - bullets[currBulletBeingFired]->position.y, 0);
    program->setModelMatrix(modelMatrix);
    bullets[currBulletBeingFired]->sprite.Draw(program);
    bullets[currBulletBeingFired]->acceleration.x = x_acc;
    bullets[currBulletBeingFired]->acceleration.y = y_acc;
    if (currBulletBeingFired == 29) currBulletBeingFired = 0;
    else currBulletBeingFired+=1;
    
    return currBulletBeingFired;
}

int readFile(ifstream& infile) {
    string line;
    int l = 0;
    while (getline(infile, line)) {
        if(line == "[header]") {
            if(!readHeader(infile)) {
                return 0; }
        }
        else if(line == "[layer]") {
            readLayerData(infile);
        }
        else if(line == "[Collision]") {
            readEntityData(infile);
            l++;
        }
    }
    infile.std::__1::ios_base::clear();
    infile.seekg(0);
    return 0;
}

void assignSprites (GLuint BG, GLuint* player1Texture, GLuint* player2Texture) {
    
    SpriteSheet* bush = new SpriteSheet(BG, 256.0f/1280.0f,576.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* fence = new SpriteSheet(BG, 384.0f/1280.0f,768.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.5);
    SpriteSheet* bGTree = new SpriteSheet(BG, 256.0f/1280.0f,704.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* tGTree = new SpriteSheet(BG, 256.0f/1280.0f,640.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* bRTree = new SpriteSheet(BG, 128.0f/1280.0f,704.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* tRTree = new SpriteSheet(BG, 128.0f/1280.0f,640.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* wUL = new SpriteSheet(BG, 640.0f/1280.0f,0.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* wU = new SpriteSheet(BG, 704.0f/1280.0f,0.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* wUR = new SpriteSheet(BG, 768.0f/1280.0f,0.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* wL = new SpriteSheet(BG, 640.0f/1280.0f,64.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* wR = new SpriteSheet(BG, 768.0f/1280.0f,64.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* w = new SpriteSheet(BG, 704.0f/1280.0f,64.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* wDL = new SpriteSheet(BG, 640.0f/1280.0f,128.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* wDR = new SpriteSheet(BG, 768.0f/1280.0f,128.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    SpriteSheet* wD = new SpriteSheet(BG, 704.0f/1280.0f,128.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1.75);
    
    SpriteSheet* rock = new SpriteSheet(BG, 128.0f/1280.0f,576.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* box = new SpriteSheet(BG, 512.0f/1280.0f,576.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* barrel = new SpriteSheet(BG, 512.0f/1280.0f,640.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    
    SpriteSheet* hedge = new SpriteSheet(BG, 0.0f/1280.0f,576.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);
    SpriteSheet* can = new SpriteSheet(BG, 576.0f/1280.0f,768.0f/832.0f ,64.0f/1280.0f,64.0f/832.0f ,1);

    
    for (int i = 0; i < v.size(); i++) {
        
        if (v[i]->name == "bush") {
            v[i]->sprite = *bush;
        }
        else if (v[i]->name == "fence") {
            v[i]->sprite = *fence;
        }
        else if (v[i]->name == "treebottomgreen") {
            v[i]->sprite = *bGTree;
        }
        else if (v[i]->name == "treetopgreen") {
            v[i]->sprite = *tGTree;
        }
        else if (v[i]->name == "treebottomred") {
            v[i]->sprite = *bRTree;
        }
        else if (v[i]->name == "treetopred") {
            v[i]->sprite = *tRTree;
        }
        else if (v[i]->name == "waterul") {
            v[i]->sprite = *wUL;
        }
        else if (v[i]->name == "wateru") {
            v[i]->sprite = *wU;
        }
        else if (v[i]->name == "waterur") {
            v[i]->sprite = *wUR;
        }
        else if (v[i]->name == "waterl") {
            v[i]->sprite = *wL;
        }
        else if (v[i]->name == "water") {
            v[i]->sprite = *w;
        }
        else if (v[i]->name == "waterr") {
            v[i]->sprite = *wR;
        }
        else if (v[i]->name == "waterdl") {
            v[i]->sprite = *wDL;
        }
        else if (v[i]->name == "waterdr") {
            v[i]->sprite = *wDR;
        }
        else if (v[i]->name == "waterd") {
            v[i]->sprite = *wD;
        }
        else if (v[i]->name == "rock") {
            v[i]->sprite = *rock;
        }
        else if (v[i]->name == "box") {
            v[i]->sprite = *box;
        }
        else if (v[i]->name == "barrel") {
            v[i]->sprite = *barrel;
        }
        else if (v[i]->name == "hedge") {
            v[i]->sprite = *hedge;
        }
        else if (v[i]->name == "can") {
            v[i]->sprite = *can;
        }
    }
    
    
    player1->spriteSheet = player1Texture;
    SpriteSheet p1 (*player1Texture, 0.0f/192.0f, 0.0f/192.0f, 48.0f/192.0f,48.0f/192.0f, 1.0f);
    player1->sprite = p1;
    player2->spriteSheet = player2Texture;
    SpriteSheet p2 (*player2Texture, 0.0f/192.0f, 0.0f/192.0f, 48.0f/192.0f,48.0f/192.0f, 1.0f);
    player2->sprite = p2;
}

void reset(int life) {
    for (int i = 0; i < v.size(); i++) {
        delete v[i];
        v[i] = nullptr;
    }
    v.clear();
    player1 = nullptr;
    player2 = nullptr;
    
    for (int i = 0; i < enemies.size(); i++) {
        enemies[i]->position.x = 500;
        enemies[i]->position.y = 500;
        enemies[i]->life = life;
    }
    for (int j = 0; j < bullets.size(); j++){
        bullets[j]->position.x = 600;
        bullets[j]->position.y = 600;
    }
}

void setupSpaceShipSprites (GLuint spriteSheetTexture) {
    SpriteSheet* enemyBlack1 = new SpriteSheet(spriteSheetTexture, 423.0f/1024.0f, 728.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlack2 = new SpriteSheet(spriteSheetTexture, 120.0f/1024.0f, 604.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlack3 = new SpriteSheet(spriteSheetTexture, 144.0f/1024.0f, 156.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlack4 = new SpriteSheet(spriteSheetTexture, 518.0f/1024.0f, 325.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlack5 = new SpriteSheet(spriteSheetTexture, 346.0f/1024.0f, 150.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlue1 = new SpriteSheet(spriteSheetTexture, 425.0f/1024.0f, 468.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlue2 = new SpriteSheet(spriteSheetTexture, 143.0f/1024.0f, 293.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlue3 = new SpriteSheet(spriteSheetTexture, 222.0f/1024.0f, 0.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlue4 = new SpriteSheet(spriteSheetTexture, 518.0f/1024.0f, 409.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyBlue5 = new SpriteSheet(spriteSheetTexture, 421.0f/1024.0f, 814.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyGreen1 = new SpriteSheet(spriteSheetTexture, 425.0f/1024.0f, 552.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyGreen2 = new SpriteSheet(spriteSheetTexture, 133.0f/1024.0f, 412.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyGreen3 = new SpriteSheet(spriteSheetTexture, 224.0f/1024.0f, 496.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyGreen4 = new SpriteSheet(spriteSheetTexture, 518.0f/1024.0f, 493.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyGreen5 = new SpriteSheet(spriteSheetTexture, 408.0f/1024.0f, 907.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyRed1 = new SpriteSheet(spriteSheetTexture, 425.0f/1024.0f, 384.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyRed2 = new SpriteSheet(spriteSheetTexture, 120.0f/1024.0f, 520.0f/1024.0f, 104.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyRed3 = new SpriteSheet(spriteSheetTexture, 224.0f/1024.0f, 580.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyRed4 = new SpriteSheet(spriteSheetTexture, 520.0f/1024.0f, 577.0f/1024.0f, 82.0f/1024.0f, 84.0f/1024.0f, 1);
    SpriteSheet* enemyRed5 = new SpriteSheet(spriteSheetTexture, 423.0f/1024.0f, 644.0f/1024.0f, 97.0f/1024.0f, 84.0f/1024.0f, 1);
    spaceShipSprites.push_back(enemyBlack1);
    spaceShipSprites.push_back(enemyBlack2);
    spaceShipSprites.push_back(enemyBlack3);
    spaceShipSprites.push_back(enemyBlack4);
    spaceShipSprites.push_back(enemyBlack5);
    spaceShipSprites.push_back(enemyBlue1);
    spaceShipSprites.push_back(enemyBlue2);
    spaceShipSprites.push_back(enemyBlue3);
    spaceShipSprites.push_back(enemyBlue4);
    spaceShipSprites.push_back(enemyBlue5);
    spaceShipSprites.push_back(enemyGreen1);
    spaceShipSprites.push_back(enemyGreen2);
    spaceShipSprites.push_back(enemyGreen3);
    spaceShipSprites.push_back(enemyGreen4);
    spaceShipSprites.push_back(enemyGreen5);
    spaceShipSprites.push_back(enemyRed1);
    spaceShipSprites.push_back(enemyRed2);
    spaceShipSprites.push_back(enemyRed3);
    spaceShipSprites.push_back(enemyRed4);
    spaceShipSprites.push_back(enemyRed5);
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("George & Betty Grand Adventures: Survive the Apocalypse!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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
    
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    Matrix projectionMatrix;
    Matrix viewMatrix;
    Matrix modelMatrix;
    
    projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
    
    float lastFrameTicks(0.0f), animationElapsed (0.0f), enemyTimer (0), red(0), green(0), blue(0), time_counter(0), currTime(0), planeMoveConst (0), spawnRate(0), shipBobbleValue (0);
    bool p1u(false), p1d(true), p1l(false), p1r(false), p2u(false), p2d(true), p2r(false), p2l(false), start(false);
    int numEnemies (500);
    string text(" ");
    
    GLuint player1Texture = LoadTexture(RESOURCE_FOLDER"p1.png");
    GLuint player2Texture = LoadTexture(RESOURCE_FOLDER"p2.png");
    GLuint BG = LoadTexture(RESOURCE_FOLDER"RPGpack_sheet.png");
    GLuint font = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
    
    srand((unsigned)time(NULL));
    ifstream map1(RESOURCE_FOLDER"map1.txt");
    ifstream map2(RESOURCE_FOLDER"map2.txt");
    ifstream map3(RESOURCE_FOLDER"map3.txt");
    
    Mix_Chunk* shoot = Mix_LoadWAV(RESOURCE_FOLDER"Shoot.wav");
    Mix_Chunk* bump = Mix_LoadWAV(RESOURCE_FOLDER"Bump.wav");
    Mix_Chunk* death = Mix_LoadWAV(RESOURCE_FOLDER"Death.wav");
    Mix_Chunk* spaceShipExplode = Mix_LoadWAV(RESOURCE_FOLDER"SpaceShipExplode.wav");
    
    Mix_Music* titleMusic = Mix_LoadMUS(RESOURCE_FOLDER"title_music.mp3");
    Mix_Music* S1Music = Mix_LoadMUS(RESOURCE_FOLDER"map1.mp3");
    Mix_Music* S2Music = Mix_LoadMUS(RESOURCE_FOLDER"map2.mp3");
    Mix_Music* S3Music = Mix_LoadMUS(RESOURCE_FOLDER"map3.mp3");
    Mix_Music* winMusic = Mix_LoadMUS(RESOURCE_FOLDER"win.mp3");
    Mix_Music* gameOverMusic = Mix_LoadMUS(RESOURCE_FOLDER"gameOver.mp3");
    
    GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");
    SpriteSheet bullet1 = SpriteSheet(spriteSheetTexture, 596.0f/1024.0f, 961.0f/1024.0f, 48.0f/1024.0f, 46.0f/1024.0f, 0.5f);
    SpriteSheet bullet2 = SpriteSheet(spriteSheetTexture, 738.0f/1024.0f, 650.0f/1024.0f, 37.0f/1024.0f, 36.0f/1024.0f, 0.5f);
    fillBulletVec(&program, modelMatrix, bullet1);
    
    setupSpaceShipSprites(spriteSheetTexture);
    
    for(int i = 0; i < numEnemies; i++){
        Entity* spaceShip = new Entity;
        spaceShip->position.x = 500;
        spaceShip->position.y = 500;
        spaceShip->entityType = ENTITY_ENEMY;
        spaceShip->life = 1;
        enemies.push_back(spaceShip);
    }
    
    Mix_OpenAudio( 44100/2, MIX_DEFAULT_FORMAT, 2, 4096 );
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                map1.close();
                map2.close();
                map3.close();
                Mix_FreeChunk(bump);
                Mix_FreeChunk(death);
                Mix_FreeChunk(shoot);
                Mix_FreeChunk(spaceShipExplode);
                Mix_FreeMusic(titleMusic);
                Mix_FreeMusic(winMusic);
                Mix_FreeMusic(gameOverMusic);
                Mix_FreeMusic(S1Music);
                Mix_FreeMusic(S2Music);
                Mix_FreeMusic(S3Music);
                done = true;
            }
            else if (event.type == SDL_KEYUP) {
                if( player1 != nullptr){
                    if (event.key.keysym.scancode == SDL_SCANCODE_W || event.key.keysym.scancode == SDL_SCANCODE_S) {
                        player1->acceleration.y = 0;
                    }
                    else if (event.key.keysym.scancode == SDL_SCANCODE_A || event.key.keysym.scancode == SDL_SCANCODE_D) {
                        player1->acceleration.x = 0;
                    }
                    else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                        bullets[currBulletBeingFired]->sprite = bullet1;
                        Mix_Chunk* shoot = Mix_LoadWAV(RESOURCE_FOLDER"Shoot.wav");
                        Mix_PlayChannel(-1, shoot, 0);
                        float x_acc = 0.0, y_acc = 0.0;
                        if (p1u) {
                            x_acc = 0;
                            y_acc = 200;
                        }
                        else if(p1d) {
                            x_acc = 0;
                            y_acc = -200;
                        }
                        else if (p1l) {
                            x_acc = -200;
                            y_acc = 0;
                        }
                        else if (p1r) {
                            x_acc = 200;
                            y_acc = 0;
                        }
                        currBulletBeingFired = shootBullet(0.2, x_acc, y_acc, player1, modelMatrix, &program);
                    }
                }
                if (player2 != nullptr) {
                    if (event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                        player2->acceleration.x = 0;
                    }
                    else if (event.key.keysym.scancode == SDL_SCANCODE_UP || event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                        player2->acceleration.y = 0;
                    }
                    else if (event.key.keysym.scancode == SDL_SCANCODE_SLASH) {
                        bullets[currBulletBeingFired]->sprite = bullet2;
                        Mix_Chunk* shoot = Mix_LoadWAV(RESOURCE_FOLDER"Shoot.wav");
                        Mix_PlayChannel(-1, shoot, 0);
                        float x_acc = 0.0, y_acc = 0.0;
                        if (p2u) {
                            x_acc = 0;
                            y_acc = 200;
                        }
                        else if(p2d) {
                            x_acc = 0;
                            y_acc = -200;
                        }
                        else if (p2l) {
                            x_acc = -200;
                            y_acc = 0;
                        }
                        else if (p2r) {
                            x_acc = 200;
                            y_acc = 0;
                        }
                        currBulletBeingFired = shootBullet(0.2, x_acc, y_acc, player2, modelMatrix, &program);
                    }
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    Mix_FreeChunk(bump);
                    Mix_FreeChunk(death);
                    Mix_FreeChunk(shoot);
                    Mix_FreeChunk(spaceShipExplode);
                    map1.close();
                    map2.close();
                    map3.close();
                    Mix_FreeMusic(titleMusic);
                    Mix_FreeMusic(winMusic);
                    Mix_FreeMusic(gameOverMusic);
                    Mix_FreeMusic(S1Music);
                    Mix_FreeMusic(S2Music);
                    Mix_FreeMusic(S3Music);
                    done = true;
                }
                if ((currState == TITLE_SCREEN || currState == GAME_OVER || currState == WIN) &&event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                    start = true;
                }
            }
        }
        
        
        glClearColor(red/255.0f, green/255.0f, blue/255.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(program.programID);
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        float fixedElapsed = elapsed;
        if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
            fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
        }
        while (fixedElapsed >= FIXED_TIMESTEP ) {
            fixedElapsed -= FIXED_TIMESTEP;
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        animationElapsed += elapsed;
        
        currTime+=elapsed;
        
        if (currState == TITLE_SCREEN && start) { // title to loading
            Mix_HaltMusic();
            currTime = 0;
            currState = LOADING;
            prevState = TITLE_SCREEN;
            reset(1);
            text = "Stage 1";
            start = false;
        }
        else if (prevState == TITLE_SCREEN && currTime >= time_counter) { // loading to S1
            currTime = 0;
            currState = S1;
            readFile(map1);
            assignSprites(BG, &player1Texture, &player2Texture);
            prevState = LOADING;
        }
        else if (currState == S1 && currTime >= time_counter) { // S1 to loading
            Mix_HaltMusic();
            currTime = 0;
            currState = LOADING;
            prevState = S1;
            reset(2);
            text = "Stage 2";
        }
        else if (prevState == S1 && currTime >= time_counter) { // loading to S2
            currTime = 0;
            currState = S2;
            readFile(map2);
            assignSprites(BG, &player1Texture, &player2Texture);
            prevState = LOADING;
        }
        else if (currState == S2 && currTime >= time_counter) { // S2 to loading
            Mix_HaltMusic();
            currTime = 0;
            currState = LOADING;
            prevState = S2;
            reset(3);
            text = "Stage 3";
        }
        else if (prevState == S2 && currTime >= time_counter) { // loading to S3
            currTime = 0;
            currState = S3;
            readFile(map3);
            assignSprites(BG, &player1Texture, &player2Texture);
            prevState = LOADING;
            
        }
        else if (currState == S3 && currTime >= time_counter) { // S3 to win
            currTime = 0;
            reset(1);
            currState = WIN;
            
        }
        else if (currState == GAME_OVER && start) { // game over to title
            currTime = 0;
            currState = TITLE_SCREEN;
            prevState = GAME_OVER;
            start = false;
        }
        else if (currState == WIN && start) { // win to title
            currTime = 0;
            currState = TITLE_SCREEN;
            prevState = WIN;
            start = false;
        }
        else if (player1 != nullptr && player2!=nullptr && player1->life <=0 && player2->life <=0) {
            // any to game over
            reset(1);
            currTime = 0;
            currState = GAME_OVER;
        }
        
        if(currState == TITLE_SCREEN){
            
            red = 0.0;
            green = 0.0;
            blue = 0.0;
            modelMatrix.identity();
            modelMatrix.Translate(-0.5, 0.4, 0);
            modelMatrix.Scale(0.5, 0.5, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Welcome!", 0.3, 0);
            
            modelMatrix.identity();
            modelMatrix.Translate(-0.45, -0.1, 0);
            modelMatrix.Scale(0.15, 0.15, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Press [ENTER] to play!", 0.3, 0);
            
            modelMatrix.identity();
            modelMatrix.Translate(-0.6, -0.3, 0);
            modelMatrix.Scale(0.15, 0.15, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Press [ESC] to exit the game!", 0.3, 0);
            if(currTime <= elapsed){
                Mix_PlayMusic(titleMusic, -1);
            }
        }
        else if(currState == GAME_OVER){
            red = 0.0;
            green = 0.0;
            blue = 0.0;
            modelMatrix.identity();
            modelMatrix.Translate(-0.85, 0.4, 0);
            modelMatrix.Scale(0.2, 0.2, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Oh no! George and Betty died!", 0.3, 0);
            
            modelMatrix.identity();
            modelMatrix.Translate(-0.57, -0.1, 0);
            modelMatrix.Scale(0.15, 0.15, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Press [ENTER] to play again!", 0.3, 0);
            
            modelMatrix.identity();
            modelMatrix.Translate(-0.6, -0.3, 0);
            modelMatrix.Scale(0.15, 0.15, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Press [ESC] to exit the game!", 0.3, 0);
            
            if(currTime <= elapsed){
                Mix_PlayMusic(gameOverMusic, -1);
            }
        }
        else if(currState == WIN){
            red = 0.0;
            green = 0.0;
            blue = 0.0;
            modelMatrix.identity();
            modelMatrix.Translate(-0.65, 0.4, 0);
            modelMatrix.Scale(0.2, 0.2, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Congrats! You survived!", 0.3, 0);
            
            modelMatrix.identity();
            modelMatrix.Translate(-0.57, -0.1, 0);
            modelMatrix.Scale(0.15, 0.15, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Press [ENTER] to play again!", 0.3, 0);
            
            modelMatrix.identity();
            modelMatrix.Translate(-0.6, -0.3, 0);
            modelMatrix.Scale(0.15, 0.15, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Press [ESC] to exit the game!", 0.3, 0);
            
            if(currTime <= elapsed){
                Mix_PlayMusic(winMusic, -1);
            }
        }
        else if(currState == S1){
            
            red = 151.0f;
            green = 198.0f;
            blue = 67.0f;
            time_counter = 120.0f;
            planeMoveConst = 2.0f;
            spawnRate = 2;
            modelMatrix.identity();
            program.setModelMatrix(modelMatrix);
            DrawTiles(&program, BG);
            
            if(currTime <= elapsed){
                Mix_PlayMusic(S1Music, -1);
            }
        }
        else if(currState == S2){
            
            red = 225.0f;
            green = 212.0f;
            blue = 184.0f;
            time_counter = 360.0f;
            planeMoveConst = 3.0f;
            spawnRate = 1;
            modelMatrix.identity();
            program.setModelMatrix(modelMatrix);
            DrawTiles(&program, BG);
            
            if(currTime <= elapsed){
                Mix_PlayMusic(S2Music, -1);
            }
        }
        else if(currState == S3){
            
            red = 182.0f;
            green = 195.0f;
            blue = 196.0f;
            time_counter = 480.0f;
            planeMoveConst = 4.0f;
            spawnRate = 0.5f;
            modelMatrix.identity();
            program.setModelMatrix(modelMatrix);
            DrawTiles(&program, BG);
            
            if(currTime <= elapsed){
                Mix_PlayMusic(S3Music, -1);
            }
        }
        else if(currState == LOADING){
            red = 0.0f;
            green = 0.0f;
            blue = 0.0f;
            time_counter = 1.0f;
            modelMatrix.identity();
            modelMatrix.Translate(-0.45, 0, 0);
            modelMatrix.Scale(0.5, 0.5, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, text, 0.3, 0);
        }
        
        if( player2 != nullptr){
            if(keys[SDL_SCANCODE_LEFT]) {
                player2->acceleration.x -= 1;
                player2->state = 1;
                if(animationElapsed > 1.0/60.0f){
                    player2->state += 4;
                    player2->state %= 16;
                    animationElapsed = 0.0f;
                }
                p2l = true;
                p2d = false;
                p2r = false;
                p2u = false;
            }
            if(keys[SDL_SCANCODE_RIGHT]) {
                player2->acceleration.x += 1;
                player2->state = 3;
                if(animationElapsed > 1.0/60.0f){
                    player2->state += 4;
                    player2->state %= 16;
                    animationElapsed = 0.0f;
                }
                p2l = false;
                p2d = false;
                p2r = true;
                p2u = false;
            }
            if(keys[SDL_SCANCODE_UP]) {
                player2->acceleration.y += 1;
                player2->state = 2;
                if(animationElapsed > 1.0/60.0f){
                    player2->state += 4;
                    player2->state %= 16;
                    animationElapsed = 0.0f;
                }
                p2l = false;
                p2d = false;
                p2r = false;
                p2u = true;
            }
            if(keys[SDL_SCANCODE_DOWN]) {
                player2->acceleration.y -= 1;
                player2->state = 0;
                if(animationElapsed > 1.0/60.0f){
                    player2->state += 4;
                    player2->state %= 16;
                    animationElapsed = 0.0f;
                }
                p2l = false;
                p2d = true;
                p2r = false;
                p2u = false;
            }
        }
        if( player1 != nullptr){
            if(keys[SDL_SCANCODE_A]) {
                player1->acceleration.x -= 1;
                player1->state = 1;
                if(animationElapsed > 1.0/60.0f){
                    player1->state += 4;
                    player1->state %= 16;
                    animationElapsed = 0.0f;
                }
                p1l = true;
                p1d = false;
                p1r = false;
                p1u = false;
            }
            if(keys[SDL_SCANCODE_D]) {
                player1->acceleration.x += 1;
                player1->state = 3;
                if(animationElapsed > 1.0/60.0f){
                    player1->state += 4;
                    player1->state %= 16;
                    animationElapsed = 0.0f;
                }
                p1l = false;
                p1d = false;
                p1r = true;
                p1u = false;
            }
            if(keys[SDL_SCANCODE_W]) {
                player1->acceleration.y += 1;
                player1->state = 2;
                if(animationElapsed > 1.0/60.0f){
                    player1->state += 4;
                    player1->state %= 16;
                    animationElapsed = 0.0f;
                }
                p1l = false;
                p1d = false;
                p1r = false;
                p1u = true;
            }
            if(keys[SDL_SCANCODE_S]) {
                player1->acceleration.y -= 1;
                player1->state = 0;
                if(animationElapsed > 1.0/60.0f){
                    player1->state += 4;
                    player1->state %= 16;
                    animationElapsed = 0.0f;
                }
                p1l = false;
                p1d = true;
                p1r = false;
                p1u = false;
            }
        }
        projectionMatrix.identity();
        program.setProjectionMatrix(projectionMatrix);
        
        viewMatrix.identity();
        if (player1 != nullptr && player2 != nullptr) {
            
            if (player1->position.x != 1000 && player2->position.x != 1000) {
                float val = sqrtf(powf((player1->position.x-player2->position.x), 2.0f) + powf(player1->position.y-player2->position.y, 2.0f));
                if (val >= 9 && val <=62) viewMatrix.Scale(1/val, 1/val,1);
                else if (val < 9) viewMatrix.Scale(1.0f/9.0f, 1.0f/9.0f, 1);
                else if (val > 62) viewMatrix.Scale(1.0f/62.0f, 1.0f/62.0f, 1);
                viewMatrix.Translate(-(player1->position.x+player2->position.x)/2, -(player1->position.y+player2->position.y)/2, 0);
            }
            else if (player1->position.x == 1000 && player2->position.x != 1000) {
                viewMatrix.Scale(1.0f/9.0f, 1.0f/9.0f, 1);
                viewMatrix.Translate(-player2->position.x, -player2->position.y, 1);
            }
            else if (player1->position.x != 1000 && player2->position.x == 1000) {
                viewMatrix.Scale(1.0f/9.0f, 1.0f/9.0f, 1);
                viewMatrix.Translate(-player1->position.x, -player1->position.y, 1);
            }
        }
        program.setViewMatrix(viewMatrix);
        
        if (player1!= nullptr && player2!= nullptr) {
            enemyTimer += elapsed;
            if(enemyTimer > spawnRate){
                
                float angle = (rand() % 360);
                float xShip = 30*cos(angle) + (player1->position.x+player2->position.x)/2;
                float yShip = 30*sin(angle) + (player1->position.y+player2->position.y)/2;
                
                
                int randColor = rand() % 20;
                enemies[currSpaceShipRendered]->sprite = *(spaceShipSprites[randColor]);
                
                enemies[currSpaceShipRendered]->position.x = xShip;
                enemies[currSpaceShipRendered]->position.y = yShip;
                enemyTimer = 0;
                currSpaceShipRendered+=1;
                currSpaceShipRendered%=numEnemies;
            }
        }
        
        for (int i = 0; i < v.size(); i++) {
            if (v[i] == nullptr) continue;
            v[i]->Update(FIXED_TIMESTEP);
            v[i]->Render(&program, modelMatrix);
            if (player1 != v[i]) {
                if(player1->collidesWith(v[i])){
                    //Mix_Chunk* bump = Mix_LoadWAV(RESOURCE_FOLDER"Shoot.wav");
                    //Mix_PlayChannel(-1, bump, 0);
                    player1->handleCollision(v[i]);
                }
            }
            if (player2 != v[i]) {
                if(player2->collidesWith(v[i])){
                    //Mix_Chunk* bump = Mix_LoadWAV(RESOURCE_FOLDER"Shoot.wav");
                    //Mix_PlayChannel(-1, bump, 0);
                    player2->handleCollision(v[i]);
                }
            }
        }
        shipBobbleValue += elapsed;
        
        for (int i = 0; i < enemies.size(); i++) {
            if(enemies[i]->position.x != 500 && enemies[i]->position.y != 500){
                float dist1 = INT_MAX;
                float dist2 = INT_MAX;
                if (player1!= nullptr) {
                    dist1 = sqrtf(powf((player1->position.x-enemies[i]->position.x), 2.0f) + powf(player1->position.y-enemies[i]->position.y, 2.0f));
                }
                if (player2!=nullptr){
                    dist2 = sqrtf(powf((player2->position.x-enemies[i]->position.x), 2.0f) + powf(player2->position.y-enemies[i]->position.y, 2.0f));
                }
                if(dist1 < dist2){
                    enemies[i]->velocity.x = planeMoveConst * (player1->position.x-enemies[i]->position.x) / dist1;
                    enemies[i]->velocity.y = planeMoveConst * (player1->position.y-enemies[i]->position.y) / dist1;
                }
                else if(dist2 < dist1){
                    enemies[i]->velocity.x = planeMoveConst * (player2->position.x-enemies[i]->position.x) / dist2;
                    enemies[i]->velocity.y = planeMoveConst * (player2->position.y-enemies[i]->position.y) / dist2;
                }
            }
            if (player1 != nullptr && player2 != nullptr) {
                enemies[i]->Update(FIXED_TIMESTEP);
                enemies[i]->Render(&program, modelMatrix, shipBobbleValue);
            }
            if(player1 != nullptr && player1->collidesWith(enemies[i])){
                Mix_Chunk* spaceShipExplode = Mix_LoadWAV(RESOURCE_FOLDER"SpaceShipExplode.wav");
                Mix_PlayChannel(-1, spaceShipExplode, 0);
                if (player1->life == 1 && player1->position.x != 1000) {
                    Mix_Chunk* death = Mix_LoadWAV(RESOURCE_FOLDER"Death.wav");
                    Mix_PlayChannel(-1, death, 0);
                }
                player1->handleCollision(enemies[i]);
            }
            if(player2 != nullptr && player2->collidesWith(enemies[i])){
                Mix_Chunk* spaceShipExplode = Mix_LoadWAV(RESOURCE_FOLDER"SpaceShipExplode.wav");
                Mix_PlayChannel(-1, spaceShipExplode, 0);
                if (player1->life == 1 && player1->position.x != 1000) {
                    Mix_Chunk* death = Mix_LoadWAV(RESOURCE_FOLDER"Death.wav");
                    Mix_PlayChannel(-1, death, 0);
                }
                player2->handleCollision(enemies[i]);
            }
            for(int k = 0; k < bullets.size(); k++){
                if(bullets[k]->collidesWith(enemies[i])){
                    Mix_Chunk* spaceShipExplode = Mix_LoadWAV(RESOURCE_FOLDER"SpaceShipExplode.wav");
                    Mix_PlayChannel(-1, spaceShipExplode, 0);
                    bullets[k]->handleCollision(enemies[i]);
                }
            }
            for (int j = 0; j < enemies.size(); j++) {
                if (i!= j) {
                    if (enemies[i]->collidesWith(enemies[j])) {
                        //Mix_Chunk* bump = Mix_LoadWAV(RESOURCE_FOLDER"Shoot.wav");
                        //Mix_PlayChannel(-1, bump, 0);
                        enemies[i]->handleCollision(enemies[j]);
                    }
                }
            }
        }
        
        for (int i = 0; i < bullets.size(); i++) {
            bullets[i]->Update(FIXED_TIMESTEP);
            bullets[i]->Render(&program, modelMatrix);
        }
        
        if(player1){
            modelMatrix.identity();
            modelMatrix.Translate(player1->position.x - 1, player1->position.y + 1, 0);
            modelMatrix.Scale(1, 1, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "George:" + to_string(player1->life), 0.3, 0);
        }
        
        if(player2){
            modelMatrix.identity();
            modelMatrix.Translate(player2->position.x - 0.9, player2->position.y + 1, 0);
            modelMatrix.Scale(1, 1, 1);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, font, "Betty:" + to_string(player2->life), 0.3, 0);
        }
        if(player1 || player2){
            if(player1->position.x != 1000 && player2->position.x != 1000){
                float val = sqrtf(powf((player1->position.x-player2->position.x), 2.0f) + powf(player1->position.y-player2->position.y, 2.0f));
                modelMatrix.identity();
                
                if(val < 9){
                    modelMatrix.Translate(((player1->position.x+player2->position.x)/2 - 1.35), ((player1->position.y+player2->position.y)/2 + 8), 0);
                    modelMatrix.Scale(0.16*9, 0.16*9, 1);
                    program.setModelMatrix(modelMatrix);
                    DrawText(&program, font, "Time:" + to_string(int(currTime)), 0.3, 0);
                }/*
                else if(val > 62){
                    modelMatrix.Translate(((player1->position.x+player2->position.x)/2 - 1.35), ((player1->position.y+player2->position.y)/2 + 8), 0);
                    modelMatrix.Scale(0.16*62, 0.16*62, 1);
                }
                else{
                    modelMatrix.Translate(((player1->position.x+player2->position.x)/2 - 1.35), ((player1->position.y+player2->position.y)/2 + 8), 0);
                    modelMatrix.Scale(0.16*val, 0.16*val, 1);
                }*/
            }
            if(player1->position.x != 1000 && player2->position.x == 1000){
                modelMatrix.identity();
                modelMatrix.Translate(player1->position.x - 1.35, player1->position.y + 8, 0);
                modelMatrix.Scale(0.16*9, 0.16*9, 1);
                program.setModelMatrix(modelMatrix);
                DrawText(&program, font, "Time:" + to_string(int(currTime)), 0.3, 0);
            }
            if(player2->position.x != 1000 && player1->position.x == 1000){
                modelMatrix.identity();
                modelMatrix.Translate(player2->position.x - 1.35, player2->position.y + 8, 0);
                modelMatrix.Scale(0.16*9, 0.16*9, 1);
                program.setModelMatrix(modelMatrix);
                DrawText(&program, font, "Time:" + to_string(int(currTime)), 0.3, 0);
            }
        }
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
