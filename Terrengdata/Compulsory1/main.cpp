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
#include <filesystem>

//Punktdata som viser kart over Nydal
using namespace std;

// Global variables
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// Camera settings
//This is the starting position of the of the camera 
Camera camera(glm::vec3(2.0f, 11.8f, 0.3f));

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
void convertLazToText(const string& inputFilename, const string& outputFilename);
vector<glm::vec3> loadPointsFromTextFile(const string& filename);
std::vector<glm::vec3> loadPointsFromMultipleTextFiles(const std::vector<std::string>& textFiles);

string vfs = ShaderLoader::LoadShaderFromFile("vs.vs");
string fs = ShaderLoader::LoadShaderFromFile("fs.fs");

int main()
{
 
    // Simuler forsinkelse før andre kjøring
    std::this_thread::sleep_for(std::chrono::seconds(2));

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

     // Liste over .laz-filer du vil konvertere
    std::vector<std::string> lazFiles = {
        "32-2-517-155-02.laz", "32-2-517-155-03.laz", "32-2-517-155-12.laz", "32-2-517-155-13.laz", 
        "32-2-517-155-22.laz", "32-2-517-155-23.laz"
    };

    // Liste over tekstfiler som resultat av konverteringen
    std::vector<std::string> textFiles = {
        "32-2-517-155-02.txt", "32-2-517-155-03.txt", "32-2-517-155-12.txt", "32-2-517-155-13.txt", 
        "32-2-517-155-22.txt", "32-2-517-155-23.txt"
    };

    for (size_t i = 0; i < lazFiles.size(); ++i)
    {
        convertLazToText(lazFiles[i], textFiles[i]);
    }

    // Laster alle punktene fra de konverterte tekstfilene
    std::vector<glm::vec3> points = loadPointsFromMultipleTextFiles(textFiles);

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
        glPointSize(3.0f); 
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
    // Slett eksisterende tekstfil hvis den allerede eksisterer (slett kun outputFilename, som er tekstfilen)
    if (std::filesystem::exists(outputFilename))
    {
        std::filesystem::remove(outputFilename); // Slett bare tekstfilen, IKKE .laz-filen
    }

    // Konverteringslogikk for å generere tekstfil basert på inputFilename (som er .laz-filen)
    try
    {
        pdal::Options options;
        options.add("filename", inputFilename);

        pdal::LasReader reader;
        reader.setOptions(options);

        pdal::PointTable table;
        reader.prepare(table);
        pdal::PointViewSet viewSet = reader.execute(table);

        std::ofstream outFile(outputFilename); // Skriv til tekstfilen
        if (!outFile.is_open())
        {
            std::cerr << "Kunne ikke åpne utdatafilen: " << outputFilename << std::endl;
            return;
        }

        size_t totalPoints = 0;
        for (auto& view : viewSet)
        {
            totalPoints += view->size();
        }
        outFile << totalPoints << std::endl;

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
        std::cout << "Konvertering fullført for: " << inputFilename << std::endl;
    }
    catch (const pdal::pdal_error& e)
    {
        std::cerr << "PDAL error: " << e.what() << " i filen " << inputFilename << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Standardfeil: " << e.what() << " i filen " << inputFilename << std::endl;
    }
}

std::vector<glm::vec3> loadPointsFromTextFile(const string& filename)
{
    ifstream inFile(filename);
    vector<glm::vec3> points;

    if (!inFile.is_open())
    {
        cerr << "Kunne ikke åpne filen: " << filename << endl;
        return points;
    }

    size_t numPoints;
    inFile >> numPoints; // Leser antall punkter fra første linje
    cout << "Antall punkter i filen: " << numPoints << endl;

    float x, y, z;

    const float xScaleFactor = 0.0001f;   // Større skaleringsfaktor for x
    const float yScaleFactor = 0.0001f;  // Standard skaleringsfaktor for y
    const float zScaleFactor = 0.0001f; // Forsterk Z for bedre dybde
    const glm::vec3 translationOffset(-59.0f, -663.0f, 0.0f); // Flytter punktene nærmere origo i x og y

    size_t pointCounter = 0;
    while (inFile >> x >> y >> z)
    {
        glm::vec3 point(x * xScaleFactor, y * yScaleFactor, z * zScaleFactor); // Forsterker z for bedre synlighet
        point += translationOffset; // Flytter punktene etter skalering
        points.push_back(point);

        // Skriv ut de første 100 punktene for å bekrefte at de er riktig lastet inn
        if (pointCounter < 10)
        {
            cout << "Punkt " << pointCounter + 1 << ": "
                << point.x << ", " << point.y << ", " << point.z <<endl;
        }
        pointCounter++;
    }

    inFile.close();
    cout << "Totalt antall lastede punkter: " << points.size() <<endl;
    return points;
}

std::vector<glm::vec3> loadPointsFromMultipleTextFiles(const std::vector<std::string>& textFiles)
{
    std::vector<glm::vec3> allPoints;

    for (const auto& textFile : textFiles)
    {
        std::cout << "Laster punkter fra " << textFile << std::endl;
        std::vector<glm::vec3> points = loadPointsFromTextFile(textFile);
        allPoints.insert(allPoints.end(), points.begin(), points.end());
    }

    return allPoints;
}










