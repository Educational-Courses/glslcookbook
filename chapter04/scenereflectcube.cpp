#include "scenereflectcube.h"

#include <cstdio>
#include <cstdlib>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "tgaio.h"
#include "glutils.h"
#include "defines.h"

using glm::vec3;

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

SceneReflectCube::SceneReflectCube() : angle(0.0f), tPrev(0.0f), rotSpeed(PI/8.0) { }

void SceneReflectCube::initScene()
{
    compileAndLinkShader();

    glEnable(GL_DEPTH_TEST);

    teapot = new VBOTeapot(14, mat4(1.0f));
    sky = new SkyBox();
    plane = new VBOPlane(1.0f,1.0f,1,1);
    float c = 3.5f;
    torus = new VBOTorus(0.7f * c, 0.3f * c, 50, 50);

    projection = mat4(1.0f);

    angle = glm::radians(90.0);

    loadCubeMap("../media/texture/cubemap_night/night");
}

void SceneReflectCube::loadCubeMap( const char * baseFileName )
{
    glActiveTexture(GL_TEXTURE0);

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    const char * suffixes[] = { "posx", "negx", "posy", "negy", "posz", "negz" };
    GLuint targets[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    GLint w,h;
    // Allocate the cube map texture
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 256, 256);

    // Load each cube-map face
    for( int i = 0; i < 6; i++ ) {
	string texName = string(baseFileName) + "_" + suffixes[i] + ".tga";
	GLubyte * data = TGAIO::read(texName.c_str(), w, h);
	glTexSubImage2D(targets[i], 0, 0, 0, w, h,
		    GL_RGBA, GL_UNSIGNED_BYTE, data);
	delete [] data;
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void SceneReflectCube::update( float t )
{
	float deltaT = t - tPrev;
	if(tPrev == 0.0f) deltaT = 0.0f;
	tPrev = t;

    angle += rotSpeed * deltaT;
    if( angle > TWOPI_F) angle -= TWOPI_F;
}

void SceneReflectCube::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 cameraPos = vec3( 7.0f * cos(angle), 2.0f, 7.0f * sin(angle));
    view = glm::lookAt(cameraPos, vec3(0.0f,0.0f,0.0f), vec3(0.0f,1.0f,0.0f));
    prog.setUniform("WorldCameraPosition", cameraPos);

    prog.setUniform("DrawSkyBox", true);
    model = mat4(1.0f);
    setMatrices();
    sky->render();
    prog.setUniform("DrawSkyBox", false);

    prog.setUniform("MaterialColor", vec4(0.5f, 0.5f, 0.5f, 1.0f));
    prog.setUniform("ReflectFactor", 0.85f);

    model = mat4(1.0f);
    model *= glm::translate(vec3(0.0f,-1.0f,0.0f));
    model *= glm::rotate(glm::radians(-90.0f), vec3(1.0f,0.0f,0.0f));
    setMatrices();
    teapot->render();
}

void SceneReflectCube::setMatrices()
{
    mat4 mv = view * model;
    prog.setUniform("ModelMatrix", model);
    prog.setUniform("MVP", projection * mv);
}

void SceneReflectCube::resize(int w, int h)
{
    glViewport(0,0,w,h);
    width = w;
    height = h;
    projection = glm::perspective(glm::radians(50.0f), (float)w/h, 0.3f, 100.0f);
}

void SceneReflectCube::compileAndLinkShader()
{
	try {
		prog.compileShader("shader/cubemap_reflect.vs",GLSLShader::VERTEX);
		prog.compileShader("shader/cubemap_reflect.fs",GLSLShader::FRAGMENT);
    	prog.link();
    	prog.use();
    } catch(GLSLProgramException & e) {
 		cerr << e.what() << endl;
 		exit( EXIT_FAILURE );
    }
}
