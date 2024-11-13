//#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Kody shader�w
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
out vec3 Color;

// Macierz modelu, widoku, projekcji
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    Color = color;
    // Okre�lenie po�o�enia punkt�w
    gl_Position = proj * view * model * vec4(position, 1.0); //gl_Position = vec4(position, 1.0); //gl_Position = vec4(position, 0.0, 1.0);
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
out vec4 outColor;
void main()
{
    outColor = vec4(Color, 1.0);
}
)glsl";

// Utworzenie zmiennych do ustawienia kamery
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
float sensitivity = 0.1f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

enum Mode
{
    POLYGON, 
    CUBE
};

bool checkShaders(GLuint shader, const std::string& type)
{
    GLint status;
    GLchar log[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status); // GL_COMPILE_STATUS - to chcemy pobra�

    if (!status) // 0 - b��d
    {
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Error: Compilation of " << type << " failed\n" << log << std::endl;
        return false;
    }
    else
    {
        std::cout << "Compilation of " << type << " OK" << std::endl;
        return true;
    }
}

GLfloat* generatePolygon(int N, float r, float z = 0.0f)
{
    const int elementsPerVertex = 6; // (x, y ,z, r, g, b)
    const int totalVertices = N + 2; // N wierzcho�k�w + 1 �rodokowy + 1 ostatni do zamkni�cia w miejscu pierwszego
    GLfloat* vertices = new GLfloat[totalVertices * elementsPerVertex];

    // Wierzcho�ek centralny (�rodek)
    int index = 0;
    vertices[index++] = 0.0f; // x
    vertices[index++] = 0.0f; // y
    vertices[index++] = z;    // z
    vertices[index++] = 1.0f; // r
    vertices[index++] = 1.0f; // g
    vertices[index++] = 1.0f; // b

    float angleStep = 2.0f * M_PI / N;

    const GLfloat colors[6][3] = 
    {
        {1.0f, 0.0f, 0.0f}, // Czerwony
        {0.0f, 1.0f, 0.0f}, // Zielony
        {0.0f, 0.0f, 1.0f}, // Niebieski
        {1.0f, 1.0f, 0.0f}, // ��ty
        {1.0f, 0.0f, 1.0f}, // R�owy
        {0.0f, 1.0f, 1.0f}  // Cyjan
    };

    for (std::size_t i = 0; i < N; i++)
    {
        float alpha = i * angleStep;
        float x = r * cos(alpha);
        float y = r * sin(alpha);

        vertices[index++] = x;    // x
        vertices[index++] = y;    // y
        vertices[index++] = z;    // z

        // Przypisanie koloru na podstawie indeksu wierzcho�ka
        const GLfloat* color = colors[i % 6];
        vertices[index++] = color[0]; // r
        vertices[index++] = color[1]; // g
        vertices[index++] = color[2]; // b
    }

    // Dodanie ostatniego wierzcho�ka, aby zamkn�� wielok�t (ten sam co pierwszy wierzcho�ek)
    vertices[index++] = vertices[6];
    vertices[index++] = vertices[7];
    vertices[index++] = vertices[8];
    vertices[index++] = vertices[9];
    vertices[index++] = vertices[10];
    vertices[index++] = vertices[11];

    return vertices;
}

void ustawKamereMysz(GLint uniView, float deltaTime, sf::Window& window) 
{
    sf::Vector2i localPosition = sf::Mouse::getPosition(window);

    if (firstMouse)
    {
        lastX = localPosition.x;
        lastY = localPosition.y;
        firstMouse = false;
    }

    // Obliczenie przesuni�cia myszy
    float xoffset = localPosition.x - lastX;
    float yoffset = lastY - localPosition.y; // odwrotnie, aby poruszanie w g�r� by�o dodatnie
    lastX = localPosition.x;
    lastY = localPosition.y;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Aktualizacja k�t�w obrotu kamery
    yaw += xoffset;
    pitch += yoffset;

    // Ograniczenie k�ta nachylenia w pionie
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Obliczanie nowego kierunku patrzenia
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
}

void ustawKamereKlawisze(GLint uniView, float deltaTime) 
{
    float cameraSpeed = 0.5f * deltaTime;

    // Poruszanie w prz�d i w ty�
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        cameraPos += cameraSpeed * cameraFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        cameraPos -= cameraSpeed * cameraFront;

    // Poruszanie w lewo i w prawo
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // Poruszanie w g�r� i w d�
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
        cameraPos += cameraSpeed * cameraUp;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
        cameraPos -= cameraSpeed * cameraUp;

    // Obr�t wok� osi Y
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        yaw -= sensitivity * deltaTime * 50.0f; // Obr�t w lewo
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        yaw += sensitivity * deltaTime * 50.0f; // Obr�t w prawo

    // Aktualizacja kierunku frontu patrzenia
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
}



int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;

    int points = 6;
    int oldPosY = 0;
    // CUBE POLYGON
    Mode currentMode = CUBE;
    GLenum prymityw = GL_TRIANGLE_FAN;

    // Okno renderingu
    sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);
    window.setFramerateLimit(20);
    window.setMouseCursorGrabbed(true);
    window.setMouseCursorVisible(false);
    //window.setKeyRepeatEnabled(false);


    // Inicjalizacja GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    // W��czenie z-bufora
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Utworzenie VAO i VBO dla wielok�ta i sze�cianu (Vertex Array/Buffer Object)
    GLuint vaoPolygon, vboPolygon, vaoCube, vboCube;
    glGenVertexArrays(1, &vaoPolygon);
    glGenBuffers(1, &vboPolygon);
    glGenVertexArrays(1, &vaoCube);
    glGenBuffers(1, &vboCube);

    // Wygenerowanie pocz�tkowego wielok�ta
    GLfloat* vertices = generatePolygon(points, 1.0f);

    // Wierzcho�ki dla sze�cianu
    float verticesCube[] =
    {
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f
    };

    // Przesy�anie wierzcho�k�w sze�cianu do VBO
    glBindVertexArray(vaoCube);
    glBindBuffer(GL_ARRAY_BUFFER, vboCube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCube), verticesCube, GL_STATIC_DRAW);

    // Kompilacja shader�w
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    if (!checkShaders(vertexShader, "Vertex shader")) return 1;

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    if (!checkShaders(fragmentShader, "Fragment shader")) return 1;

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Pobieranie atrybut�w dla pozycji i koloru
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    
    float cameraSpeed = 0.05f;
    float obrot = 0.0f;

    // Macierz projekcji
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    bool running = true;
    points = 6;
    prymityw = GL_TRIANGLE_FAN;
    oldPosY = 0;

    GLint uniView = glGetUniformLocation(shaderProgram, "view");

    sf::Clock clock;
    float deltaTime; // przechowuje czas w sekundach jaki up�yn�� od ostatniego od�wie�enia klatki

    while (running)
    {
        deltaTime = clock.restart().asSeconds(); // reset zegara i zwracanie czasu od ostatniego resetu

        static int frameCount = 0;
        static sf::Clock fpsClock;
        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f)
        {
            window.setTitle("OpenGL - FPS: " + std::to_string(frameCount));
            frameCount = 0;
            fpsClock.restart();
        }

        sf::Event windowEvent;
        while (window.pollEvent(windowEvent))
        {
            if (windowEvent.type == sf::Event::Closed)
            {
                running = false;
            }
            else if (windowEvent.type == sf::Event::KeyPressed)
            {
                if (windowEvent.key.code == sf::Keyboard::Escape) 
                {
                    running = false;
                }
                else if (windowEvent.key.code == sf::Keyboard::Space)
                {
                    currentMode = (currentMode == CUBE) ? POLYGON : CUBE;
                }

                if (currentMode == POLYGON)
                {
                    switch (windowEvent.key.code)
                    {
                    case sf::Keyboard::Num1:
                        prymityw = GL_POINTS;
                        break;
                    case sf::Keyboard::Num2:
                        prymityw = GL_LINES;
                        break;
                    case sf::Keyboard::Num3:
                        prymityw = GL_LINE_STRIP;
                        break;
                    case sf::Keyboard::Num4:
                        prymityw = GL_LINE_LOOP;
                        break;
                    case sf::Keyboard::Num5:
                        prymityw = GL_TRIANGLES;
                        break;
                    case sf::Keyboard::Num6:
                        prymityw = GL_TRIANGLE_STRIP;
                        break;
                    case sf::Keyboard::Num7:
                        prymityw = GL_TRIANGLE_FAN;
                        break;
                    case sf::Keyboard::Num8:
                        prymityw = GL_QUADS;
                        break;
                    case sf::Keyboard::Num9:
                        prymityw = GL_POLYGON;
                        break;
                    default:
                        break;
                    }
                }
                else if (currentMode == CUBE)
                {

                }
            }

            else if (windowEvent.type == sf::Event::MouseMoved && currentMode == POLYGON)
            {
                if (windowEvent.mouseMove.y > oldPosY) 
                {
                    points = std::min(points + 1, 20);
                }
                else if (windowEvent.mouseMove.y < oldPosY) 
                {
                    points = std::max(points - 1, 3);
                }

                oldPosY = windowEvent.mouseMove.y;
                delete[] vertices;
                vertices = generatePolygon(points, 1.0f);

                glBindVertexArray(vaoPolygon);
                glBindBuffer(GL_ARRAY_BUFFER, vboPolygon);
                glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (points + 2) * 6, vertices, GL_STATIC_DRAW);
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Macierz widoku
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint uniView = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

        ustawKamereMysz(uniView, deltaTime, window); // Ustawienie widoku kamery na podstawie ruchu myszy
        ustawKamereKlawisze(uniView, deltaTime);     // Obs�uga klawiszy do poruszania si�

        if (currentMode == CUBE) 
        {
            // Macierz modelu                                                              x     y     z
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(obrot), glm::vec3(0.0f, 1.0f, 0.0f));
            
            // Wys�anie do shadera
            GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(vaoCube);

            // W��czenie atrybutu pozycji wierzcho�ka
            glEnableVertexAttribArray(posAttrib);
            glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

            // W��czenie atrybutu koloru
            glEnableVertexAttribArray(colAttrib);
            glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

            // Renderowanie sze�cianu jako zbi�r tr�jk�t�w
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        else if (currentMode == POLYGON) 
        {
            // Maicerz modelu (jednostkowa)
            glm::mat4 model = glm::mat4(1.0f);

            // Wys�anie do shadera
            GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));

            // Ustawienie VAO
            glBindVertexArray(vaoPolygon);

            // W��czenie atrybutu pozycji wierzcho�ka
            glEnableVertexAttribArray(posAttrib);
            glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

            // W��czenie atrybutu koloru
            glEnableVertexAttribArray(colAttrib);
            glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

            // Renderowanie wielok�ta w zadanym trybie
            glDrawArrays(prymityw, 0, points + 2);
        }

        window.display();
    }

    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteBuffers(1, &vboCube);
    glDeleteBuffers(1, &vboPolygon);
    glDeleteVertexArrays(1, &vaoCube);
    glDeleteVertexArrays(1, &vaoPolygon);
    delete[] vertices;
    window.close();
    return 0;
}
