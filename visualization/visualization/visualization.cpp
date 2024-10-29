//#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Kody shaderów
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
    // Okreœlenie po³o¿enia punktów
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


bool checkShaders(GLuint shader, const std::string& type)
{
    GLint status;
    GLchar log[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status); // GL_COMPILE_STATUS - to chcemy pobraæ

    if (!status) // 0 - b³¹d
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
    const int totalVertices = N + 2; // N wierzcho³ków + 1 œrodokowy + 1 ostatni do zamkniêcia w miejscu pierwszego
    GLfloat* vertices = new GLfloat[totalVertices * elementsPerVertex];

    // Wierzcho³ek centralny (œrodek)
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
        {1.0f, 1.0f, 0.0f}, // ¯ó³ty
        {1.0f, 0.0f, 1.0f}, // Ró¿owy
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

        // Przypisanie koloru na podstawie indeksu wierzcho³ka
        const GLfloat* color = colors[i % 6];
        vertices[index++] = color[0]; // r
        vertices[index++] = color[1]; // g
        vertices[index++] = color[2]; // b
    }

    // Dodanie ostatniego wierzcho³ka, aby zamkn¹æ wielok¹t (ten sam co pierwszy wierzcho³ek)
    vertices[index++] = vertices[6];
    vertices[index++] = vertices[7];
    vertices[index++] = vertices[8];
    vertices[index++] = vertices[9];
    vertices[index++] = vertices[10];
    vertices[index++] = vertices[11];

    return vertices;
}



int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;

    int points = 6;
    int oldPosY = 0;
    GLenum prymityw = GL_TRIANGLE_FAN;

    // Okno renderingu
    sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

    // Inicjalizacja GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);

    // Utworzenie VAO (Vertex Array Object)
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Utworzenie VBO (Vertex Buffer Object)
    GLuint vbo;
    glGenBuffers(1, &vbo);

    // Wygenerowanie pocz¹tkowego wielok¹ta
    GLfloat* vertices = generatePolygon(points, 1.0f);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (points + 2) * 6, vertices, GL_STATIC_DRAW);

    // Utworzenie i skompilowanie shaderów
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    //checkShaders(vertexShader, "Vertex Shader");

    if (!checkShaders(vertexShader, "Vertex shader"))
    {
        return 1;
    }


    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
   /// checkShaders(fragmentShader, "Fragment Shader");

    if (!checkShaders(fragmentShader, "Fragment Shader"))
    {
        return 1;
    }

    // Zlinkowanie shaderów
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Specyfikacja formatu danych wierzcho³kowych
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    // Deklaracja zmiennych zwi¹zanych z kamer¹
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float cameraSpeed = 0.05f;
    float obrot = 0.0f;


    // Macierz modelu
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    GLint uniModel = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));


    // Macierz widoku
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    view = glm::lookAt
       (glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));


    // Macierz projekcji
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));



    // Rozpoczêcie pêtli zdarzeñ
    bool running = true;
    while (running)
    {
        sf::Event windowEvent;
        while (window.pollEvent(windowEvent))
        {
            switch (windowEvent.type)
            {
            case sf::Event::Closed:
                running = false;
                break;

            case sf::Event::KeyPressed:
                switch (windowEvent.key.code)
                {
                case sf::Keyboard::Escape:
                    running = false;
                    break;

                case sf::Keyboard::Num1:
                    prymityw = GL_LINES;
                    break;

                case sf::Keyboard::Num2:
                    prymityw = GL_POINTS;
                    break;

                case sf::Keyboard::Num3:
                    prymityw = GL_TRIANGLE_FAN;
                    break;

                case sf::Keyboard::Num4:
                    prymityw = GL_LINE_STRIP;
                    break;

                case sf::Keyboard::Num5:
                    prymityw = GL_LINE_LOOP;
                    break;

                case sf::Keyboard::Num6:
                    prymityw = GL_TRIANGLES;
                    break;

                case sf::Keyboard::Num7:
                    prymityw = GL_TRIANGLE_STRIP;
                    break;

                case sf::Keyboard::Num8:
                    prymityw = GL_QUADS;
                    break;

                case sf::Keyboard::Num9:
                    prymityw = GL_QUAD_STRIP;
                    break;

                case sf::Keyboard::Num0:
                    prymityw = GL_POLYGON;
                    break;

                case sf::Keyboard::W:
                    cameraPos += cameraSpeed * cameraFront;
                    break;

                case sf::Keyboard::S:
                    cameraPos -= cameraSpeed * cameraFront;
                    break;

                case sf::Keyboard::A:
                    cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                    break;

                case sf::Keyboard::D:
                    cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                    break;
                /*case sf::Keyboard::Q:
                    obrot += 0.05f;
                    cameraFront.x = sin(obrot);
                    cameraFront.z = -cos(obrot);
                    cameraFront = glm::normalize(cameraFront);
                    break;

                case sf::Keyboard::E:
                    obrot -= 0.05f;
                    cameraFront.x = sin(obrot);
                    cameraFront.z = -cos(obrot);
                    cameraFront = glm::normalize(cameraFront);
                    break;*/
                }
                break;

            case sf::Event::MouseMoved:
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

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (points + 2) * 6, vertices, GL_STATIC_DRAW);
                break;
            }
        }

        // Nadanie scenie koloru czarnego
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

        glDrawArrays(prymityw, 0, points + 2);

        // Wymiana buforów tylni/przedni
        window.display();
    }

    // Kasowanie programu i czyszczenie buforów
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    // Zamkniêcie okna renderingu
    window.close();
    return 0;
}
