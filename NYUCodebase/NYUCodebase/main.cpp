/*
Assignment #4 - Sound
Ryan Goudjil
N18559209
CS3113

*/
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include <vector>
#include <stdio.h>    
#include <stdlib.h>     /* srand, rand */
#include <time.h> 
#include<iostream>
using namespace std;
#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

const int LEVEL_HEIGHT = 12;
const int LEVEL_WIDTH = 24;

// GRAPHICS
float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
SDL_Window* displayWindow;
unsigned int levelData[LEVEL_HEIGHT][LEVEL_WIDTH];

// MATRICES
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

// MUSIC
Mix_Chunk *ballCollide;
Mix_Chunk *ballScore;
Mix_Music *music;

// GAMELOOP
//float toShoot = 2.0f;
float moveDown = 3.0f;
float lastFrameTicks = 0.0f;
float angle = 0.0f;

// SCORES
int score1 = 0;
int score2 = 0;

// STATES
bool title = true;
bool game = true;

// TEXTURES
GLuint blockTexture;
GLuint playerTexture;
GLuint example;
GLuint textTexture;
GLuint paddleTexture;
GLuint ballTexture;

// GENERAL
bool done = false;
SDL_Event event;

ShaderProgram* program;
void DrawSpriteSheetSprite(ShaderProgram* program, int index, int spriteCountX, int spriteCountY, float x, float y, GLuint textID);

// classes 
class Entity{
public:
	
	void Draw(ShaderProgram* program){}
	void Update(float elapsed){}

	float position_x;
	float position_y;

	int textureID;
	bool isPlayer;

	float width;
	float height;

	float velocity_x;
	float velocity_y;
};

class Block :public Entity{
public:
	// a static entity, doesn't need to check things like velocity or gravity
	Block(float px, float py, float w, float h) :position_x(px), position_y(py), width(w), height(h),textureID(blockTexture){}
	float position_x;
	float position_y;

	int textureID;
	float width;
	float height;

	void Draw(ShaderProgram* program){
		program->setModelMatrix(modelMatrix);
		program->setProjectionMatrix(projectionMatrix);
		program->setViewMatrix(viewMatrix);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);

	}
};

class Player :public Entity{
public:
	bool onGround;

	float position_x;
	float position_y;

	int textureID;

	float width;
	float height;

	float velocity_x;
	float velocity_y;

	float acceleration_x;
	float acceleration_y;

	Player(float px, float py ) : width(1.0f),height(1.0f),velocity_x(0.0f), velocity_y(0.0f), acceleration_x(0.0f),acceleration_y(-9.81f), onGround(false), textureID(playerTexture){}
	void jump(){
		if (onGround) velocity_y = 4.0f;
	}
	void Draw(ShaderProgram* program){
		program->setModelMatrix(modelMatrix);
		program->setProjectionMatrix(projectionMatrix);
		program->setViewMatrix(viewMatrix);

		modelMatrix.identity();
		modelMatrix.Translate(position_x, position_y, 0.0);
		modelMatrix.Scale(width, height, 0.0);
		program->setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, textureID);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	void Update(float elapsed){
		onGround = false;
		velocity_x += acceleration_x*elapsed;
		velocity_y += acceleration_y*elapsed;
		velocity_x = 0.0f;
		velocity_y = 0.0f;
	}

	bool checkCollission(Entity* other){ // other is block
		float penetration = 0.0f; 
		float top = position_y + height / 2; // iniatilize all these variables to make the if clauses less wordy
		float bottom = position_y - height / 2;
		float right = position_x + width / 2;
		float left = position_x + width / 2;

		float topOther = other->position_y + other->height / 2;
		float bottomOther = other->position_y - other->height / 2;
		float rightOther = other->position_x + other->width / 2;
		float leftOther = other->position_x + other->width / 2;

		if (top < bottomOther && //check if bottom is colliding
			fabs(left - leftOther) < other->width &&
			fabs(right - rightOther) < other->width){
			onGround = true;
			penetration = (position_y - other->position_y - (height/2)- (other->height/2));
			position_y += penetration + 0.0005;
		}
		if (left < rightOther && //check if left is colliding
			fabs(top - topOther) < other->height &&
			fabs(bottom - bottomOther) < other->height){
			penetration = (position_x - other->position_x - (width / 2) - (other->width / 2));
			position_x += penetration + 0.0005;
		}
		if (top < bottomOther && //check if right is colliding
			fabs(left - leftOther) < other->width &&
			fabs(right - rightOther) < other->height){
			penetration = (other->position_x - position_x - (width / 2) - (other->width / 2));
			position_x -= penetration + 0.0005;
		}
		if(onGround) return true;
		else return false;
	}
};

vector<Entity*> entities;
vector<Block*> blocks;
Player * player;

GLuint LoadTexture(const char *image_path) {
	SDL_Surface *surface = IMG_Load(image_path);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	SDL_FreeSurface(surface);
	return textureID;
}

void buildMap(){
	int rando = 0;
	for (int y = 0; y < LEVEL_HEIGHT; y++){ // fill level array
		for (int x = 0; x < LEVEL_WIDTH; x++){
			rando = rand() % 2 + 1;
			if (y % 2 == 0) levelData[y][x] = 0; // if even numbered line, skip it
			else if (rando > 1) levelData[y][x] = 1; // else 50% chance of generating a tile
		}
	}

	//now create new blocks according to level array
	for (int y = 0; y < LEVEL_HEIGHT; y++){
		for (int x = 0; x < LEVEL_WIDTH; x++){
			if (levelData[y][x] == 1) blocks.push_back(new Block(float((8 / LEVEL_WIDTH) / 2 + x), float(((8 / LEVEL_HEIGHT) / 2 - y)),1.0f,1.0f));
		}
	}
}

void SetUp(){
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 1280, 720);
	// ratio is 1.77779 : 1 but for calculation's sake i use another ratio [1280/80:720/80]
	projectionMatrix.setOrthoProjection(-8.0f, 8.0f, -4.5f, 4.5f, -1.0f, 1.0f);
	playerTexture = LoadTexture(RESOURCE_FOLDER"player.png");
	blockTexture = LoadTexture(RESOURCE_FOLDER"emoji.png");
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	player = new Player(0.0f,0.0f);
	Block b1(0.0f, -1.0f, 1.0f,1.0f);
	blocks.push_back(&b1);
	buildMap();
	ballCollide = Mix_LoadWAV("blip.wav");
	ballScore = Mix_LoadWAV("bicycle_bell.wav");
	music = Mix_LoadMUS("mariomusic.mp3");
	Mix_PlayMusic(music, -1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Update(float elapsed){
	player->Update(elapsed);
	for (int i = 0; i < blocks.size(); i++){
		player->checkCollission(blocks[i]);  // check collissions with every static block
	}
	viewMatrix.identity();
	viewMatrix.Translate(-(player->position_x), -(player->position_y), 0.0f); // move view matrix to scroll
	program->setViewMatrix(viewMatrix);
}

void ProcessEvents(){
	//Mix_PlayMusic(music,-1);
	while (SDL_PollEvent(&event)){
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE){
			done = true;
		}
		else if (event.type == SDL_KEYDOWN){
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE){
				player->jump();
			}
		}

	}
	float ticks = (float)SDL_GetTicks() / 1000.0F;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	
	glClearColor(0.4f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_LEFT]){
		player->acceleration_x = -4.0f; // moves player left
	}

	if (keys[SDL_SCANCODE_RIGHT]){
		player->acceleration_x = 4.0f; // moves player right
	}
	
	//check if player is colliding
	for (int i = 0; i<blocks.size(); i++){
		player->checkCollission(blocks[i]); // goes through all static objects [blocks] and checks if colliding
	}
	Update(elapsed);
}

void Render(ShaderProgram* program){
	
	glClearColor(0.5f, 0.0f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	modelMatrix.identity();
	modelMatrix.Translate(0.0f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	for (int i = 0; i < blocks.size(); i++) { 
		blocks[i]->Draw(program); 
		cout << "current block: " << i << blocks[i]->position_x << ","<< blocks[i]->position_y << endl;
	}
	player->Draw(program);
	
	SDL_GL_SwapWindow(displayWindow);

}

void CleanUp(){
// need this to delete everything and reset values
	Mix_FreeChunk(ballCollide);
	Mix_FreeChunk(ballScore);
	Mix_FreeMusic(music);
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	SetUp();
	
	program = new ShaderProgram(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	while (!done){
		ProcessEvents();
		Render(program);
	}
	SDL_Quit();
	return 0;
}
