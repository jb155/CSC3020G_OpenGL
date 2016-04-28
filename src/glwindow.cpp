#include <iostream>
#include <stdio.h>

#include "SDL.h"
#include <GL/glew.h>
#include <GL/glut.h>

#include "glwindow.h"
#include "geometry.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>

using namespace std;
GeometryData geometry;

enum WindowState
{
    VIEW,
    ROTATE,
    SCALE,
    TRANSLATE
};
WindowState currentWindowType;
int colorLoc;
float objScale = 1.0f;
float objectPos[] {
  0.0f,
  0.0f,
  0.0f
};

const char* glGetErrorString(GLenum error)
{
    switch(error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNRECOGNIZED";
    }
}

void glPrintError(const char* label="Unlabelled Error Checkpoint", bool alwaysPrint=false)
{
    GLenum error = glGetError();
    if(alwaysPrint || (error != GL_NO_ERROR))
    {
        printf("%s: OpenGL error flag is %s\n", label, glGetErrorString(error));
    }
}

GLuint loadShader(const char* shaderFilename, GLenum shaderType)
{
    FILE* shaderFile = fopen(shaderFilename, "r");
    if(!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    long shaderSize = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    char* shaderText = new char[shaderSize+1];
    size_t readCount = fread(shaderText, 1, shaderSize, shaderFile);
    shaderText[readCount] = '\0';
    fclose(shaderFile);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char**)&shaderText, NULL);
    glCompileShader(shader);

    delete[] shaderText;

    return shader;
}

GLuint loadShaderProgram(const char* vertShaderFilename,
                       const char* fragShaderFilename)
{
    GLuint vertShader = loadShader(vertShaderFilename, GL_VERTEX_SHADER);
    GLuint fragShader = loadShader(fragShaderFilename, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &logLength, message);
        cout << "Shader load error: " << message << endl;
        return 0;
    }

    return program;
}

OpenGLWindow::OpenGLWindow()
{
}


void OpenGLWindow::initGL()
{
    // We need to first specify what type of OpenGL context we need before we can create the window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);           //Anti-aliasing
    /* Enable Z depth testing so objects closest to the viewpoint are in front of objects further away */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    sdlWin = SDL_CreateWindow("OpenGL Prac 1",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_OPENGL);
    if(!sdlWin)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to create window", 0);
    }
    SDL_GLContext glc = SDL_GL_CreateContext(sdlWin);
    SDL_GL_MakeCurrent(sdlWin, glc);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = true;
    GLenum glewInitResult = glewInit();
    glGetError(); // Consume the error erroneously set by glewInit()
    if(glewInitResult != GLEW_OK)
    {
        const GLubyte* errorString = glewGetErrorString(glewInitResult);
        cout << "Unable to initialize glew: " << errorString;
    }

    int glMajorVersion;
    int glMinorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
    cout << "Loaded OpenGL " << glMajorVersion << "." << glMinorVersion << " with:" << endl;
    cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
    cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
    cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
    cout << "\tGLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0,0,0,1);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Note that this path is relative to your working directory
    // when running the program (IE if you run from within build
    // then you need to place these files in build as well)
    shader = loadShaderProgram("simple.vert", "simple.frag");
    glUseProgram(shader);

    colorLoc = glGetUniformLocation(shader, "objectColor");
    glUniform3f(colorLoc, 100.0f, 100.0f, 100.0f);

    // Load the model that we want to use and buffer the vertex attributes
    geometry.loadFromOBJFile("sample-bunny.obj");

    vertexLoc = glGetAttribLocation(shader, "position");

    matrixLoc = glGetUniformLocation(shader, "mat4Loc");

    //model = glm::perspective()
    //model = glm::

    modelMat4 = glm::scale(identMat4, glm::vec3(2.0f,2.0f,2.0f));
    finalMat4 = modelMat4 * identMat4;

    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, &finalMat4[0][0]);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    //glBufferData(GL_ARRAY_BUFFER, 9*sizeof(float), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, geometry.vertexCount() * sizeof(geometry.textureCoordData()) * 3, geometry.vertexData(), GL_STATIC_DRAW);
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(vertexLoc);
    glEnableVertexAttribArray(matrixLoc);

    glPrintError("Setup complete", true);
}

void OpenGLWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE_ARB);

    //std::cout<<currentWindowType<<std::endl;
    switch (currentWindowType) {
      case 0:
      {
        //std::cout<<"View"<<std::endl;
        break;
      }
      case 1:
      {
        std::vector<float> v = geometry.getMouseLoc();  //get mouse loc

        int direct = 0;
        if(v[0]<0||v[1]<0){
          direct = -1;
        }else if (v[0]>0||v[1]>0){
          direct = 1;
        }


        float dist = (std::sqrt(v[0]*v[0] + v[1]*v[1])*direct +0.5f)/50.0f; //Calc dist of center of screen to mouse cusor

        if(v[0]!=0&&v[1]!=0){   //if mouse is center of screen...dont do anything
          switch (rotateAxis) {
            case 1:                   //x
            {
              modelMat4 = glm::rotate(modelMat4, dist, glm::vec3(1.0f,0.0f,0.0f)); //rotate on mouse x
              break;
            }
            case 2:                   //y
            {
              modelMat4 = glm::rotate(modelMat4, dist, glm::vec3(0.0f,1.0f,0.0f)); //rotate on mouse y
              break;
            }
            case 3:                   //z
            {
              modelMat4 = glm::rotate(modelMat4, dist, glm::vec3(0.0f,0.0f,1.0f)); //rotate on mouse z
              break;
            }
          }
        }
        break;
      }
      case 2://Scale
      {
        std::vector<float> v = geometry.getMouseLoc();  //get mouse loc


        float dist = ((std::sqrt(std::abs(v[0]*v[0]) + std::abs(v[1]*v[1])))/2 + 0.75f);  //Calc dist of center of screen to mouse cusor
        //std::cout<<dist<<std::endl;
        //std::cout<<objScale<<std::endl;
        if(((objScale*dist>0.25f)&&(dist<1))||((objScale*dist<2.0f)&&(dist>1))){  //clamps scale when shrinking
          modelMat4 = glm::scale(modelMat4, glm::vec3(dist,dist,dist));
          objScale = objScale*dist; //keep track of scaling
        }
        break;
      }
      case 3:
      {
        std::vector<float> v = geometry.getMouseLoc();//get mouse loc

        int direct = 0;
        if(v[0]<0||v[1]<0){
          direct = -1;
        }else if (v[0]>0||v[1]>0){
          direct = 1;
        }


        float dist = (std::sqrt(v[0]*v[0] + v[1]*v[1])*direct +0.5f)/75.0f; //Calc dist of center of screen to mouse cusor

        //std::cout<<objectPos[0]<<std::endl;
        //std::cout<<"translateAxis "<<translateAxis<<std::endl;

        if(v[0]!=0&&v[1]!=0){   //if mouse is center of screen...dont do anything
          switch (translateAxis) {
            case 1:                   //x
            {
              std::cout << "x" << std::endl;
              glm::vec3 direction = glm::vec3 (1.0f,0.0f,0.0f) * dist * -0.25f;
              modelMat4 = glm::translate(modelMat4, direction); //translate model
              objectPos[0] = objectPos[0] + (dist * -0.25f);  //keep track of translate
              break;
            }
            case 2:                   //y
            {
              std::cout << "y" << std::endl;
              glm::vec3 direction = glm::vec3 (0.0f,1.0f,0.0f) * dist * 0.25f;
              modelMat4 = glm::translate(modelMat4, direction); //translate model
              objectPos[1] += dist * 0.25f; //keep track of translate
              break;
            }
            case 3:                   //z
            {
              std::cout << "z" << std::endl;
              glm::vec3 direction = glm::vec3 (0.0f,0.0f,1.0f) * dist * 0.25f;
              modelMat4 = glm::translate(modelMat4, direction); //translate model
              objectPos[2] += dist * 0.25f; //keep track of translate
              break;
            }
          }
        }
        break;
      }
    }

    finalMat4 = modelMat4 * identMat4;
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, &finalMat4[0][0]);
    glEnableVertexAttribArray(vertexLoc);
    glEnableVertexAttribArray(matrixLoc);
    glDrawArrays(GL_TRIANGLES, 0, geometry.vertexCount());
    // Swap the front and back buffers on the window, effectively putting what we just "drew"
    // onto the screen (whereas previously it only existed in memory)
    SDL_GL_SwapWindow(sdlWin);
}

// The program will exit if this function returns false
bool OpenGLWindow::handleEvent(SDL_Event e)
{
    // A list of keycode constants is available here: https://wiki.libsdl.org/SDL_Keycode
    // Note that SDL provides both Scancodes (which correspond to physical positions on the keyboard)
    // and Keycodes (which correspond to symbols on the keyboard, and might differ across layouts)
    if(e.type == SDL_KEYDOWN)
    {
      //Escape
        if(e.key.keysym.sym == SDLK_ESCAPE)
        {
            return false;
        }
        //Rotate
        if(e.key.keysym.sym == SDLK_r){         //step through rotation axis none, x, y, z, wrap around
          translateAxis = 0;
          if(rotateAxis < 3){
            currentWindowType = ROTATE;
            rotateAxis++;
          }else{
            currentWindowType = VIEW;
            rotateAxis = 0;
          }
        }
        //Scale
        if(e.key.keysym.sym == SDLK_s){
          translateAxis = 0;
          rotateAxis = 0;
          if(currentWindowType == SCALE){
            currentWindowType = VIEW;
          }else{
            currentWindowType = SCALE;
          }
        }
        //Translate
        if(e.key.keysym.sym == SDLK_t){         //step through translate axis none, x, y, z, wrap around
          rotateAxis = 0;
          if(rotateAxis < 3){
            currentWindowType = TRANSLATE;
            translateAxis++;
          }else{
            currentWindowType = VIEW;
            translateAxis = 0;
          }
        }


        //colour
        if(e.key.keysym.sym == SDLK_1){
          colorLoc = glGetUniformLocation(shader, "objectColor");
          glUniform3f(colorLoc, 100.0f, 0.0f, 0.0f);
        }
        if(e.key.keysym.sym == SDLK_2){
          colorLoc = glGetUniformLocation(shader, "objectColor");
          glUniform3f(colorLoc, 0.0f, 100.0f, 0.0f);
        }
        if(e.key.keysym.sym == SDLK_3){
          colorLoc = glGetUniformLocation(shader, "objectColor");
          glUniform3f(colorLoc, 0.0f, 0.0f, 100.0f);
        }
        if(e.key.keysym.sym == SDLK_4){
          colorLoc = glGetUniformLocation(shader, "objectColor");
          glUniform3f(colorLoc, 100.0f, 100.0f, 0.0f);
        }
        if(e.key.keysym.sym == SDLK_5){
          colorLoc = glGetUniformLocation(shader, "objectColor");
          glUniform3f(colorLoc, 100.0f, 100.0f, 100.0f);
        }
    }
    return true;
}

void OpenGLWindow::cleanup()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vao);
    SDL_DestroyWindow(sdlWin);
}
