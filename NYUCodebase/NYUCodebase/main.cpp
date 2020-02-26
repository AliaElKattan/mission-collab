#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <string.h>
#include <vector>
#include <SDL_mixer.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window* displayWindow;


//Load textures and images
GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

//Standard rectangular object
class Plank{ //this ended up being my class for all untextured rectangular objects
public: 
	Plank();
	void drawPlank(ShaderProgram &program, float x, float y);
	float yposition;
	float xposition;
	bool isColliding(Plank rain); //for stuff falling
	//bool isLanded(float x, float x2, float y); //check if it's colliding w/ the players
	bool isLanded(float x, float y); //check if it's colliding w/ the players
};


//Sprite
class Entity {

private:
	int index;
	int spriteCountX;
	int spriteCountY;
public:
	Entity();
	void DrawSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCount);
	bool isColliding(float x, float y, float plankx, float planky); 
	bool isAlive = true;
};

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing);


int main(int argc, char *argv[])
{

	//initialize audio
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Chunk *beepSound, *loseSound, *winSound;
	Mix_Music *introMusic;
	introMusic = Mix_LoadMUS("intro_music.wav");
	beepSound = Mix_LoadWAV("beep_sound.wav");
	loseSound = Mix_LoadWAV("lose.wav");
	winSound = Mix_LoadWAV("bonus.wav");


	//ASSIGN VARIABLES
	bool falling = false;
	float angle;
	float degrees = 0;
	int maxrow;
	int gameState = 0; //gamestate 0 = opening, 1 = main game, 2 = you win, 3 = you lose
	int miss = 0;
	bool collision = false;



	//float planky = 1.5f;
	//initial location of plank
	float plankx_r;
	float plankx_l;
	float playerx = -1.7f;
	float playery = -.7f;
	float playerx2 = -1.5f;
	float playery2 = -.7f;
	bool musicOn = false;
	int level = 1;

	//SETUP
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Alia's Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	//SHADERS

	ShaderProgram textured_program;
	ShaderProgram untext_program;
	textured_program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl"); //for textured
	untext_program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl"); //for untextured


	GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"font2.png");

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix2 = glm::mat4(1.0f);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	


	glEnable(GL_BLEND);


	GLuint spritesheet = LoadTexture(RESOURCE_FOLDER"characters_1.png");
	Entity player;
	Entity player2;
	Entity enemies[8][4];
	Plank plank;
	Plank scoreboard;
	Plank goal;
	Plank goal2;
	Plank floor;
	Plank floor2;
	Plank box;
	Plank box2;
	plank.yposition = .8;
	plank.xposition = 0.f;
	box.xposition = plank.xposition;
	box.yposition = plank.yposition + .02;
	float box_xmovement = 0;
	float box_ymovement = 0;


	float gravity = -.01;
	float velocity = 0;
	float jumpVelocity = .07f;
	float playerY = 0;



	Plank rain[5], rain2[5];

	//set them with a for loop instead
	rain[0].xposition = -1;
	rain[1].xposition = -.7;
	rain[2].xposition = -.4;
	rain[3].xposition = .4;
	rain[4].xposition = .7;

	rain2[0].xposition = -1.4;
	rain2[1].xposition = -.9;
	rain2[2].xposition = -.3;
	rain2[3].xposition = .5;
	rain2[4].xposition = 1.4;


	rain[0].yposition = 1.;
	rain[1].yposition = 1.2;
	rain[2].yposition = 1.4;
	rain[3].yposition = 1.7;
	rain[4].yposition = 1.9;

	rain2[0].yposition = 1.1;
	rain2[1].yposition = 1.5;
	rain2[2].yposition = 1.2;
	rain2[3].yposition = 1.;
	rain2[4].yposition = 1.9;



	SDL_Event event;

	
	float lastFrameTicks = 0.0f;
	bool done = false;
	while (!done) {

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			else if (event.type == SDL_KEYDOWN) {

			}


		}


		//change background color depending on screen
		if (gameState == 0) {
			glClearColor(0.09f, 0.09f, .09f, 1.0f); //BACKGROUND COLOUR
			glClear(GL_COLOR_BUFFER_BIT);
		}

		if (gameState>0) {

			if (level == 1) {
				glClearColor(0.09f, 0.09f, .3f, 1.0f); //BACKGROUND COLOUR
				glClear(GL_COLOR_BUFFER_BIT);
			}

			if (level == 2) {
				glClearColor(0.09f, 0.2f, .09f, 1.0f); //BACKGROUND COLOUR
				glClear(GL_COLOR_BUFFER_BIT);
			}

			if (level == 3) {
				glClearColor(0.25f, 0.09f, .09f, 1.0f); //BACKGROUND COLOUR
				glClear(GL_COLOR_BUFFER_BIT);
			}
		}
	
		textured_program.SetModelMatrix(modelMatrix);
		textured_program.SetProjectionMatrix(projectionMatrix);
		textured_program.SetViewMatrix(viewMatrix);

		untext_program.SetModelMatrix(modelMatrix);
		untext_program.SetProjectionMatrix(projectionMatrix);
		untext_program.SetViewMatrix(viewMatrix);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		//press Q to exit
		if (keys[SDL_SCANCODE_Q]) {
			exit(0);
		}

		//title screen
		if (gameState == 0) {

			if (keys[SDL_SCANCODE_SPACE]) {
				gameState = 1;
			}

			if (musicOn == false) {
				Mix_PlayMusic(introMusic, 1);
				musicOn = true;
			}
			glUseProgram(textured_program.programID);
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.00f, .3f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "MISSION COLLAB", .15f, .001f);

			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-.83f, .0f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "use WASD and arrow keys to move", .07f, -.009f);

			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-.5f, -.15f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "press Q to exit", .07f, -.009f);


			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-.75f, -.5f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "press space to start", .08f, -.001f);

		}

		if (gameState == 1) { //game on
			
				glBindTexture(GL_TEXTURE_2D, spritesheet);
			modelMatrix = glm::mat4(1.0f);

			//move player
			if (keys[SDL_SCANCODE_LEFT]) {
				if (playerx > -1.7f)
					playerx -= elapsed;

				modelMatrix = glm::translate(modelMatrix, glm::vec3(playerx, playery, 1.0f)); //have this multiple times to change the sprite depending on the direction of movement
				textured_program.SetModelMatrix(modelMatrix);
				player.DrawSprite(textured_program, 77, 12, 8);
				// go left
			}
			else if (keys[SDL_SCANCODE_RIGHT]) {
				// go right

				if (playerx < 1.7f)
					playerx += elapsed;

				modelMatrix = glm::translate(modelMatrix, glm::vec3(playerx, playery, 1.0f));
				textured_program.SetModelMatrix(modelMatrix);
				player.DrawSprite(textured_program, 17, 12, 8);
			}

			//if standing
			else {//draw player 1
				modelMatrix = glm::translate(modelMatrix, glm::vec3(playerx, playery, 1.0f));
				textured_program.SetModelMatrix(modelMatrix);
				player.DrawSprite(textured_program, 65, 12, 8);
			}

			if (keys[SDL_SCANCODE_A]) {
				if (playerx2 > -1.7f)
					playerx2 -= elapsed;

				modelMatrix = glm::mat4(1.0f);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(playerx2, playery2, 1.0f));
				textured_program.SetModelMatrix(modelMatrix);
				player2.DrawSprite(textured_program, 8, 12,8);
				// go left
			}
			else if (keys[SDL_SCANCODE_D]) {
				// go right

				if (playerx2 < 1.7f)
					playerx2 += elapsed;

				modelMatrix = glm::mat4(1.0f);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(playerx2, playery2, 1.0f));
				textured_program.SetModelMatrix(modelMatrix);
				player2.DrawSprite(textured_program, 20, 12, 8);
			}

			//if standing
			else {
				//draw player 2
				modelMatrix = glm::mat4(1.0f);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(playerx2, playery2, 1.0f));
				textured_program.SetModelMatrix(modelMatrix);
				player2.DrawSprite(textured_program, 68, 12, 8);
			}


			//plank

			plank.xposition = (playerx + playerx2) / 2; //center plank between both players


			plankx_r = plank.xposition + .3f; //right edge of plank
			plankx_l = plank.xposition - .3f;//left edge of plank


			//if plank is landed on both players
			if (plank.isLanded(playerx, playery) == true && plank.isLanded(playerx2, playery) == true) { //this is added for testing
				//dont fall
				/*untext_program.SetColor(1.f, 0.f, 1.f, 1.0f);*/
			}

			//if plank isnt touching either player

			//this used to be 1 function that checks if its touching both, but wanted to separate it for clarity
			if (plank.isLanded(playerx, playery) == false && plank.isLanded(playerx2, playery) == false) { //this is added for testing
			//fall
				plank.yposition -= elapsed * 1.5;
			}

			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(plank.xposition, plank.yposition, 1.0f));


			//if players are too close to each other (plank not balanced)
			if (abs(playerx - playerx2) < .15) {
				degrees -= .05;
			}


			//if players are a bit farther away from each other (plank balanced)
			
			if (abs(playerx - playerx2) > .15) {
								
				if (degrees > 0) {
					//degrees += elapsed * 4;
					degrees -= .05;
				}

				if (degrees < 0) {

					//degrees += elapsed * 4;
					degrees += .05;
					/*if (box_xmovement > 0) {
						box_xmovement -= elapsed * .05;

					}*/
					
					
				}

			}



			angle = degrees * (3.1415926f / 180.0f);
			modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(2.5f, .4f, 1.0f));

			untext_program.SetModelMatrix(modelMatrix);
			untext_program.SetColor(.1f, .05f, .05f, 1.f);
			plank.drawPlank(untext_program, 0, -.4);


			modelMatrix = glm::mat4(1.0f);

			//box is placed on plank, box_xmovement & box_ymovement are what makes it slide
			box.xposition = plank.xposition + box_xmovement;

			if (box.xposition < plankx_r && box.xposition> plankx_l) { //if box is on plank
				box.yposition = plank.yposition + .1 + box_ymovement;
			}


			//tan(angle) = opposite/adjacent, which means y = tan(angle)x. as a ratio, x is 1, so I don't multiply by it
			//reverse movement when users start balancing the plank

			if (abs(playerx - playerx2) <.15) {
				if (degrees < 0) {
					box_ymovement += ((elapsed * .26 * -angle) * tan(angle));  //multiplying by the angle so the speed of movement is dependent on how big the angle is
					box_xmovement += (elapsed * .23 * -angle);
				}

				if (degrees > 0) {
					box_ymovement -= ((elapsed * .26 * -angle) * tan(angle));
					box_xmovement -= (elapsed * .23 * -angle);
				}
			}

			if (abs(playerx - playerx2) >.15) {
				if (degrees >0) {
					box_ymovement += ((elapsed * .26 * -angle) * tan(angle));  //multiplying by the angle so the speed of movement is dependent on how big the angle is
					box_xmovement += (elapsed * .23 * -angle);
				}

				if (degrees < 0) {
					box_ymovement -= ((elapsed * .26 * -angle) * tan(angle));
					box_xmovement -= (elapsed * .23 * -angle);
				}


			}
			



			if (box.xposition - .01 > plankx_r || box.xposition + .01 < plankx_l) { //if box is falling
				box.yposition -= elapsed * 1.5;
			}

			modelMatrix = glm::translate(modelMatrix, glm::vec3(box.xposition, box.yposition, 1.0f));
			modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));

			modelMatrix = glm::scale(modelMatrix, glm::vec3(.65f, .65f, 1.0f));

			untext_program.SetColor(1., .0f, .0f, 1.f);

			untext_program.SetModelMatrix(modelMatrix);
			box.drawPlank(untext_program, 0, -.4);


			modelMatrix = glm::mat4(1.0);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(box.xposition, box.yposition, 1.0f));
			modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));

			modelMatrix = glm::scale(modelMatrix, glm::vec3(.6f, .6f, 1.0f));


			untext_program.SetColor(.8f, .7f, .6f, 1.f);

			untext_program.SetModelMatrix(modelMatrix);
			box2.drawPlank(untext_program, 0, -.4);


			glUseProgram(textured_program.programID);
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.6f, .85f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);


			//if (plank.xposition > 1.5) { //go to next level pointer
			if (plank.xposition > 1.7) { //go to next level
				Mix_PlayChannel(-1, winSound, 0);
				miss = 1;


				falling = false;

				playerx = -1.7;
				playerx2 = -1.5;

				plank.yposition = .8f;
				plank.xposition = (playerx + playerx2) / 2;

				degrees = 0;

				box.xposition = plank.xposition;
				box.yposition = plank.yposition + .02;

				box_xmovement = 0;
				box_ymovement = 0;


				if (level == 3) {
					gameState = 3;
				}


				if (level < 3)
					level++;
			}

		

		//if (miss < 3 && (plank.yposition < -1.777 || box.yposition < -1.777)) { //restart level if they lose

		if (box.yposition < -1.1) { //restart level if they lose
			Mix_PlayChannel(-1, loseSound, 0);
			falling = false;

			playerx = -1.7;
			playerx2 = -1.5;

			plank.yposition = .8f;
			plank.xposition = (playerx + playerx2) / 2;

			degrees = 0;

			box.xposition = plank.xposition;
			box.yposition = plank.yposition + .02;

			box_xmovement = 0;
			box_ymovement = 0;
			miss++;

		}




		modelMatrix = glm::mat4(1.0f);

		//modelMatrix = glm::translate(modelMatrix, glm::vec3(0,0, 1.0f));
		untext_program.SetColor(0.f, .0f, .0f, .7f);


		modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.37, .85, 1.0f));

		modelMatrix = glm::scale(modelMatrix, glm::vec3(2.7f, .4f, 1.0f));

		untext_program.SetModelMatrix(modelMatrix);
		scoreboard.drawPlank(untext_program, 0, -.4);



		modelMatrix = glm::mat4(1.0f);
		untext_program.SetColor(1.f, 1.f, 1.f, 1.f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -.9, 1.0f));

		modelMatrix = glm::scale(modelMatrix, glm::vec3(20.f, .9f, 1.0f));

		untext_program.SetModelMatrix(modelMatrix);
		floor.drawPlank(untext_program, 0, 0);



		modelMatrix = glm::mat4(1.0f);
		untext_program.SetColor(0.05f, .05f, .05f, 1.f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -.9, 1.0f));

		modelMatrix = glm::scale(modelMatrix, glm::vec3(20.f, .85f, 1.0f));

		untext_program.SetModelMatrix(modelMatrix);
		floor.drawPlank(untext_program, 0, 0);



		modelMatrix = glm::mat4(1.0f);

		untext_program.SetColor(1.f, .0f, .0f, 1.f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(1.77, -.45, 1.0f));

		modelMatrix = glm::scale(modelMatrix, glm::vec3(.8f, .8f, 1.0f));

		untext_program.SetModelMatrix(modelMatrix);
		goal2.drawPlank(untext_program, 0, 0);


		modelMatrix = glm::mat4(1.0f);
		untext_program.SetColor(1.f, .7f, .0f, 1.f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(1.77, -.45, 1.0f));

		modelMatrix = glm::scale(modelMatrix, glm::vec3(.7f, .7f, 1.0f));

		untext_program.SetModelMatrix(modelMatrix);
		goal.drawPlank(untext_program, 0, 0);



		if (miss == 0) {
			DrawText(textured_program, fontTexture, "LIVES: 3", .07f, -.005f);
		}

		if (miss == 1) {
			DrawText(textured_program, fontTexture, "LIVES: 2", .07f, -.005f);
		}

		if (miss == 2) {
			DrawText(textured_program, fontTexture, "LIVES: 1", .07f, -.005f);
		}

		if (miss == 3) {

			gameState = 4;
		}

		//^read an int into a string instead

		glUseProgram(untext_program.programID);
		/*untext_program.SetColor(1.f, .2f, .2f, 1.0f);*/


		if (level == 2) {

			miss = 0;

			modelMatrix = glm::mat4(1.0f);

			//modelMatrix = glm::translate(modelMatrix, glm::vec3(0,0, 1.0f));
			untext_program.SetColor(1.f, 1.f, .4f, 1.0f);

			modelMatrix = glm::translate(modelMatrix, glm::vec3(rain[1].xposition, rain[1].yposition, 1.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(.1f, .4f, 1.0f));

			untext_program.SetModelMatrix(modelMatrix);

			for (int j = 0; j < 5; j++) {
				rain[j].drawPlank(untext_program, rain[j].xposition, rain[j].yposition);

				if (rain[j].yposition > -1.85) {
					rain[j].yposition -= elapsed;
				}

				if (rain[j].yposition < -1.85) {
					rain[j].yposition = 1.9f;
					rain[j].xposition = rain[j + 1].xposition / 2;
				}

				if (plank.isColliding(rain[j]) == true) {
					collision = true;
				}

			}


			if (collision == true) {
				
				Mix_PlayChannel(-1, loseSound, 0);
				collision = false;
				falling = false;

				playerx = -1.7;
				playerx2 = -1.5;

				plank.yposition = .8f;
				plank.xposition = (playerx + playerx2) / 2;

				degrees = 0;

				box.xposition = plank.xposition;
				box.yposition = plank.yposition + .02;

				box_xmovement = 0;
				box_ymovement = 0;
				miss++;

			}


			//untext_program.SetColor(1.f, .2f, .2f, 1.0f);


		}


		if (level == 3) {

			miss = 2;

			modelMatrix = glm::mat4(1.0f);

			//modelMatrix = glm::translate(modelMatrix, glm::vec3(0,0, 1.0f));
			untext_program.SetColor(1.f, 1.f, .4f, 1.0f);

			modelMatrix = glm::translate(modelMatrix, glm::vec3(rain2[1].xposition, rain2[1].yposition, 1.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(.8f, .8f, 1.0f));

			untext_program.SetModelMatrix(modelMatrix);

			for (int m = 0; m < 5; m++) {
				rain2[m].drawPlank(untext_program, rain2[m].xposition, rain2[m].yposition);

				if (rain2[m].yposition > -1.1) {
					rain2[m].yposition -= elapsed;
				}

				if (rain2[m].yposition < -1.1) {
					rain2[m].yposition = 1.2f;
					rain2[m].xposition = rain2[m + 1].xposition / 2;
				}

				if (plank.isColliding(rain2[m]) == true) {
					collision = true;
				}

			}


			if (collision == true) {
				Mix_PlayChannel(-1, loseSound, 0);
				collision = false;
				falling = false;

				playerx = -1.7;
				playerx2 = -1.5;

				plank.yposition = .8f;
				plank.xposition = (playerx + playerx2) / 2;

				degrees = 0;

				box.xposition = plank.xposition;
				box.yposition = plank.yposition + .02;

				box_xmovement = 0;
				box_ymovement = 0;
				miss++;

			}


			}


		//if (plank.xposition < -1.2) {

			glUseProgram(textured_program.programID);
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-.48f, .3f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);

			if (level == 1) {
				DrawText(textured_program, fontTexture, "LEVEL 1", .15f, .001f);
			}

			if (level == 2) {
				DrawText(textured_program, fontTexture, "LEVEL 2", .15f, .001f);
			}

			if (level == 3) {
				DrawText(textured_program, fontTexture, "LEVEL 3", .15f, .001f);
			}
		//}



		}

	
		if (gameState == 3) {

			glUseProgram(textured_program.programID);
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.05f, .3f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "CONGRATULATIONS", .15f, .001f);

			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-.3f, .0f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "You win!", .08f, .0f);

			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.f, -.3f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "press space to try again", .08f, .0f);


			if (keys[SDL_SCANCODE_SPACE]) {
				gameState = 1;
				level = 1;
				falling = false;

				playerx = -1.7;
				playerx2 = -1.5;

				plank.yposition = .8f;
				plank.xposition = (playerx + playerx2) / 2;

				degrees = 0;

				box.xposition = plank.xposition;
				box.yposition = plank.yposition + .02;

				box_xmovement = 0;
				box_ymovement = 0;
				miss = 0;
			}

		}

		if (gameState == 4) {

			glUseProgram(textured_program.programID);
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-.8f, .3f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "GAME OVER", .18f, .001f);

			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.35f, .0f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "You lose.", .08f, .0f);

			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.f, -.3f, 1.0f));
			textured_program.SetModelMatrix(modelMatrix);
			DrawText(textured_program, fontTexture, "press space to try agian", .08f, .0f);


			if (keys[SDL_SCANCODE_SPACE]) {
				gameState = 1;
				level = 1;
				falling = false;

				playerx = -1.7;
				playerx2 = -1.5;

				plank.yposition = .8f;
				plank.xposition = (playerx + playerx2) / 2;

				degrees = 0;

				box.xposition = plank.xposition;
				box.yposition = plank.yposition + .02;

				box_xmovement = 0;
				box_ymovement = 0;
				miss = 0;
			}
		}


		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDisableVertexAttribArray(textured_program.positionAttribute);
		glDisableVertexAttribArray(textured_program.texCoordAttribute);


		SDL_GL_SwapWindow(displayWindow);
	}

	Mix_FreeChunk(winSound);
	Mix_FreeChunk(beepSound);
	Mix_FreeChunk(loseSound);
	Mix_FreeMusic(introMusic);

	SDL_Quit();

	return 0;
}

Entity::Entity() {
	
};

bool Entity::isColliding(float x, float y, float plankx, float planky) {

	if (plankx > (x - .1) && plankx<(x + .1) && planky>(y - .1) && planky < (y + .1)) {
		return true;
	}
	else
		return false;
}

void Entity::DrawSprite(ShaderProgram &program, int i, int scX, int scY) {

	
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(.0f, .70f, 1.0f));

		index = i;
		spriteCountX = scX;
		spriteCountY = scY;

		float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
		float v = (float)(((int)index) / spriteCountY) / (float)spriteCountY;
		float spriteWidth = 1.0 / (float)spriteCountX;
		float spriteHeight = 1.0 / (float)spriteCountY;

		float texCoords[] = {
			u, v + spriteHeight,
			u + spriteWidth, v,
			u, v,
			u + spriteWidth, v,
			u, v + spriteHeight,
			u + spriteWidth, v + spriteHeight };

		float vertices[] = { -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 0.1f,  -0.1f,-0.1f, 0.1f, -0.1f };

		glUseProgram(program.programID);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
	}
	



Plank::Plank() {
}



void Plank::drawPlank(ShaderProgram &untext_program, float plankx, float planky) {

/*
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(plankx, planky, 1.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.3f, .4f, 1.0f));*/

		float plank[] = { .1f,-.1f,.1f,.1f,-.1f,-.1f,-.1f,-.1f,.1f,.1f,-.1f,.1f };
		glVertexAttribPointer(untext_program.positionAttribute, 2, GL_FLOAT, false, 0, plank);
		glEnableVertexAttribArray(untext_program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(untext_program.positionAttribute);
	
}

bool Plank::isColliding(Plank rain) {
	float x_left = xposition - .05f;
	float x_right = xposition + .05f;
	//check for all objects falling down
	//for (int i = 0; i < 10; i++) {
		/*if (rain[i].xposition > x_left && rain[i].xposition < x_right && rain[i].yposition + .1 <yposition - .2) {
			return true;*/

	if (rain.xposition > x_left && rain.xposition < x_right && rain.yposition < yposition && rain.yposition > -.9) {
		return true;

		//}
	}

	return false;
}

//bool Plank::isLanded(float player1x, float player2x, float playery) {
//	if (yposition < (playery + .3f) && yposition > playery && ((xposition < player1x && xposition > player2x) || (xposition > player1x && xposition < player2x))) {
//		return true;
//	}
//	else return false;
//}
 

bool Plank::isLanded(float playerx, float playery) {
	float x_left = xposition - .3f;
	float x_right = xposition + .3f;

	if (playerx > x_left && playerx < x_right && yposition < (playery + .14f) && yposition > playery) {
		return true;
	}
	else return false;
}


void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
		((size + spacing) * i) + (-0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
		texture_x, texture_y,
		texture_x, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x + character_size, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x, texture_y + character_size,
			});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);


	glUseProgram(program.programID);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program.positionAttribute);

}