// hw4.cpp : Defines the entry point for the console application.
//
#include "glad/glad.h"
#include "include\SDL.h"
#include "include\SDL_opengl.h"
  //Include order can matter here

#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include "stdafx.h"
#include <math.h>
using namespace std;

int screenWidth = 800;
int screenHeight = 800;
bool saveOutput = false;
float timePast = 0;
float viewRotate = 0.01;
bool getKey[5] = { FALSE, FALSE, FALSE, FALSE, FALSE};
bool doorOpened[5] = { FALSE, FALSE, FALSE, FALSE, FALSE };
int closing[5] = { 0, 0, 0, 0, 0 };
glm::vec3 keyAposition(0.0, 0.2, 0.0);

//SJG: Store the object coordinates
//You should have a representation for the state of each object

float colR = 1, colG = 1, colB = 1;

glm::vec3 position;  //Cam Position
glm::vec3 center(0, 0.2, 0);  //Look at point

bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;
void Win2PPM(int width, int height);

//srand(time(NULL));
float rand01() {
	return rand() / (float)RAND_MAX;
}

struct verticeAndIndices {
	GLfloat* vertices;
	GLuint* indices;
	int num_vertices;
	int num_indices;
};

void drawGeometry(int shaderProgram, int map_w, int map_h, char* map, GLuint* keyBuffer, GLuint* wallBuffer, GLuint* robotBuffer, verticeAndIndices wall, verticeAndIndices robot, verticeAndIndices key);

verticeAndIndices objLoader(char* path);

GLuint* newBufferForModel(verticeAndIndices model, GLuint programID);

bool detectCollision(glm::vec3 nextStep, char* map, int map_w, int map_h);

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)
	
							   //Ask SDL to get a recent version of OpenGL (3 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);//

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);

	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}
	
	// load map file

	FILE* mapFile = fopen("map.txt", "r");
	int map_w, map_h;
	fscanf(mapFile, "%d %d\n", &map_w, &map_h);
	cout << map_w << ' ' << map_h << endl;
	char* map = new char[map_w * map_h];
	for (int i = 0; i < (map_w * map_h); i++) {
		fscanf(mapFile, "%s\n", &map[i]);
		cout << map[i];
	}

	//Load Wall
	verticeAndIndices wall = objLoader("models/wall.obj");
	//Load Robot
	verticeAndIndices robot = objLoader("models/robot.obj");
	//load Key
	verticeAndIndices key = objLoader("models/key.obj");


	//// Allocate Texture 0 (Wood) ///////
	SDL_Surface* surface = SDL_LoadBMP("wood.bmp");
	if (surface == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex0;
	glGenTextures(1, &tex0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);

	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Load the texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);


	SDL_FreeSurface(surface);
	//// End Allocate Texture ///////


	//// Allocate Texture 1 (Brick) ///////
	SDL_Surface* surface1 = SDL_LoadBMP("brick.bmp");
	if (surface == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex1;
	glGenTextures(1, &tex1);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE1);

	glBindTexture(GL_TEXTURE_2D, tex1);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//How to filter
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w, surface1->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface1->pixels);


	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface1);
	//// End Allocate Texture ///////

	int texturedShader = InitShader("vertexTex.glsl", "fragmentTex.glsl");

	int phongShader = InitShader("vertex.glsl", "fragment.glsl");

	GLuint* wallBuffer = newBufferForModel(wall, texturedShader);

	GLuint* robotBuffer = newBufferForModel(robot, texturedShader);

	GLuint* keyBuffer = newBufferForModel(key, texturedShader);

	cout << wallBuffer[0] << ' ' << wallBuffer[1] << ' ' << wallBuffer[2] << endl;
	cout << robotBuffer[0] << ' ' << robotBuffer[1] << ' ' << robotBuffer[2] << endl;

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");

	glEnable(GL_DEPTH_TEST);

	int count_grid = 0;
	for (int i = map_h / -2; i < round(map_h / 2.0); i++) {
		for (int j = map_w / -2; j < round(map_w / 2.0); j++) {
			if (map[count_grid] == 'S') {

				position.x = 0.5 * j;
				position.y = 0.2;
				position.z = 0.5 * i;

				goto finishedPlacingView;
			}
			count_grid++;
		}
	}
finishedPlacingView:

	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&windowEvent)) {  //inspect all events in the queue
			if (windowEvent.type == SDL_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) { //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
			}

			//SJG: Use key input to change the state of the object
			//     We can use the ".mod" flag to see if modifiers such as shift are pressed
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_UP) { //If "up key" is pressed
				glm::vec3 direction = center - position;
				glm::vec3 temp(position.x + direction.x * 0.01, position.y, position.z + direction.z * 0.01);
				if (!detectCollision(temp, map, map_w, map_h)) {
					position.x = position.x + direction.x * 0.01;
					position.z = position.z + direction.z * 0.01;
					center.x = center.x + direction.x * 0.01;
					center.z = center.z + direction.z * 0.01;
					keyAposition.x = keyAposition.x + direction.x * 0.01;
					keyAposition.z = keyAposition.z + direction.z * 0.01;
				}
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_DOWN) { //If "down key" is pressed
				glm::vec3 direction = center - position;
				glm::vec3 temp(position.x - direction.x * 0.01, position.y, position.z - direction.z * 0.01);
				if (!detectCollision(temp, map, map_w, map_h)) {
					position.x = position.x - direction.x * 0.01;
					position.z = position.z - direction.z * 0.01;
					center.x = center.x - direction.x * 0.01;
					center.z = center.z - direction.z * 0.01;
					keyAposition.x = keyAposition.x - direction.x * 0.01;
					keyAposition.z = keyAposition.z - direction.z * 0.01;
				}
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_LEFT) { //If "LEFT key" is pressed
				glm::vec3 direction = center - position;
				glm::mat4 rot;
				rot = glm::rotate(rot, viewRotate * 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
				glm::vec4 new_direction = rot * glm::vec4(direction, 1);
				center.x = new_direction.x + position.x;
				center.z = new_direction.z + position.z;
				direction = keyAposition - position;
				new_direction = rot * glm::vec4(direction, 1);
				keyAposition.x = new_direction.x + position.x;
				keyAposition.z = new_direction.z + position.z;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_RIGHT) { //If "RIGHT key" is pressed
				glm::vec3 direction = center - position;
				glm::mat4 rot;
				rot = glm::rotate(rot, viewRotate * -3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
				glm::vec4 new_direction = rot * glm::vec4(direction, 1);
				center.x = new_direction.x + position.x;
				center.z = new_direction.z + position.z;
				direction = keyAposition - position;
				new_direction = rot * glm::vec4(direction, 1);
				keyAposition.x = new_direction.x + position.x;
				keyAposition.z = new_direction.z + position.z;
			}
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_c) { //If "c" is pressed
				colR = rand01();
				colG = rand01();
				colB = rand01();
			}

		}

		// Clear the screen to default color
		glClearColor(1.0f, 0.9f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(texturedShader);

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			cerr << "OpenGL error: " << err << endl;
		}

		if (!saveOutput) timePast = SDL_GetTicks() / 1000.f;
		if (saveOutput) timePast += .07; //Fix framerate at 14 FPS

		glm::mat4 view = glm::lookAt(
			position,						//Cam Position
			center,						//Look at point
			glm::vec3(0.0f, 1.0f, 0.0f));   //Up
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 proj = glm::perspective(3.14f / 4, screenWidth / (float)screenHeight, 0.01f, 10.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

		//navigation(position, map, map_w, map_h);
		drawGeometry(texturedShader, map_w, map_h, map, keyBuffer, wallBuffer, robotBuffer, wall, robot, key);


		if (saveOutput) Win2PPM(screenWidth, screenHeight);

		SDL_GL_SwapWindow(window); //Double buffering
	}

	//Clean Up
	glDeleteProgram(phongShader);
	//glDeleteBuffers(1, vbo);
	//glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}

void drawGeometry(int shaderProgram, int map_w, int map_h, char* map, GLuint* keyBuffer, GLuint* wallBuffer, GLuint* robotBuffer, verticeAndIndices wall, verticeAndIndices robot, verticeAndIndices key) {

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(colR, colG, colB);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");

	GLint uniModel = glGetUniformLocation(shaderProgram, "model");

	GLint uniKey = glGetUniformLocation(shaderProgram, "getKey");

	GLint cam = glGetUniformLocation(shaderProgram, "cam_position");
	glUniform3fv(cam, 1, glm::value_ptr(position));

	int count_grid = 0;
	for (int i = map_h / -2; i < round(map_h / 2.0); i++) {
		for (int j = map_w / -2; j < round(map_w / 2.0); j++) {
			glm::mat4 ori_model;
			if (map[count_grid] == 'W') {
				glBindVertexArray(wallBuffer[0]);										
				glBindBuffer(GL_ARRAY_BUFFER, wallBuffer[1]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallBuffer[2]);
				ori_model = glm::translate(ori_model, glm::vec3(0.5 * j, 0, 0.5 * i));
				ori_model = glm::scale(ori_model, glm::vec3(1, 1, 1));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(ori_model));
				glUniform1i(uniTexID, 1); //Set texture ID to use 				
				glDrawElements(GL_TRIANGLES, wall.num_indices, GL_UNSIGNED_INT, 0);
			}
			//else if (map[count_grid] == 'D') {
			//	glBindVertexArray(robotBuffer[0]);
			//	glBindBuffer(GL_ARRAY_BUFFER, robotBuffer[1]);
			//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, robotBuffer[2]);
			//	ori_model = glm::translate(ori_model, glm::vec3(0.5 * j, 0, 0.5 * i));
			//	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(ori_model));
			//	glUniform1i(uniTexID, 1); //Set texture ID to use 
			//	glDrawElements(GL_TRIANGLES, robot.num_indices, GL_UNSIGNED_INT, 0);
			//}
			else if (map[count_grid] >= 'a' && map[count_grid] <= 'e') {
				glBindVertexArray(keyBuffer[0]);
				glBindBuffer(GL_ARRAY_BUFFER, keyBuffer[1]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, keyBuffer[2]);
				if (!getKey[map[count_grid] - 'a']) {
					ori_model = glm::translate(ori_model, glm::vec3(0.5 * j, 0, 0.5 * i));
					ori_model = glm::rotate(ori_model, timePast * 3.14f / 2, glm::vec3(0.0f, 1.0f, 0.0f));
					glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(ori_model));
				}
				else {
					glm::vec3 cam2center;
					cam2center = center - position;
					float fov = 3.14f / 4;
					float sideLength = 0.01 / cos(fov / 2);
					glm::mat4 rot;
					rot = glm::rotate(rot, fov / 2, glm::vec3(0.0, 1.0, 0.0));
					glm::vec4 newd = rot * glm::vec4(cam2center, 1.0);
					glm::vec3 newdirect(newd.x, newd.y, newd.z);
					
					float norm2 = sqrt(pow(newdirect.x, 2) + pow(newdirect.y, 2) + pow(newdirect.z, 2));
					newdirect.x = newdirect.x / norm2 * (sideLength + 0.005);
					newdirect.y = newdirect.y / norm2 * (sideLength + 0.005);
					newdirect.z = newdirect.z / norm2 * (sideLength + 0.005);
					glm::mat4 rot2;
					rot2 = glm::rotate(rot2, fov * (-1), glm::vec3(0.0, 1.0, 0.0));
					glm::vec4 seconds = rot2 * glm::vec4(newdirect, 1.0);
					glm::vec3 secondside(seconds.x, seconds.y, seconds.z);
					glm::vec3 subtense = secondside - newdirect;
					subtense.x = subtense.x * 0.05 * (map[count_grid] - 'a' + 1);
					subtense.y = subtense.y * 0.05 * (map[count_grid] - 'a' + 1);
					subtense.z = subtense.z * 0.05 * (map[count_grid] - 'a' + 1);
					keyAposition = newdirect + subtense + position;

					ori_model = glm::translate(ori_model, glm::vec3(0.0f, 0.005f, 0.0f));
					ori_model = glm::translate(ori_model, keyAposition);
					ori_model = glm::rotate(ori_model, timePast * 3.14f / 2, glm::vec3(0.0f, 1.0f, 0.0f));
					ori_model = glm::scale(ori_model, glm::vec3(0.005f, 0.005f, 0.005f));
					ori_model = glm::translate(ori_model, glm::vec3(0.0f, -0.15f, 0.0f));
					glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(ori_model));
				}
				glUniform1i(uniKey, getKey[map[count_grid] - 'a']);
				glUniform1i(uniTexID, 'a' - map[count_grid] - 1); //Set texture ID to use
				glDrawElements(GL_TRIANGLES, key.num_indices, GL_UNSIGNED_INT, 0);
			}
			else if (map[count_grid] >= 'A' && map[count_grid] <= 'E') {
				int closingWay = 1;
				glBindVertexArray(wallBuffer[0]);
				glBindBuffer(GL_ARRAY_BUFFER, wallBuffer[1]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallBuffer[2]);
				ori_model = glm::translate(ori_model, glm::vec3(0.5 * j, 0, 0.5 * i));
				if (map[count_grid - 1] == '0') {
					ori_model = glm::rotate(ori_model, 3.14f / 2, glm::vec3(0.0, 1.0, 0.0));
					if (i == (map_h / -2)) {
						closingWay = -1;
					}
					else {
						closingWay = 1;
					}
				}
				if (doorOpened[map[count_grid] - 'A'] == TRUE) {
					ori_model = glm::translate(ori_model, glm::vec3(0.25 * closingWay, 0, 0));
					ori_model = glm::scale(ori_model, glm::vec3(1.0 - 0.0025 * closing[map[count_grid] - 'A'], 1.0, 1.0));
					ori_model = glm::translate(ori_model, glm::vec3(-0.25 * closingWay, 0, 0));
					if (closing[map[count_grid] - 'A'] < 400) {
						closing[map[count_grid] - 'A']++;
					}
				}
				ori_model = glm::scale(ori_model, glm::vec3(1.0, 1.0, 0.2));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(ori_model));
				glUniform1i(uniTexID, 'A' - map[count_grid] - 1); //Set texture ID to use 				
				
				glDrawElements(GL_TRIANGLES, wall.num_indices, GL_UNSIGNED_INT, 0);
			}
			count_grid++;
			glBindVertexArray(0);
		}
	}
	
	glBindVertexArray(wallBuffer[0]);										 
	glBindBuffer(GL_ARRAY_BUFFER, wallBuffer[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallBuffer[2]);
	glm::mat4 setGround;
	setGround = glm::scale(setGround, glm::vec3(11.0, 0.1, 5.0));
	setGround = glm::translate(setGround, glm::vec3(0.0, -0.25, 0.0));
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(setGround));
	glUniform1i(uniTexID, 0);
	glDrawElements(GL_TRIANGLES, wall.num_indices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

}


// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile) {
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

						 // allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

								  // append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName) {
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders

									// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);

	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}

void Win2PPM(int width, int height) {
	char outdir[10] = "out/"; //Must be exist!
	int i, j;
	FILE* fptr;
	static int counter = 0;
	char fname[32];
	unsigned char *image;

	/* Allocate our buffer for the image */
	image = (unsigned char *)malloc(3 * width*height * sizeof(char));
	if (image == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for image\n");
	}

	/* Open the file */
	sprintf(fname, "%simage_%04d.ppm", outdir, counter);
	if ((fptr = fopen(fname, "w")) == NULL) {
		fprintf(stderr, "ERROR: Failed to open file for window capture\n");
	}

	/* Copy the image into our buffer */
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);

	/* Write the PPM file */
	fprintf(fptr, "P6\n%d %d\n255\n", width, height);
	for (j = height - 1; j >= 0; j--) {
		for (i = 0; i<width; i++) {
			fputc(image[3 * j*width + 3 * i + 0], fptr);
			fputc(image[3 * j*width + 3 * i + 1], fptr);
			fputc(image[3 * j*width + 3 * i + 2], fptr);
		}
	}

	free(image);
	fclose(fptr);
	counter++;
}

verticeAndIndices objLoader(char* path) {
	verticeAndIndices vai;
	FILE* wallFile = fopen(path, "r");
	int num_vertices = 0, num_normals = 0, num_texture = 0, num_vertices_index = 0, num_texture_index = 0, num_normals_index = 0;
	if (wallFile == 0) {
		cout << "error";
	}
	while (1) {// first scan to get the numbers of vertices, triangles, normals and texture coordinates.
		char lineHeader[100];
		int res = fscanf(wallFile, "%s", lineHeader);
		if (res == EOF) {
			break;
		}
		if (strcmp(lineHeader, "v") == 0) {
			num_vertices++;
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			num_texture++;
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			num_normals++;
		}
		else if (strcmp(lineHeader, "f") == 0) {
			num_vertices_index++;
		}
	}
	float* wall_vertices = new float[num_vertices * 3];
	float* wall_texture = new float[num_texture * 2];
	float* wall_normals = new float[num_normals * 3];
	int* wall_vertices_index = new int[num_vertices_index * 3];
	int* wall_texture_index = new int[num_vertices_index * 3];
	int* wall_normals_index = new int[num_vertices_index * 3];
	num_vertices = 0, num_normals = 0, num_texture = 0, num_vertices_index = 0, num_texture_index = 0, num_normals_index = 0;
	rewind(wallFile);
	while (1) {// second scan to get the indices of vertices, triangles, normals and texture coordinates.
		char lineHeader[50];
		int res = fscanf(wallFile, "%s", lineHeader);
		if (res == EOF) {
			break;
		}
		if (strcmp(lineHeader, "v") == 0) {
			fscanf(wallFile, "%f %f %f\n", &wall_vertices[num_vertices], &wall_vertices[num_vertices + 1], &wall_vertices[num_vertices + 2]);
			wall_vertices[num_vertices] = wall_vertices[num_vertices] / 10.0;
			wall_vertices[num_vertices + 1] = wall_vertices[num_vertices + 1] / 10.0;
			wall_vertices[num_vertices + 2] = wall_vertices[num_vertices + 2] / 10.0;
			num_vertices = num_vertices + 3;
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			fscanf(wallFile, "%f %f\n", &wall_texture[num_texture], &wall_texture[num_texture + 1]);
			float norm2 = sqrt(pow(wall_texture[num_texture], 2) + pow(wall_texture[num_texture + 1], 2));
			wall_texture[num_texture] = wall_texture[num_texture] / norm2;
			wall_texture[num_texture + 1] = wall_texture[num_texture + 1] / norm2;
			num_texture = num_texture + 2;
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			fscanf(wallFile, "%f %f %f\n", &wall_normals[num_normals], &wall_normals[num_normals + 1], &wall_normals[num_normals + 2]);
			num_normals = num_normals + 3;
		}
		else if (strcmp(lineHeader, "f") == 0) {
			int vertexIndex[3], uvIndex[3], normalIndex[3];
			fscanf(wallFile, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			wall_vertices_index[num_vertices_index++] = vertexIndex[0];
			wall_vertices_index[num_vertices_index++] = vertexIndex[1];
			wall_vertices_index[num_vertices_index++] = vertexIndex[2];
			wall_texture_index[num_texture_index++] = uvIndex[0];
			wall_texture_index[num_texture_index++] = uvIndex[1];
			wall_texture_index[num_texture_index++] = uvIndex[2];
			wall_normals_index[num_normals_index++] = normalIndex[0];
			wall_normals_index[num_normals_index++] = normalIndex[1];
			wall_normals_index[num_normals_index++] = normalIndex[2];
		}
	}

	GLuint* rearranged_index = new GLuint[num_vertices_index];
	GLfloat* rearranged_vertices = new GLfloat[num_vertices_index * 8];
	int added_vertices = 0;
	for (int i = 0; i < num_normals_index; i++) {
		if (i != 0) {
			bool already_have = FALSE;
			for (int j = 0; j < added_vertices * 8; j = j + 8) {// See whether the vertices(position + texture + normal) are already in the rearranged array
				if (rearranged_vertices[j] == wall_vertices[wall_vertices_index[i] * 3 - 3] && rearranged_vertices[j + 1] == wall_vertices[wall_vertices_index[i] * 3 - 2] && rearranged_vertices[j + 2] == wall_vertices[wall_vertices_index[i] * 3 - 1] &&
					rearranged_vertices[j + 3] == wall_texture[wall_texture_index[i] * 2 - 2] && rearranged_vertices[j + 4] == wall_texture[wall_texture_index[i] * 2 - 1] &&
					rearranged_vertices[j + 5] == wall_normals[wall_normals_index[i] * 3 - 3] && rearranged_vertices[j + 6] == wall_normals[wall_normals_index[i] * 3 - 2] && rearranged_vertices[j + 7] == wall_normals[wall_normals_index[i] * 3 - 1]) {
					already_have = TRUE;
					rearranged_index[i] = j / 8;
					break;
				}
			}
			if (!already_have) {
				rearranged_vertices[added_vertices * 8] = wall_vertices[wall_vertices_index[i] * 3 - 3];
				rearranged_vertices[added_vertices * 8 + 1] = wall_vertices[wall_vertices_index[i] * 3 - 2];
				rearranged_vertices[added_vertices * 8 + 2] = wall_vertices[wall_vertices_index[i] * 3 - 1];
				rearranged_vertices[added_vertices * 8 + 3] = wall_texture[wall_texture_index[i] * 2 - 2];
				rearranged_vertices[added_vertices * 8 + 4] = wall_texture[wall_texture_index[i] * 2 - 1];
				rearranged_vertices[added_vertices * 8 + 5] = wall_normals[wall_normals_index[i] * 3 - 3];
				rearranged_vertices[added_vertices * 8 + 6] = wall_normals[wall_normals_index[i] * 3 - 2];
				rearranged_vertices[added_vertices * 8 + 7] = wall_normals[wall_normals_index[i] * 3 - 1];
				rearranged_index[i] = added_vertices;
				added_vertices++;
			}
		}
		else {
			rearranged_vertices[added_vertices * 8] = wall_vertices[wall_vertices_index[i] * 3 - 3];
			rearranged_vertices[added_vertices * 8 + 1] = wall_vertices[wall_vertices_index[i] * 3 - 2];
			rearranged_vertices[added_vertices * 8 + 2] = wall_vertices[wall_vertices_index[i] * 3 - 1];
			rearranged_vertices[added_vertices * 8 + 3] = wall_texture[wall_texture_index[i] * 2 - 2];
			rearranged_vertices[added_vertices * 8 + 4] = wall_texture[wall_texture_index[i] * 2 - 1];
			rearranged_vertices[added_vertices * 8 + 5] = wall_normals[wall_normals_index[i] * 3 - 3];
			rearranged_vertices[added_vertices * 8 + 6] = wall_normals[wall_normals_index[i] * 3 - 2];
			rearranged_vertices[added_vertices * 8 + 7] = wall_normals[wall_normals_index[i] * 3 - 1];
			added_vertices++;
			rearranged_index[0] = 0;
		}
	}

	delete wall_vertices;
	delete wall_texture;
	delete wall_normals;
	delete wall_vertices_index;
	delete wall_texture_index;
	delete wall_normals_index;

	GLfloat* r_vertices = new GLfloat[added_vertices * 8];
	for (int i = 0; i < added_vertices * 8; i = i + 8) {
		for (int j = 0; j < 8; j++) {
			r_vertices[i + j] = rearranged_vertices[i + j];
			//cout << r_vertices[i + j]<<' ';
		}
		//cout << endl;
	}
	/*for (int i = 0; i < num_normals_index; i = i + 3) {
	for (int j = 0; j < 3; j++) {
	cout << rearranged_index[i + j] << ' ';
	}
	cout << endl;
	}*/

	vai.vertices = r_vertices;
	vai.indices = rearranged_index;
	vai.num_vertices = added_vertices;
	vai.num_indices = num_normals_index;

	fclose(wallFile);
	return vai;
}

GLuint* newBufferForModel(verticeAndIndices model, GLuint programID) {
	GLuint* vbi = new GLuint[3];

	glGenVertexArrays(1, &vbi[0]); //Create a VAO
	glBindVertexArray(vbi[0]); //Bind the above created VAO to the current context

	glGenBuffers(1, &vbi[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vbi[1]);
	glBufferData(GL_ARRAY_BUFFER, model.num_vertices * 8 * sizeof(float), model.vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vbi[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbi[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.num_indices * sizeof(unsigned int), model.indices, GL_STATIC_DRAW);

	int posAttrib = glGetAttribLocation(programID, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
	glEnableVertexAttribArray(posAttrib);

	int normAttrib = glGetAttribLocation(programID, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	int texAttrib = glGetAttribLocation(programID, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	return vbi;
}

bool detectCollision(glm::vec3 nextStep, char* map, int map_w, int map_h) {

	int count_grid = 0;
	if(abs(nextStep.x) >= (map_w / 4.0) || abs(nextStep.z) >= (map_h / 4.0)){
		return TRUE;
	}
	for (int i = map_h / -2; i < round(map_h / 2.0); i++) {
		for (int j = map_w / -2; j < round(map_w / 2.0); j++) {
			glm::mat4 ori_model;
			if (map[count_grid] == 'W') {
				float norm1 = fmax(abs(nextStep.x - 0.5 * j) , abs(nextStep.z - 0.5 * i));
				//cout<< fmax(abs(nextStep.x - 0.5 * j), abs(nextStep.z - 0.5 * i))<<endl;
				if (norm1 <= 0.26) {
					return TRUE;
				}
			}
			else if (map[count_grid] <= 'e' && map[count_grid] >= 'a') {
				float norm1 = fmax(abs(nextStep.x - 0.5 * j), abs(nextStep.z - 0.5 * i));
				if (norm1 <= 0.26) {
					getKey[map[count_grid] - 'a'] = TRUE;
				}			
			}
			else if (map[count_grid] <= 'E' && map[count_grid] >= 'A') {
				float norm1 = fmax(abs(nextStep.x - 0.5 * j), abs(nextStep.z - 0.5 * i));
				if (norm1 <= 0.26) {
					if (getKey[map[count_grid] - 'A'] == TRUE){
						doorOpened[map[count_grid] - 'A'] = TRUE;
						return FALSE;
					}
					else {
						return TRUE;
					}
				}
			}
			count_grid++;
		}
	}
	return FALSE;
}

