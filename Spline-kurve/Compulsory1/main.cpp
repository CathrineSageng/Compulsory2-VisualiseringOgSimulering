#include<iostream>
#include "glm/mat4x3.hpp"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<vector>
#include "Shader.h"
#include "ShaderFileLoader.h"
#include "Camera.h"

using namespace std;

// Global variables
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// Camera settings
//This is the starting position of the of the camera 
Camera camera(glm::vec3(1.0f, 1.0f, 5.0f));
//Keeps track of the last position of the mouse cursor 
GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
//Avoids sudden jumps in the camera orientation when the mouse is first detected. 
bool firstMouse = true;

// Time between current frame and last frame
float deltaTime = 0.0f;
//Stores the timestamp of previous frame. 
float lastFrame = 0.0f;

//The control points from the Matematikk 3 lecture file, chapter 12. 
std::vector<glm::vec3> controlPoints = {
    glm::vec3(0.0f, 0.0f,  0.0f), glm::vec3(1.0f, 0.0f,  0.0f), glm::vec3(2.0f, 0.0f,  0.0f), glm::vec3(3.0f, 0.0f,  0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f,  2.0f), glm::vec3(2.0f, 1.0f,  2.0f), glm::vec3(3.0f, 1.0f,  0.0f),
    glm::vec3(0.0f, 2.0f,  0.0f), glm::vec3(1.0f,  2.0f,  0.0f), glm::vec3(2.0f,  2.0f,  0.0f), glm::vec3(3.0f,  2.0f,  0.0f),
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
glm::vec3 BSplineSurface(float u, float v, const vector<glm::vec3>& controlPoints, int width);


std::string vfs = ShaderLoader::LoadShaderFromFile("vs.vs");
std::string fs = ShaderLoader::LoadShaderFromFile("fs.fs");

int main()
{
    std::cout << "vfs " << vfs.c_str() << std::endl;
    std::cout << "fs " << fs.c_str() << std::endl;
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Spline-kurver", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("vs.vs", "fs.fs"); // you can name your shader files however you like

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    //Generating surface points for the B-spline surface 
    vector<glm::vec3> surfacePoints;
    //The number of points on the surface in each direction. Here 100 points will be calculated 
    int pointsOnTheSurface = 10; 
    for (int i = 0; i < pointsOnTheSurface; ++i) 
    {
        for (int j = 0; j < pointsOnTheSurface; ++j) 
        {
            //Normalizes u and v to be in a range of 0 to 1
            float u = i / static_cast<float>(pointsOnTheSurface - 1);
            float v = j / static_cast<float>(pointsOnTheSurface - 1);
            //The width of 4 is telling how many control points there are in each row 
            surfacePoints.push_back(BSplineSurface(u, v, controlPoints, 4));
        }
    }

   //Generates wireframe for the B-spline surface
   //Connects the surface points in a grid with lines 
    vector<unsigned int> indices;
    for (int i = 0; i < pointsOnTheSurface - 1; ++i) 
    {
        for (int j = 0; j < pointsOnTheSurface - 1; ++j) 
        {
            //Connects a point (i,j) to a point directly under it at (i+1,j)
            indices.push_back(i * pointsOnTheSurface + j);
            indices.push_back((i + 1) * pointsOnTheSurface + j);
            //Connects a point (i,j) to a point directly to its right at (i,j+1)
            indices.push_back(i * pointsOnTheSurface + j);
            indices.push_back(i * pointsOnTheSurface + (j + 1));
        }
    }

    //For the wireframe 
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, surfacePoints.size() * sizeof(glm::vec3), &surfacePoints[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    //Points on the surface
    unsigned int surfaceVBO, surfaceVAO;
    glGenVertexArrays(1, &surfaceVAO);
    glGenBuffers(1, &surfaceVBO);

    glBindVertexArray(surfaceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, surfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, surfacePoints.size() * sizeof(glm::vec3), &surfacePoints[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //For the contol points 
    unsigned int controlVBO, controlVAO;
    glGenVertexArrays(1, &controlVAO);
    glGenBuffers(1, &controlVBO);

    glBindVertexArray(controlVAO);
    glBindBuffer(GL_ARRAY_BUFFER, controlVBO);
    glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(glm::vec3), &controlPoints[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        // Time calculation for movement
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);

        //Render wireframe
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Render surface points
        glBindVertexArray(surfaceVAO);
        glPointSize(6.0f);
        glDrawArrays(GL_POINTS, 0, surfacePoints.size());
        glBindVertexArray(0);

        // Render control points
        glBindVertexArray(controlVAO);
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, controlPoints.size());
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

//Calculates the points on a biquadratic B-spline surface. 
glm::vec3 BSplineSurface(float u, float v, const vector<glm::vec3>& controlPoints, int width)
{
    //u and v are parameters between 0 and 1. u is horisontal direction and v i vertical direction. 
    //The control points defines the shape of the surface. 
    //witdh it hod many control points there are in the horisontal direction 
    //(1.0f - u) * (1.0f - u) is the first basis function for quadratic B-spline.
    // 2 * u * (1.0f - u) is the second basis function for quadratic B-spline.
    //u * u is the third basis function for quadratic B-spline.
    float n_u[3] = { (1.0f - u) * (1.0f - u), 2 * u * (1.0f - u), u * u };
    float n_v[3] = { (1.0f - v) * (1.0f - v), 2 * v * (1.0f - v), v * v };

    //The function only takes the grid of 3x3 of control points. So it is only accessing the 3 first points in both the horisontal and vertical 
    //direction. If the function takes a grid of 4x3 we need to switch it to a cubic B-spline. 
    //The nedted loop iterates in the vertical and horisontal direction 
    //is the 3D position of the control point. 
    glm::vec3 point(0.0f);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            point += n_u[i] * n_v[j] * controlPoints[i * width + j];
        }
    }
    return point;
}








