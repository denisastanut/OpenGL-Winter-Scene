#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "Skybox.hpp"

#include <iostream>


const float GLOBAL_SCALE = 6.0f;

// window
gps::Window myWindow;

// matrices
glm::mat4 model; // local space to world space
glm::mat4 view; // world space to camera space
glm::mat4 projection; // camera space to clip space
glm::mat3 normalMatrix;


// LIGHTS
// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
float lightRotationAngle = 0.0f;

// point light positions
glm::vec3 pointLightPos1; // first lamp
glm::vec3 pointLightPos2; // second lamp
glm::vec3 pointLightPos3; // house lamp
glm::vec3 pointLightPos4; // road lamp
glm::vec3 pointLightColor; // color for all of them

GLint pointLightPosLoc;
GLint pointLightColorLoc;

// fire
glm::vec3 fireLightPos;
glm::vec3 fireLightColor;

glm::vec3 secondFireLightPos;
glm::vec3 secondFireLightColor;

// light visibility
bool showPointLights = true; // M
bool showGlobalLight = true; // N


// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;


// SHADOWS
GLuint shadowMapFBO; // framebuffer
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 4096;  // width of the shadow map
const unsigned int SHADOW_HEIGHT = 4096; // height of the shadow map


// SHADERS
gps::Shader myBasicShader; // lighting, shadows, and textures
gps::Shader skyboxShader; 
gps::Shader lightShader; // light source
gps::Shader depthMapShader; // renders the scene from the light's view


// CAMERA
gps::Camera myCamera(
    glm::vec3(0.0f, 5.0f, 25.0f)* GLOBAL_SCALE, 
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

GLfloat cameraSpeed = 0.25f;


// INPUT
GLboolean pressedKeys[1024];
float lastX = 0.0f, lastY = 0.0f;
bool firstMouse = true;


// ANIMATION
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float tailAngle = 0.0f;


// SKYBOX
gps::SkyBox mySkyBox;


// 3D MODELS
gps::Model3D lightCube;
gps::Model3D myScene;
gps::Model3D huskyBody;
gps::Model3D huskyTail;

GLenum currentRenderMode = GL_FILL;

// husky control
float huskyX = -1.9f; // X coordinate
float huskyZ = 1.2f;   // Z coordinate
bool animHusky = true; // I


// FOG
GLint fogInitLoc;
int fogInit = 0; // 0 - disabled, 1 - enabled (F)

// SNOW
struct SnowFlake {
    glm::vec3 position;
    float speed;
};

const int SNOW_COUNT = 2000;
std::vector<SnowFlake> snowFlakes;
gps::Model3D snowFlakeModel; // used the light cube for the snowflakes
bool isSnowing = true; // G


//debug lights
bool lightDebugMode = false; // V
int selectedLightIndex = 1;  // 1->5



GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)


void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    // wireframe/point rendering mode
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
        currentRenderMode = (currentRenderMode == GL_FILL) ? GL_LINE : GL_FILL;
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
        currentRenderMode = (currentRenderMode == GL_FILL) ? GL_POINT : GL_FILL;

    // husky animation (I)
    if (key == GLFW_KEY_I && action == GLFW_PRESS)
        animHusky = !animHusky;

    // auto presentation (O)
    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        if (myCamera.autoPresentation) {
            myCamera.resetToInitial();
        }
        else {
            myCamera.autoPresentation = true;
        }
    }

    // global light (N)
    // lights (M)
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        showPointLights = !showPointLights;
        myBasicShader.useShaderProgram();
        glm::vec3 color = showPointLights ? pointLightColor : glm::vec3(0.0f);
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor"), 1, glm::value_ptr(color));
    }
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        showGlobalLight = !showGlobalLight;
        myBasicShader.useShaderProgram();
        glm::vec3 color = showGlobalLight ? lightColor : glm::vec3(0.0f);
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(color));
    }

    // fog (F)
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        fogInit = !fogInit;
        myBasicShader.useShaderProgram();
        glUniform1i(fogInitLoc, fogInit);
    }

    // snow (G)
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
        isSnowing = !isSnowing;


    // DEBUG LIGHTS
    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        lightDebugMode = !lightDebugMode;
        if (lightDebugMode) {
            std::cout << "MOD EDITARE LUMINI: ACTIVAT. Select 1-5" << std::endl;
        }
        else {
            std::cout << "MOD EDITARE LUMINI: DEZACTIVAT." << std::endl;
        }
    }

    // SPACE
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && lightDebugMode) {
        glm::vec3 pos;
        if (selectedLightIndex == 1) {
            pos = pointLightPos1;
        }
        if (selectedLightIndex == 2) {
            pos = pointLightPos2;
        }
        if (selectedLightIndex == 3) {
            pos = pointLightPos3;
        }
        if (selectedLightIndex == 4) {
            pos = pointLightPos4;
        }
        if (selectedLightIndex == 5) {
            pos = fireLightPos;
        }
        if (selectedLightIndex == 6) {
            pos = secondFireLightPos;
        }

        std::cout << "Lumina " << selectedLightIndex << " Pozitie: "
            << "glm::vec3(" << pos.x << "f, " << pos.y << "f, " << pos.z << "f);" << std::endl;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) { 
        selectedLightIndex = 1; 
        std::cout << "Selectat: Lumina 1" << std::endl; 
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) { 
        selectedLightIndex = 2; 
        std::cout << "Selectat: Lumina 2" << std::endl; 
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) { 
        selectedLightIndex = 3; 
        std::cout << "Selectat: Lumina 3" << std::endl;
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        selectedLightIndex = 4;
        std::cout << "Selectat: Lumina 4" << std::endl; 
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) { 
        selectedLightIndex = 5; 
        std::cout << "Selectat: FOC" << std::endl;
    }
    if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
        selectedLightIndex = 6; 
        std::cout << "Selectat: FOC SECUNDAR" << std::endl; 
    }
}


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    if (!myCamera.autoPresentation) {
        myCamera.rotate(yoffset * 0.1f, xoffset * 0.1f);

        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
}


void processMovement() {
    if (myCamera.autoPresentation) {
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        return;
    }

    float effectiveSpeed = cameraSpeed;

    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, effectiveSpeed);
        // update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, effectiveSpeed);
        // update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, effectiveSpeed);
        // update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, effectiveSpeed);
        // update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_Z]) {
        myCamera.move(gps::MOVE_UP, effectiveSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_X]) {
        myCamera.move(gps::MOVE_DOWN, effectiveSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    // camera rotation
    if (pressedKeys[GLFW_KEY_Q]) {
        myCamera.rotate(0.0f, -1.0f); 
        // update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        myCamera.rotate(0.0f, 1.0f);
        // update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }


    // light rotation
    if (pressedKeys[GLFW_KEY_J]) {
        lightRotationAngle -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_L]) {
        lightRotationAngle += 1.0f;
    }

    // DEBUG LIGHTS
    if (lightDebugMode) {
        float lightSpeed = 2.0f * deltaTime;
        glm::vec3* currentLight = nullptr;

        if (selectedLightIndex == 1) currentLight = &pointLightPos1;
        else if (selectedLightIndex == 2) {
            currentLight = &pointLightPos2;
        }
        else if (selectedLightIndex == 3) {
            currentLight = &pointLightPos3;
        }
        else if (selectedLightIndex == 4) {
            currentLight = &pointLightPos4;
        }
        else if (selectedLightIndex == 5) {
            currentLight = &fireLightPos;
        }
        else if (selectedLightIndex == 6) {
            currentLight = &secondFireLightPos;
        }


        if (currentLight) {
            if (pressedKeys[GLFW_KEY_UP]) {
                currentLight->z -= lightSpeed;
            }
            if (pressedKeys[GLFW_KEY_DOWN]) {
                currentLight->z += lightSpeed;
            }
            if (pressedKeys[GLFW_KEY_LEFT]) {
                currentLight->x -= lightSpeed;
            }
            if (pressedKeys[GLFW_KEY_RIGHT]) {
                currentLight->x += lightSpeed;
            }
            if (pressedKeys[GLFW_KEY_PAGE_UP]) {
                currentLight->y += lightSpeed;
            }
            if (pressedKeys[GLFW_KEY_PAGE_DOWN]) {
                currentLight->y -= lightSpeed;
            }
        }
    }
    else {
        if (pressedKeys[GLFW_KEY_UP]) {
            huskyZ -= 0.05f;
        }
        if (pressedKeys[GLFW_KEY_DOWN]) {
            huskyZ += 0.05f;
        }
        if (pressedKeys[GLFW_KEY_LEFT]) {
            huskyX -= 0.05f;
        }
        if (pressedKeys[GLFW_KEY_RIGHT]) {
            huskyX += 0.05f;
        }
    }
}


// init FBO for shadow mapping
void initFBO() {
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);

    glBindTexture(GL_TEXTURE_2D, depthMapTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


// calculate view and projection matrices
glm::mat4 computeLightSpaceTrMatrix(glm::vec3 currentLightDir) {
    glm::mat4 lightView = glm::lookAt(glm::normalize(currentLightDir) * 20.0f * GLOBAL_SCALE, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    float orthoSize = 20.0f * GLOBAL_SCALE;
    const GLfloat near_plane = 0.1f, far_plane = 50.0f * GLOBAL_SCALE;

    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);

    return lightProjection * lightView;
}


void initOpenGLWindow() {
    myWindow.Create(1024, 768, "Winter Scene");

    glfwMaximizeWindow(myWindow.getWindow());
}

void setWindowCallbacks() {
    glfwSetFramebufferSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}


void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

    int width, height;
    glfwGetFramebufferSize(myWindow.getWindow(), &width, &height);

    glViewport(0, 0, width, height); 

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
    glEnable(GL_BLEND); // for transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // standard blending math
}


// load 3D models
void initModels() {
    std::cout << "Loading scene..." << std::endl;

    myScene.LoadModel("models/fullScene/scene.obj", "models/fullScene/");

    huskyBody.LoadModel("models/husky_body/body.obj", "models/husky_body/");
    huskyTail.LoadModel("models/husky_tail/tail.obj", "models/husky_tail/");

    lightCube.LoadModel("models/cube/cube.obj", "models/cube/");

    std::vector<const GLchar*> faces;
    faces.push_back("models/skybox/right.png");
    faces.push_back("models/skybox/left.png");
    faces.push_back("models/skybox/top.png");
    faces.push_back("models/skybox/bottom.png");
    faces.push_back("models/skybox/back.png");
    faces.push_back("models/skybox/front.png");

    mySkyBox.Load(faces); // load and create the cubemap
}

// initializes the snow cubes
void initSnow() {
    for (int i = 0; i < SNOW_COUNT; i++) {
        SnowFlake flake;

        // random positions
        float x = ((rand() % 4000) / 100.0f) - 20.0f;
        float y = ((rand() % 1500) / 100.0f);
        float z = ((rand() % 4000) / 100.0f) - 20.0f;

        flake.position = glm::vec3(x, y, z);

        // random falling speed
        flake.speed = 1.0f + ((rand() % 200) / 100.0f);

        snowFlakes.push_back(flake);
    }
}


// loads shaders
void initShaders() {
    myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");

    skyboxShader.loadShader(
        "shaders/skyboxShader.vert", 
        "shaders/skyboxShader.frag");

    lightShader.loadShader(
        "shaders/lightCube.vert", 
        "shaders/lightCube.frag");

    depthMapShader.loadShader(
        "shaders/depthMap.vert", 
        "shaders/depthMap.frag");
}


void initUniforms() {
    myBasicShader.useShaderProgram();

    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");

    // create model matrix
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    // send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // create projection matrix
    int width, height;
    glfwGetFramebufferSize(myWindow.getWindow(), &width, &height);
    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set the light direction (direction towards the light)
    lightDir = glm::vec3(0.5f, 1.0f, 0.5f);
    // send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::normalize(lightDir)));

    //set light color
    lightColor = glm::vec3(0.1f, 0.1f, 0.3f);
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    pointLightPos1 = glm::vec3(-1.40331f, 0.9f, 5.91541f) * GLOBAL_SCALE;
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos1"), 1, glm::value_ptr(pointLightPos1));

    pointLightPos2 = glm::vec3(0.500488f, 0.9f, 5.87305f) * GLOBAL_SCALE;
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos2"), 1, glm::value_ptr(pointLightPos2));

    pointLightPos3 = glm::vec3(-10.2773f, 0.8f, -0.597198f) * GLOBAL_SCALE;
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos3"), 1, glm::value_ptr(pointLightPos3));

    pointLightPos4 = glm::vec3(-5.91641f, 0.8f, 2.39001f) * GLOBAL_SCALE;
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos4"), 1, glm::value_ptr(pointLightPos4));

    pointLightColor = glm::vec3(5.0f, 4.0f, 3.0f);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor"), 1, glm::value_ptr(pointLightColor));

    fireLightPos = glm::vec3(7.63786f, -0.612076f, 4.61048f) * GLOBAL_SCALE;
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fireLightPos"), 1, glm::value_ptr(fireLightPos));

    fireLightColor = glm::vec3(6.0f, 3.0f, 0.0f);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fireLightColor"), 1, glm::value_ptr(fireLightColor));

    secondFireLightPos = glm::vec3(6.21447f, 0.267128f, -10.7604f);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "secondFireLightPos"), 1, glm::value_ptr(secondFireLightPos));

    secondFireLightColor = glm::vec3(1.5f, 0.4f, 0.02f);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "secondFireLightColor"), 1, glm::value_ptr(secondFireLightColor));


    // link the fog
    fogInitLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogInit");
    glUniform1i(fogInitLoc, fogInit);
}


void renderFullScene(gps::Shader shader) {
    shader.useShaderProgram();
    glPolygonMode(GL_FRONT_AND_BACK, currentRenderMode);

    model = glm::scale(glm::mat4(1.0f), glm::vec3(GLOBAL_SCALE));

    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.05f)); // Scalarea originala a scenei se aplica PESTE cea globala

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    GLint normLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
    if (normLoc != -1) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    myScene.Draw(shader);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void renderHusky(gps::Shader shader) {
    shader.useShaderProgram();
    glPolygonMode(GL_FRONT_AND_BACK, currentRenderMode);

    glm::vec3 dogPosition = glm::vec3(huskyX, -0.3f, huskyZ);
    float dogScale = 0.35f;

    glm::mat4 modelBody = glm::scale(glm::mat4(1.0f), glm::vec3(GLOBAL_SCALE));
    modelBody = glm::translate(modelBody, dogPosition);
    modelBody = glm::scale(modelBody, glm::vec3(dogScale));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelBody));

    // normal matrix body
    GLint normLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
    if (normLoc != -1) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * modelBody));
        glUniformMatrix3fv(normLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    huskyBody.Draw(shader);

    // draw tail
    glm::mat4 modelTail = modelBody;
    modelTail = glm::translate(modelTail, glm::vec3(0.0f, 0.70f, -0.35f));

    float currentTailAngle = animHusky ? tailAngle : 0.0f;
    modelTail = glm::rotate(modelTail, glm::radians(currentTailAngle), glm::vec3(0.0f, 0.0f, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelTail));

    if (normLoc != -1) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * modelTail));
        glUniformMatrix3fv(normLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    huskyTail.Draw(shader);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void renderLights(gps::Shader shader) {
    shader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    auto drawCubeAt = [&](glm::vec3 pos) {
        model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, glm::vec3(0.05f * GLOBAL_SCALE)); // Scalam doar cubuletul vizual sa fie mic
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        lightCube.Draw(shader);
        };

    /*drawCubeAt(pointLightPos1);
    drawCubeAt(pointLightPos2);
    drawCubeAt(pointLightPos3);
    drawCubeAt(pointLightPos4);
    drawCubeAt(fireLightPos);
    drawCubeAt(secondFireLightPos);*/
}


void renderSnow(gps::Shader shader, float deltaTime) {
    shader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    for (int i = 0; i < SNOW_COUNT; i++) {
        snowFlakes[i].position.y -= snowFlakes[i].speed * deltaTime;

        if (snowFlakes[i].position.y < -1.0f) {
            snowFlakes[i].position.y = 15.0f;
        }

        // draw
        model = glm::scale(glm::mat4(1.0f), glm::vec3(GLOBAL_SCALE));
        model = glm::translate(model, snowFlakes[i].position);
        model = glm::scale(model, glm::vec3(0.02f)); 

        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        lightCube.Draw(shader); // use the cube model for light
    }
}

void renderScene() {
    glm::mat4 lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 currentLightDir = glm::vec3(lightRotation * glm::vec4(lightDir, 0.0f));
    currentLightDir = glm::normalize(currentLightDir);

    glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix(currentLightDir);

    depthMapShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    renderFullScene(depthMapShader);
    renderHusky(depthMapShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    int width, height;
    glfwGetFramebufferSize(myWindow.getWindow(), &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myBasicShader.useShaderProgram();

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glUniform3fv(lightDirLoc, 1, glm::value_ptr(currentLightDir));

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos1"), 1, glm::value_ptr(pointLightPos1));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos2"), 1, glm::value_ptr(pointLightPos2));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos3"), 1, glm::value_ptr(pointLightPos3));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos4"), 1, glm::value_ptr(pointLightPos4));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fireLightPos"), 1, glm::value_ptr(fireLightPos));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "secondFireLightPos"), 1, glm::value_ptr(secondFireLightPos));

    renderFullScene(myBasicShader);
    renderHusky(myBasicShader);
    renderLights(lightShader);

    if (isSnowing) {
        renderSnow(lightShader, deltaTime);
    }

    mySkyBox.Draw(skyboxShader, view, projection);
}


void cleanup() {
    myWindow.Delete();
}


int main(int argc, const char* argv[]) {
    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();
    setWindowCallbacks();
    initSnow();
    initFBO();

    glCheckError();

    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        tailAngle = sin(currentFrame * 5.0f) * 20.0f;

        if (myCamera.autoPresentation) {
            myCamera.updateAutoPresentation(deltaTime, GLOBAL_SCALE);

            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        }
        else {
            processMovement();
        }

        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}