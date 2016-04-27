#include <iostream>
#include <stdio.h>

#include "SDL.h"
#include <GL/glew.h>
#include <GL/glut.h>

#include "glwindow.h"
#include "geometry.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
GeometryData geometry;

enum WindowState
{
    VIEW,
    ROTATE,
    SCALE
};
WindowState currentWindowType;
int colorLoc;

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

    modelMat4 = glm::rotate(modelMat4, 45.0f, glm::vec3(0.0f,0.0f,1.0f));
    modelMat4 = glm::scale(modelMat4, glm::vec3(0.5f,0.5f,0.5f));
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
        std::cout <<"rortate that bitch"<<std::endl;
        //geometry.rotateObject();
        //glBufferData(GL_ARRAY_BUFFER, geometry.vertexCount() * sizeof(geometry.textureCoordData()), geometry.vertexData(), GL_STATIC_DRAW);
        break;
      }
      case 2:
      {
        //std::cout<<"Scale"<<std::endl;
        break;
      }
    }
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
        if(e.key.keysym.sym == SDLK_ESCAPE)
        {
            return false;
        }

        if(e.key.keysym.sym == SDLK_r){         //step through rotation axis none, x, y, z, wrap around
          if(currentWindowType == ROTATE){
            currentWindowType = VIEW;
          }else{
            currentWindowType = ROTATE;
          }
        }

        if(e.key.keysym.sym == SDLK_s){
          if(currentWindowType == SCALE){
            currentWindowType = VIEW;
          }else{
            currentWindowType = SCALE;
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
