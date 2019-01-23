// Based loosly on the first triangle OpenGL tutorial
// http://www.opengl.org/wiki/Tutorial:_OpenGL_3.1_The_First_Triangle_%28C%2B%2B/Win%29
// This program will render two triangles
// Most of the OpenGL code for dealing with buffer objects, etc has been moved to a 
// utility library, to make creation and display of mesh objects as simple as possible

//only load console window during debug mode
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#include "rt3d.h"
#include "rotatingCube.h"
#include "bass.h"
#include "rt3dObjLoader.h"
#include "md2model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include <Windows.h>
#include <mmsystem.h>
#include <iostream>
#include <vector>
#include <SDL_ttf.h>

using namespace std;
using namespace glm;

//positioning the walls
glm::vec3 wall1Position(0.0f, -3.0f, -50.0f);
glm::vec3 wall2Position(-50.0f, -3.0f, -0.0f);
glm::vec3 wall3Position(50.0f, -3.0f, -0.0f);
glm::vec3 wall4Position(0.0f, -3.0f, 50.0f);

//booleans used for allowing sound files to be played
bool allowWalkSound = true, allowJumpSound = true;

//class which contains rotating cube
rotatingCube* rotCube;

//Used by BASS library
HSAMPLE *samples = NULL;

//used for labels
TTF_Font* textFont;

// md2 stuff
md2model tmpModel;
int currentAnim = 0;
GLuint meshIndexCount = 0;
GLuint md2VertCount = 0;

//info for 3D cubes
GLuint cubeIndexCount = 36;
GLuint cubeVertCount = 8;

//array of mesh objects
GLuint meshObjects[2];

//array of textures
GLuint textures[10];

//array of indices for cube
GLuint cubeIndices[] = {0, 1, 2,
						0, 2, 3,
						1, 0, 5,
						0, 4, 5,
						6, 3, 2,
						3, 6, 7,
						1, 5, 6,
						1, 6, 2,
						0, 3, 4,
						3, 7, 4,
						6, 5, 4,
						7, 6, 4};

//array for colours if textures aren't used
GLfloat colours[] = {	1.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 1.0f,
						0.0f, 0.0f, 0.0f
};

//array of vertices
GLfloat cubeVertices[] = {	-0.5f, -0.5f, -0.5f,
							-0.5f,  0.5f, -0.5f,
							 0.5f,  0.5f, -0.5f,
							 0.5f, -0.5f, -0.5f, 
							-0.5f, -0.5f, 0.5f,
							-0.5f,  0.5f, 0.5f,
							 0.5f,  0.5f, 0.5f,
							 0.5f, -0.5f, 0.5f
};

//used by the shaders
GLuint ShaderProgram;

//used for moving the player
GLfloat dx = 0.0f, dy = 1.2f, dz = -5.0f;

//used for rotating the camera
GLfloat rotateValueX = 0.0f;

//used to save mouse x position
float x_prev = 0.0f;

//light and materials, to deternmine how shiny etc objects are
rt3d::lightStruct light0;
rt3d::materialStruct material0;
rt3d::materialStruct materialSkyBox;
rt3d::lightStruct lightSkyBox;

//stack of mat4's
std::stack<glm::mat4> mvStack;

//array of Labels
GLuint labels[5];

//used by lookAt function
glm::vec3 eye(2.0f, 8.0f, 8.0f);
glm::vec3 at(0.0f, 1.0f, 1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);

//Set up rendering context
SDL_Window * setupRC(SDL_GLContext &context) {
	SDL_Window * window;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) // Initialize video
        rt3d::exitFatalError("Unable to initialize SDL"); 

	
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); 

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // Turn on x4 multisampling anti-aliasing (MSAA)

	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering
 
    //Create window
    window = SDL_CreateWindow("Real-Time 3D Coursework", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1400, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
	if (!window) // Check window was created OK
        rt3d::exitFatalError("Unable to create window");
 
    context = SDL_GL_CreateContext(window); // Create opengl context and attach to window
    SDL_GL_SetSwapInterval(1); // set swap buffers to sync with monitor's vertical refresh rate
	return window;
}

//Loads textures
GLuint loadBitmap(char *fname)
{
	GLuint texID;
	glGenTextures(1, &texID); // generate texture ID

	// load file - using core SDL library
	SDL_Surface *tmpSurface;
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface)
	{
		std::cout << "Error loading bitmap" << std::endl;
	}

	// bind texture and set parameters
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	SDL_PixelFormat *format = tmpSurface->format;
	GLuint externalFormat, internalFormat;
	
	if (format->Amask) {
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGRA;
	}
	else {
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tmpSurface->w, tmpSurface->h, 0,
		externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(tmpSurface); // texture loaded, free the temporary buffer
	return texID;	// return value of texture ID
}

//Loads the sound files
HSAMPLE loadSample(char * filename)
{
	HSAMPLE sam;
	if (sam = BASS_SampleLoad(FALSE, filename, 0, 0, 3, BASS_SAMPLE_OVER_POS))
		cout << "sample " << filename << " loaded!" << endl;
	else
	{
		cout << "Can't load sample";
		exit(0);
	}
	return sam;
}



//initialiser
void init(void) {
	
	//Shader programs
	ShaderProgram = rt3d::initShaders("phong-tex.vert", "phong-tex.frag");

	//Label initialiser
	if (TTF_Init() == -1)
		cout << "TTF failed to initialise." << endl;

	//only font used
	textFont = TTF_OpenFont("ABOVE.ttf", 50);
	if (textFont == NULL)
		cout << "Failed to open font." << endl;

	//enabling GL functions
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	light0 = {

		{ 0.2f, 0.2f, 0.2f, 1.0f }, // ambient

		{ 0.7f, 0.7f, 0.7f, 1.0f }, // diffuse

		{ 0.8f, 0.8f, 0.8f, 1.0f }, // specular

		{ 0.0f, 0.0f, 1.0f, 1.0f } // position

	};

	material0 = {

		{ 0.4f, 0.2f, 0.2f, 1.0f }, // ambient

		{ 0.8f, 0.5f, 0.5f, 1.0f }, // diffuse

		{ 1.0f, 0.8f, 0.8f, 1.0f }, // specular

		5.0f // shininess
	};

	materialSkyBox = {
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // ambient
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // specular
		1.0f  // shininess
	};

	lightSkyBox = {
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // ambient
		{ 0.0f, 0.0f, 0.0f, 1.0f }, // diffuse
		{ 0.0f, 0.0f, 0.0f, 1.0f }, // specular
		{ 0.0f, 1.0f, 0.0f, 1.0f }  // position
	};

	rt3d::setLight(ShaderProgram, light0);
	rt3d::setMaterial(ShaderProgram, material0);

	//initialising the bitmaps
	textures[0] = loadBitmap("Textures/fabric.bmp");
	textures[1] = loadBitmap("Textures/studdedmetal.bmp");
	textures[2] = loadBitmap("Textures/Coin.bmp");
	textures[3] = loadBitmap("Textures/seamlessSky.bmp");

	//Initialize default output device
	if (!BASS_Init(-1, 44100, 0, 0, NULL))
		cout << "Can't initialize device";

	//array of sound  files
	samples = new HSAMPLE[2];

	//starting the array of labels
	labels[0] = 0;

	//adding sound files to the array to be played later in code
	samples[0] = loadSample("SoundFiles/jump.wav");
	samples[1] = loadSample("SoundFiles/walk.wav");

	//texture data
	GLfloat cubeTexCoords[] = { 0.0f, 0.0f,
								0.0f, 1.0f,
								1.0f, 1.0f,
								1.0f, 0.0f,
								1.0f, 1.0f,
								1.0f, 0.0f,
								0.0f, 0.0f,
								0.0f, 1.0f
	};

	//first mesh
	meshObjects[0] = rt3d::createMesh(cubeVertCount, cubeVertices, nullptr, cubeVertices, cubeTexCoords, cubeIndexCount, cubeIndices);

	//loading in and initialising the rotating cube
	rotCube = new rotatingCube();

	rotCube->init();
	
	//setting up the classes shader program
	rotCube->Set_ShaderID(ShaderProgram);

	//used by the 3D model
	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;
	vector<GLuint> indices;
	rt3d::loadObj("cube.obj", verts, norms, tex_coords, indices);
	GLuint size = indices.size();
	meshIndexCount = size;
	meshObjects[1] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), size, indices.data());

	textures[1] = loadBitmap("hobgoblin2.bmp");
	meshObjects[1] = tmpModel.ReadMD2Model("tris.MD2");
	md2VertCount = tmpModel.getVertDataCount();
}

// textToTexture
GLuint textToTexture(const char * str, GLuint textID) {
	GLuint texture = textID;
	TTF_Font * font = textFont;

	SDL_Surface * stringImage = TTF_RenderText_Blended(font, str, { 255, 255, 255 });

	if (stringImage == NULL) {
		std::cout << "String surface not created." << std::endl;
	}

	if (texture == 0) {
		glGenTextures(1, &texture);//This avoids memory leakage, only initialise //first time 
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, stringImage->w, stringImage->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, stringImage->pixels);
	glBindTexture(GL_TEXTURE_2D, NULL);

	SDL_FreeSurface(stringImage);
	return texture;
}

//clearing the labels text to texture
void clearTextTexture(GLuint textID) {
	if (textID != NULL) {
		glDeleteTextures(1, &textID);
	}
}

//used for moving forward and backwards
glm::vec3 moveForward(glm::vec3 cam, GLfloat angle, GLfloat d) 
{
	return glm::vec3(cam.x + d*std::sin(glm::radians(angle)),
		cam.y, cam.z - d*std::cos(glm::radians(angle)));
}

//used for moving left and right
glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d)
{
	return glm::vec3(pos.x + d*std::cos(glm::radians(angle)),
		pos.y, pos.z + d*std::sin(glm::radians(angle)));
}

//playing the jumping sound effect
void playJumpSound()
{
	if (allowJumpSound) {
		HCHANNEL ch = BASS_SampleGetChannel(samples[0], FALSE);
		BASS_ChannelSetAttribute(ch, BASS_ATTRIB_FREQ, 0);
		BASS_ChannelSetAttribute(ch, BASS_ATTRIB_VOL, 0.5);
		BASS_ChannelSetAttribute(ch, BASS_ATTRIB_PAN, -1);
		if (!BASS_ChannelPlay(ch, FALSE))
			cout << "Can't play sample" << endl;
	}
}

//playing the walking sound effect
void playWalkSound()
{
	if (allowWalkSound) {
		HCHANNEL ch = BASS_SampleGetChannel(samples[1], FALSE);
		BASS_ChannelSetAttribute(ch, BASS_ATTRIB_FREQ, 0);
		BASS_ChannelSetAttribute(ch, BASS_ATTRIB_VOL, 0.5);
		BASS_ChannelSetAttribute(ch, BASS_ATTRIB_PAN, -1);
		if (!BASS_ChannelPlay(ch, FALSE))
			cout << "Can't play sample" << endl;
	}
}

void update(SDL_Event _event) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	
	//moving around
	if (keys[SDL_SCANCODE_W]) 
	{	
		//eye position
		eye = moveForward(eye, rotateValueX, 0.2f);
		
		//moving the player model
		dz += 1.0f;

		//running animation
		currentAnim = 1;
		//cout << "Walking animation" << endl;

		//playing walking sound
		playWalkSound();
		allowWalkSound = false;
	}
	else{ 
		//resetting the animation to idle
		currentAnim = 0; 
		allowWalkSound = true; 
	}

	if (keys[SDL_SCANCODE_S]) 
	{
		//eye position
		eye = moveForward(eye, rotateValueX, -0.2f);

		//moving the player model
		dz -= 1.0f;

		//running animation
		currentAnim = 1;
		//cout << "Walking animation" << endl;
	}
	if (keys[SDL_SCANCODE_A]) 
	{
		//eye position
		eye = moveRight(eye, rotateValueX, -0.2f);

		//moving the player model
		dx += 1.0f;

		//running animation
		currentAnim = 1;
		//cout << "Walking animation" << endl;
	}
	if (keys[SDL_SCANCODE_D])
	{
		//eye position
		eye = moveRight(eye, rotateValueX, 0.2f);

		//moving the player model
		dx -= 1.0f;

		//running animation
		currentAnim = 1;
		//cout << "Walking animation" << endl;
	}
	
	// character jump
	if (keys[SDL_SCANCODE_SPACE]) {
		//jumping animation
		currentAnim = 6;

		//playing jumping sound
		playJumpSound();
		allowJumpSound = false;
	}
	else{ allowJumpSound = true; }

	//updating the rotating code
	rotCube->update();

	//testing if player model has intersected cubes z position
	//COLLISION TESTING
	if (dz == rotCube->getPosition().z && dx == rotCube->getPosition().x)
	{
		if (rotCube->returnDrawValue())
		{
			//debugging
			cout << "Cube found" << endl;

			//stop drawing object
			rotCube->foundObject();
		}
	}
}


void draw(SDL_Window * window) {
	// clearing the screen
	glClearColor(0.5f,0.5f,0.5f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);

	//creating a projection matrix
	glm::mat4 projection(1.0f);

	//creating a modelview matrix
	glm::mat4 modelview(1.0);

	//pushing the modelview onto the stack
	mvStack.push(modelview);
	
	// just to allow easy scaling of complete scene
	GLfloat scale(5.0f); 

	//creating the projection matrix
	projection = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 1.0f, 200.0f);
	rt3d::setUniformMatrix4fv(ShaderProgram, "projection", glm::value_ptr(projection));

	//rotating the camera based on mouse position
	mvStack.top() = glm::rotate(mvStack.top(), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	mvStack.top() = glm::rotate(mvStack.top(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));


	//keeps camera locked to looking at the model
	eye.x = dx;
	eye.z = dz - 30;

	//moving camera based on keypresses and mouse position
	at = moveForward(eye, rotateValueX, -1.0f);
	mvStack.top() = glm::lookAt(eye, at, up);

	//draw the skybox
	glDepthMask(GL_FALSE);
	glCullFace(GL_FRONT);
	mvStack.push(mvStack.top());
	glBindTexture(GL_TEXTURE_2D, textures[3]); //apply the textures
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(4.0f, 4.0f, 4.0f));
	glm::mat4 view = mvStack.top();
	mvStack.top() = glm::mat4(glm::mat3(view)); // remove translation from the view matrix
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(ShaderProgram, materialSkyBox);  // sky material
	rt3d::setLight(ShaderProgram, lightSkyBox); // sky light
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
	glCullFace(GL_BACK);
	glDepthMask(GL_TRUE);

	//set the materials and lighting to their default value
	rt3d::setMaterial(ShaderProgram, material0);
	rt3d::setLight(ShaderProgram, light0);

	//draw a cube for ground plane
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, -5.0f, 0.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(100.0f, 0.1f, 100.0f));
	mvStack.top() = glm::rotate(mvStack.top(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(ShaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], cubeIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 1st wall
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), wall1Position);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(100.0f, 10.0f, 5.0f));
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(ShaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], cubeIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 2nd wall
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), wall2Position);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(5.0f, 10.0f, 100.0f));
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(ShaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], cubeIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 3rd wall
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), wall3Position);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(5.0f, 10.0f, 100.0f));
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(ShaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], cubeIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 4th wall
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), wall4Position);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(100.0f, 10.0f, 5.0f));
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(ShaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], cubeIndexCount, GL_TRIANGLES);
	mvStack.pop();

	glBindTexture(GL_TEXTURE_2D, textures[2]); //changing textures
	rotCube->draw(&mvStack);	//drawing the rotating cube

	// Animate the md2 model, and update the mesh with new vertex data
	tmpModel.Animate(currentAnim, 0.1);
	rt3d::updateMesh(meshObjects[1], RT3D_VERTEX, tmpModel.getAnimVerts(), tmpModel.getVertDataSize());

	// draw the hobgoblin
	glCullFace(GL_FRONT);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	rt3d::materialStruct tmpMaterial = material0;
	rt3d::setMaterial(ShaderProgram, tmpMaterial);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(dx, dy, dz));
	mvStack.top() = glm::rotate(mvStack.top(), glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	mvStack.top() = glm::rotate(mvStack.top(), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	mvStack.top() = glm::rotate(mvStack.top(), glm::radians(rotateValueX), glm::vec3(0.0f, 0.0f, -1.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(scale*0.05, scale*0.05, scale*0.05));
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawMesh(meshObjects[1], md2VertCount, GL_TRIANGLES);
	mvStack.pop();

	//DRAWING A HUD
	glUseProgram(ShaderProgram);
	glDisable(GL_DEPTH_TEST);//Disable depth test for HUD label

	if (rotCube->returnDrawValue())
	{
		labels[0] = textToTexture("Find the object", labels[0]);
	}
	else
		labels[0] = textToTexture("object found!", labels[0]);
	glBindTexture(GL_TEXTURE_2D, labels[0]);
	mvStack.push(glm::mat4(1.0));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-0.8f, 0.8f, 0.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(0.20f, 0.2f, 0.0f));
	mvStack.top() = glm::rotate(mvStack.top(), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	rt3d::setUniformMatrix4fv(ShaderProgram, "projection", glm::value_ptr(glm::mat4(1.0)));
	rt3d::setUniformMatrix4fv(ShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
	glEnable(GL_DEPTH_TEST);//Re-enable depth test after HUD label 
	glCullFace(GL_BACK);


	//final pop
	mvStack.pop(); // initial matrix
	glDepthMask(GL_TRUE); // make sure depth test is on
    SDL_GL_SwapWindow(window); // swap buffers
}

// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {
	SDL_Renderer *renderTarget = nullptr;
    SDL_Window *window; // window handle
    SDL_GLContext glContext; // OpenGL context handle
	window = setupRC(glContext); // Create window and render context 
	renderTarget = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Required on Windows *only* init GLEW to access OpenGL beyond 1.1
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) { // glewInit failed, something is seriously wrong
		std::cout << "glewInit failed, aborting." << endl;
		exit (1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();

	bool running = true; // set running to true
	SDL_Event sdlEvent;  // variable to detect SDL events

	bool b_button_released = false;

	while (running) {	// the event loop
		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_QUIT)
				running = false;

			if (sdlEvent.type == SDL_MOUSEBUTTONUP) 
			{
				b_button_released = true;
			}
				
			//checking if the left button has been pressed
			if (sdlEvent.button.button == SDL_BUTTON_LEFT)
			{
				//local variables for mouse pos
				int x = 0.0f, y = 0.0f;

				//checks current mouse state
				if (SDL_GetMouseState(&x, &y))
				{
					float f_curr_x = x;
					
					float f_dx = f_curr_x - x_prev;

					if (b_button_released)
					{
						b_button_released = false;
						f_dx = 0.0f;
					}
					x_prev = f_curr_x;
					rotateValueX += glm::radians(f_dx) *10;
				}
			}
		}
		update(sdlEvent);	// call the update function
		draw(window); // call the draw function
	}
	//free up space used by bass library
	BASS_Free();

	//delete & close window
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}