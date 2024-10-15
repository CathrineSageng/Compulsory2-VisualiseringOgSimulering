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

#include <pdal/pdal.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/Options.hpp>
#include <fstream>
#include <cstdlib> // For rand()
#include <ctime>   // For time()


using namespace std;

// Global variables
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// Camera settings
//This is the starting position of the of the camera 
Camera camera(glm::vec3(0.5f, 0.5f, 30.0f));

//Keeps track of the last position of the mouse cursor 
GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
//Avoids sudden jumps in the camera orientation when the mouse is first detected. 
bool firstMouse = true;

// Time between current frame and last frame
float deltaTime = 0.0f;
//Stores the timestamp of previous frame. 
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void convertLazToText(const std::string& inputFilename, const std::string& outputFilename);
std::vector<glm::vec3> loadPointsFromTextFile(const std::string& filename);

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Terreng", NULL, NULL);
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

    // Konverter .laz-filen til en tekstfil
    convertLazToText("32-1-512-125-45.laz", "utdata.txt");

    // Les punktene fra tekstfilen
    std::vector<glm::vec3> points = loadPointsFromTextFile("utdata.txt");

    // Sjekk at vi faktisk har punkter
    if (points.empty())
    {
        std::cerr << "Ingen punkter å rendre." << std::endl;
        return -1;
    }

    // Opprett VAO og VBO for punktene
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind VAO og VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);

    // Konfigurer vertex-attributt
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Frigjør bufferet
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


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

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);

        // Tegn punktsky
        glBindVertexArray(VAO);
        glPointSize(20.0f); // Juster punktstørrelse
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
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


void convertLazToText(const std::string& inputFilename, const std::string& outputFilename)
{
    // Lag et Options-objekt fra PDAL for å spesifisere filen
    pdal::Options options;
    options.add("filename", inputFilename);

    // Sett opp PDAL LasReader med de spesifiserte opsjonene
    pdal::LasReader reader;
    reader.setOptions(options);

    // Lag en PointTable for å lagre punktdataene
    pdal::PointTable table;
    reader.prepare(table);
    pdal::PointViewSet viewSet = reader.execute(table);

    // Åpne utdatafilen for skriving
    std::ofstream outFile(outputFilename);
    if (!outFile.is_open())
    {
        std::cerr << "Kunne ikke åpne utdatafilen." << std::endl;
        return;
    }

    // Telle og skrive antall punkter på første linje i tekstfilen
    size_t totalPoints = 0;
    for (auto& view : viewSet)
    {
        totalPoints += view->size();
    }
    outFile << totalPoints << std::endl;

    // Iterer gjennom punktene og skriv dem til filen
    for (auto& view : viewSet)
    {
        for (pdal::PointId i = 0; i < view->size(); ++i)
        {
            double x = view->getFieldAs<double>(pdal::Dimension::Id::X, i);
            double y = view->getFieldAs<double>(pdal::Dimension::Id::Y, i);
            double z = view->getFieldAs<double>(pdal::Dimension::Id::Z, i);
            outFile << x << " " << y << " " << z << std::endl;
        }
    }

    outFile.close();
    std::cout << "Konvertering fullført! Punktdata skrevet til " << outputFilename << std::endl;
}

std::vector<glm::vec3> loadPointsFromTextFile(const std::string& filename)
{
    std::ifstream inFile(filename);
    std::vector<glm::vec3> points;

    if (!inFile.is_open())
    {
        std::cerr << "Kunne ikke åpne filen: " << filename << std::endl;
        return points;
    }

    size_t numPoints;
    inFile >> numPoints; // Leser antall punkter fra første linje
    std::cout << "Antall punkter i filen: " << numPoints << std::endl;

    float x, y, z;
    //const float scaleFactor = 0.00001f; // Skaleringsfaktor for å bringe punktene nærmere kameraet
    //const glm::vec3 translationOffset(-5.8f, -66.03f, -5.0f); // Justering for å bringe punktene til origo

    const float xScaleFactor = 0.001f;   // Større skaleringsfaktor for x
    const float yScaleFactor = 0.001f;  // Standard skaleringsfaktor for y
    const float zScaleFactor = 0.001f * 100.0f; // Forsterk Z for bedre dybde
    /*const float scaleFactor = 0.001f;*/ // Øk skaleringsfaktoren for å se om spredningen blir bedre
    const glm::vec3 translationOffset(-580.0f, -6603.0f, 0.0f); // Flytter punktene nærmere origo i x og y

    size_t pointCounter = 0;
    while (inFile >> x >> y >> z)
    {
        /*glm::vec3 point(x * scaleFactor, y * scaleFactor, z * scaleFactor);*/
        glm::vec3 point(x * xScaleFactor, y * yScaleFactor, z * zScaleFactor); // Forsterker z for bedre synlighet
        point += translationOffset; // Flytter punktene etter skalering
        points.push_back(point);

        // Skriv ut de første 10 punktene for å bekrefte at de er riktig lastet inn
        if (pointCounter < 100)
        {
            std::cout << "Punkt " << pointCounter + 1 << ": "
                << point.x << ", " << point.y << ", " << point.z << std::endl;
        }
        pointCounter++;
    }

    inFile.close();
    std::cout << "Totalt antall lastede punkter: " << points.size() << std::endl;
    return points;
}









