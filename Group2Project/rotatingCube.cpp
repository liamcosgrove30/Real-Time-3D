#include "rotatingCube.h"
GLuint rotCubeIndices[] = { 0, 1, 2,
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
7, 6, 4 };

GLfloat rotCubeVertices[] = { -0.5f, -0.5f, -0.5f,
-0.5f,  0.5f, -0.5f,
0.5f,  0.5f, -0.5f,
0.5f, -0.5f, -0.5f,
-0.5f, -0.5f, 0.5f,
-0.5f,  0.5f, 0.5f,
0.5f,  0.5f, 0.5f,
0.5f, -0.5f, 0.5f
};

void rotatingCube::init(void)
{
	//Shader programs
	//ShaderProgram = rt3d::initShaders("phong-tex.vert", "phong-tex.frag");

	//texture data
	GLfloat rotCubeTexCoords[] = { 0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f
	};

	material0 = {

		{ 0.4f, 0.2f, 0.2f, 1.0f }, // ambient

		{ 0.8f, 0.5f, 0.5f, 1.0f }, // diffuse

		{ 1.0f, 0.8f, 0.8f, 1.0f }, // specular

		2.0f // shininess
	};

	//first mesh
	meshObjects[0] = rt3d::createMesh(rotCubeVertCount, rotCubeVertices, nullptr, rotCubeVertices, rotCubeTexCoords, rotCubeIndexCount, rotCubeIndices);

	position = vec3(0.0f, 0.0f, 10.0f);
}


void rotatingCube::update(void)
{
	//making the cube rotate
	rotator += 1.0f;
}

void rotatingCube::Set_ShaderID(GLuint _id)
{
	myShaderProgram = _id;
}

void rotatingCube::foundObject()
{
	allowDraw = false;
}

void rotatingCube::draw(std::stack<glm::mat4> *_mvStack)
{
	if (allowDraw)
	{
		//rotating box
		_mvStack->push(_mvStack->top());
		_mvStack->top() = glm::translate(_mvStack->top(), position);
		_mvStack->top() = glm::scale(_mvStack->top(), glm::vec3(3.0f, 3.0f, 3.0f));
		_mvStack->top() = glm::rotate(_mvStack->top(), glm::radians(rotator), glm::vec3(0.0f, 1.0f, 0.0f));
		rt3d::setUniformMatrix4fv(myShaderProgram, "modelview", glm::value_ptr(_mvStack->top()));
		rt3d::setMaterial(myShaderProgram, material0);
		rt3d::drawIndexedMesh(meshObjects[0], rotCubeIndexCount, GL_TRIANGLES);
		_mvStack->pop();
	}
}

vec3 rotatingCube::getPosition()
{
	return position;
}

bool rotatingCube::returnDrawValue()
{
	return allowDraw;
}

