#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Windows.h>
#include <mmsystem.h>
#include <iostream>
#include <vector>
#include <stack>
#include "rt3d.h"

using namespace std;
using namespace glm;

class rotatingCube
{
private:
	GLuint rotCubeIndexCount = 36;
	GLuint rotCubeVertCount = 8;
	//std::stack<glm::mat4> mvStack;
	GLuint meshObjects[1];
	rt3d::materialStruct material0;
	GLfloat rotator = 0.0f;
	GLuint myShaderProgram;
	glm::mat4 myprojection;
	glm::vec3 position;
	bool allowDraw = true;
public:
	rotatingCube() {};
	void Set_ShaderID(GLuint _id);
	void init(void);
	void update(void);
	void draw(std::stack<glm::mat4> *_mvStack);
	glm::vec3 getPosition();
	void foundObject();
	bool returnDrawValue();
};