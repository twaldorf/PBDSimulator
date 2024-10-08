#include <SDL.h>
#undef main
#include <glad.h>
#include "shader.h"
#include "geometry.h"
#include "camera.h"
#include <vec3.hpp>
#include <mat4x4.hpp>
#include <ext/matrix_transform.hpp>
#include <ext/matrix_clip_space.hpp>
#include <iostream>

SDL_Window* displayInit();

void framebuffer_size_callback(SDL_Window* window, int width, int height);
void processInput(SDL_Window* window);
void mouse_callback(SDL_Window* window, double xposIn, double yposIn);

// window properties
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool WindowShouldClose = false;

// camera initialization
Camera camera(glm::vec3(0.0f, 0.0f, 50.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main() {
    SDL_Window* window = displayInit();
    SDL_Event event;

    Shader shader("vCADShader.glsl", "fCADShader.glsl");

    // object initialization
    Plane ground(glm::vec3(0.0f, -4.0f, 0.0f));
    Surface test_surface(60, 60, 1, 100.f);

    // initial transformations
    glm::mat4 rot = glm::mat4(1.0f);
    rot = glm::translate(rot, glm::vec3(0.0f, 0.0f, 0.1f));
    rot = glm::rotate(rot, glm::radians(90.0f), glm::vec3(1, 0, 0));

    ground.applyTransformation(rot);


    // simulation and display
    while (!WindowShouldClose)
    {
        float currentFrame = SDL_GetTicks();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set up simple shader uniforms
        shader.use();
        shader.setVec3("viewPos", camera.Position);
        shader.setVec3("lightColor", 1.f, 1.f, 1.f);
        shader.setVec3("objectColor", 0.5f, 0.5f, 1.f);

        // view/projection transformations for vertex shader
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f));
        shader.setMat4("model", model);

        // update simulation
        test_surface.updatePoints(deltaTime, glm::vec3(0.f, -9.8f, 0.0f));

        // rotate and shift surface
        glm::mat4 rot = glm::mat4(1.0f);
        rot = glm::rotate(rot, glm::cos(deltaTime) / 1000.f, glm::vec3(1, 0, 1));

        glm::mat4 shift = glm::translate(glm::mat4(1.0f), 1.f * glm::vec3(glm::cos(currentFrame) / 20.f, 0.0f, 0.f));
        test_surface.applyTransToRange(0, test_surface.width, shift);
        test_surface.applyTransToRange(0, test_surface.width, rot);

        // set frag shader uniform for dynamic light position from debug plane
        glm::vec3 center = ground.getCenter();
        shader.setVec3("lightPos", center.x, center.y, center.z);

        // draw surface and debug plane
        test_surface.Draw(shader);
        ground.Draw(shader);

        SDL_GL_SwapWindow(window);
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT)
            WindowShouldClose = true;
    }

    SDL_Quit();

    return 0;
}


SDL_Window* displayInit() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Failed to initialize SDL" << std::endl;
    }
    SDL_GL_LoadLibrary(NULL);

    SDL_Surface* screenSurface = NULL;

    SDL_Window* window = SDL_CreateWindow("PBD Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_OPENGL);

    screenSurface = SDL_GetWindowSurface(window);

    SDL_UpdateWindowSurface(window);

    // load GL
    SDL_GLContext context = SDL_GL_CreateContext(window);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_MakeCurrent(window, context);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    // standard vertical sync
    SDL_GL_SetSwapInterval(1);

    // disable depth buffer
    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    return window;
}

void processInput(SDL_Window* window) {
    /* if (SDL_GetKey(window, SDL_KEY_ESCAPE) == SDL_PRESS) {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }*/
}

void mouse_callback(SDL_Window* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    // camera.ProcessMouseMovement(xoffset, yoffset);
}

void framebuffer_size_callback(SDL_Window* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
