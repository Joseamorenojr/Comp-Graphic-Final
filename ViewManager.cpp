///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;
    const char* g_ViewName = "view";
    const char* g_ProjectionName = "projection";

    Camera* g_pCamera = nullptr;

    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;

    bool bOrthographicProjection = false;
}

// Add forward declaration for Mouse Scroll Callback
void Mouse_Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset);

ViewManager::ViewManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_pWindow = NULL;
    g_pCamera = new Camera();
    g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
    g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
    g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    g_pCamera->Zoom = 80;
    g_pCamera->MovementSpeed = 20;
}

ViewManager::~ViewManager()
{
    m_pShaderManager = NULL;
    m_pWindow = NULL;
    if (g_pCamera != NULL)
    {
        delete g_pCamera;
        g_pCamera = NULL;
    }
}

GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
    glfwSetScrollCallback(window, Mouse_Scroll_Callback); // use non-member function

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;
    return window;
}

void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
    if (gFirstMouse)
    {
        gLastX = xMousePos;
        gLastY = yMousePos;
        gFirstMouse = false;
    }

    float xOffset = xMousePos - gLastX;
    float yOffset = gLastY - yMousePos;
    gLastX = xMousePos;
    gLastY = yMousePos;

    g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

// Define the scroll callback outside the class
void Mouse_Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_pCamera->MovementSpeed += (float)yoffset;
    if (g_pCamera->MovementSpeed < 1.0f)
        g_pCamera->MovementSpeed = 1.0f;
}

void ViewManager::ProcessKeyboardEvents()
{
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_pWindow, true);

    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);

    if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
        bOrthographicProjection = false;
    if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
        bOrthographicProjection = true;
}

void ViewManager::PrepareSceneView()
{
    glm::mat4 view;
    glm::mat4 projection;

    float currentFrame = glfwGetTime();
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    ProcessKeyboardEvents();

    view = g_pCamera->GetViewMatrix();

    if (bOrthographicProjection)
    {
        float scale = 10.0f;
        projection = glm::ortho(-scale, scale, -scale * (float)WINDOW_HEIGHT / WINDOW_WIDTH,
            scale * (float)WINDOW_HEIGHT / WINDOW_WIDTH, 0.1f, 100.0f);

        g_pCamera->Front = glm::vec3(0.0f, -1.0f, -1.0f);
    }
    else
    {
        projection = glm::perspective(glm::radians(g_pCamera->Zoom),
            (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    if (m_pShaderManager != NULL)
    {
        m_pShaderManager->setMat4Value(g_ViewName, view);
        m_pShaderManager->setMat4Value(g_ProjectionName, projection);
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}
