#define _USE_MATH_DEFINES
#define STB_IMAGE_IMPLEMENTATION
#include <cmath>
#include <SFML/Window.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include "shaders.h"
#include "stb_image.h"

// Utworzenie zmiennych do ustawienia kamery
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
float sensitivity = 0.1f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

// Funkcja do sprawdzania b³êdów shaderów
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

// Funkcja do sprawdzania b³êdów OpenGL
void checkGLErrors(const std::string& context) 
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error in " << context << ": " << err << std::endl;
    }
}

// Funkcje ustawiaj¹ce kamerê
void setCameraMouse(GLint uniView, float deltaTime, sf::Window& window)
{
    sf::Vector2i localPosition = sf::Mouse::getPosition(window);

    if (firstMouse)
    {
        lastX = localPosition.x;
        lastY = localPosition.y;
        firstMouse = false;
    }

    // Obliczenie przesuniêcia myszy
    float xoffset = localPosition.x - lastX;
    float yoffset = lastY - localPosition.y; // odwrotnie, aby poruszanie w górê by³o dodatnie
    lastX = localPosition.x;
    lastY = localPosition.y;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Aktualizacja k¹ta obrotu kamery
    yaw += xoffset;
    pitch += yoffset;

    // Ograniczenie k¹ta nachylenia w pionie
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Obliczanie nowego kierunku patrzenia
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void setCameraKeys(float deltaTime)
{
    float cameraSpeed = 5.0f * deltaTime; // Zwiêkszono prêdkoœæ kamery

    // Poruszanie w przód i w ty³
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        cameraPos += cameraSpeed * cameraFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        cameraPos -= cameraSpeed * cameraFront;

    // Poruszanie w lewo i w prawo
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // Poruszanie w górê i w dó³
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
        cameraPos += cameraSpeed * cameraUp;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
        cameraPos -= cameraSpeed * cameraUp;
}

struct Vertex 
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

bool loadObj(const std::string& filePath, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Cannot open file: " << filePath << std::endl;
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords; // Przechowywanie UV

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {  // Vertex position
            glm::vec3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (prefix == "vt") {  // Vertex texture coordinate
            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        }
        else if (prefix == "vn") {  // Vertex normal
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "f") {  // Face
            std::string vertexStr;
            for (int i = 0; i < 3; i++) {
                if (!(iss >> vertexStr)) {
                    std::cerr << "Error: Not enough vertex data in face" << std::endl;
                    return false;
                }

                std::istringstream viss(vertexStr);
                std::string posStr, texStr, normStr;
                size_t firstSlash = vertexStr.find('/');
                size_t lastSlash = vertexStr.rfind('/');

                unsigned int posIdx = 0;
                unsigned int texIdx = 0;
                unsigned int normIdx = 0;

                if (firstSlash == std::string::npos) {
                    // Only position
                    posIdx = std::stoi(vertexStr);
                }
                else if (firstSlash == lastSlash) {
                    // Format: v/vt
                    posIdx = std::stoi(vertexStr.substr(0, firstSlash));
                    texIdx = std::stoi(vertexStr.substr(firstSlash + 1));
                }
                else {
                    // Format: v/vt/vn or v//vn
                    posIdx = std::stoi(vertexStr.substr(0, firstSlash));
                    if (lastSlash > firstSlash + 1) {
                        // v/vt/vn
                        texIdx = std::stoi(vertexStr.substr(firstSlash + 1, lastSlash - firstSlash - 1));
                        normIdx = std::stoi(vertexStr.substr(lastSlash + 1));
                    }
                    else {
                        // v//vn
                        normIdx = std::stoi(vertexStr.substr(lastSlash + 1));
                    }
                }

                // Kontrola zakresów indeksów
                if (posIdx < 1 || posIdx > positions.size()) {
                    std::cerr << "Error: Position index out of range in face: " << posIdx << std::endl;
                    return false;
                }
                if (texIdx < 1 || texIdx > texCoords.size()) {
                    std::cerr << "Error: Texture index out of range in face: " << texIdx << std::endl;
                    return false;
                }
                if (normIdx < 1 || normIdx > normals.size()) {
                    std::cerr << "Error: Normal index out of range in face: " << normIdx << std::endl;
                    return false;
                }

                Vertex vertex;
                vertex.position = positions[posIdx - 1];
                vertex.normal = normals[normIdx - 1];
                vertex.texCoord = texCoords[texIdx - 1]; // Przypisanie UV

                vertices.push_back(vertex);
                indices.push_back(vertices.size() - 1);
            }
        }
    }

    std::cout << "Loaded OBJ: " << filePath << " with "
        << vertices.size() << " vertices and "
        << indices.size() << " indices." << std::endl;
    return true;
}

void setObjectColor(GLuint shaderProgram, GLint uniObjectColor, float r, float g, float b, float a) 
{
    if (uniObjectColor != -1) 
    {
        glUniform4f(uniObjectColor, r, g, b, a);
    }
    else 
    {
        std::cerr << "Warning: 'objectColor' uniform not found." << std::endl;
    }
}

GLuint loadTexture(const char* filepath)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    // Wczytaj obraz w pionowej orientacji
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Ustawienia tekstury
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        stbi_image_free(data);
        return 0; // Zwrot 0 w przypadku niepowodzenia
    }

    return textureID;
}

int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 2;
    settings.attributeFlags = sf::ContextSettings::Core;

    // Okno renderingu
    sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    // Inicjalizacja GLEW
    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) 
    {
        std::cerr << "GLEW Initialization failed: " << glewGetErrorString(glewStatus) << std::endl;
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    checkGLErrors("After GLEW Init");

    // W³¹czenie z-bufora
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE); // Wy³¹czenie culling


    // Kompilacja shaderów
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    if (!checkShaders(vertexShader, "Vertex shader")) return 1;

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    if (!checkShaders(fragmentShader, "Fragment shader")) return 1;

    // Linkowanie shaderów do programu
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    // Przypisanie lokalizacji atrybutów przed linkowaniem
    glBindAttribLocation(shaderProgram, 0, "position");
    glBindAttribLocation(shaderProgram, 1, "normal");

    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);

    // Sprawdzenie linkowania programu
    GLint programStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &programStatus);
    if (!programStatus) 
    {
        GLchar log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cerr << "Error: Linking shader program failed\n" << log << std::endl;
        return -1;
    }
    std::cout << "Shader program linked successfully." << std::endl;


    GLuint chairTexture = loadTexture("wood.png");
    GLuint tableTexture = loadTexture("wood.png");

    bool hasChairTexture = (chairTexture != 0);
    bool hasTableTexture = (tableTexture != 0);

    if (!hasChairTexture || !hasTableTexture)
    {
        std::cerr << "Failed to load one or more textures. Using object colors instead." << std::endl;
    }

    glUseProgram(shaderProgram);
    checkGLErrors("After Shader Program Linking");

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // Pobierz lokalizacjê uniformów
    GLint uniModel = glGetUniformLocation(shaderProgram, "model");
    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    GLint uniObjectColor = glGetUniformLocation(shaderProgram, "objectColor");
    GLint uniUseTexture = glGetUniformLocation(shaderProgram, "useTexture");

    if (uniObjectColor == -1) 
    {
        std::cerr << "Warning: 'objectColor' uniform not found." << std::endl;
    }

    // Za³aduj modele
    std::vector<Vertex> chairVertices;
    std::vector<unsigned int> chairIndices;

    std::vector<Vertex> tableVertices;
    std::vector<unsigned int> tableIndices;

    if (!loadObj("chair.obj", chairVertices, chairIndices)) 
    {
        std::cerr << "Error loading chair.obj" << std::endl;
        return -1;
    }

    if (!loadObj("table.obj", tableVertices, tableIndices)) 
    {
        std::cerr << "Error loading table.obj" << std::endl;
        return -1;
    }

    // VAO i VBO dla krzes³a
    GLuint vaoChair, vboChair, eboChair;
    glGenVertexArrays(1, &vaoChair);
    glGenBuffers(1, &vboChair);
    glGenBuffers(1, &eboChair);

    glBindVertexArray(vaoChair);

    glBindBuffer(GL_ARRAY_BUFFER, vboChair);
    glBufferData(GL_ARRAY_BUFFER, chairVertices.size() * sizeof(Vertex), &chairVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboChair);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, chairIndices.size() * sizeof(unsigned int), &chairIndices[0], GL_STATIC_DRAW);

    // Ustawienia atrybutów
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); // Dodane UV
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    checkGLErrors("After setting up VAO Chair");

    // VAO i VBO dla sto³u
    GLuint vaoTable, vboTable, eboTable;
    glGenVertexArrays(1, &vaoTable);
    glGenBuffers(1, &vboTable);
    glGenBuffers(1, &eboTable);

    glBindVertexArray(vaoTable);

    glBindBuffer(GL_ARRAY_BUFFER, vboTable);
    glBufferData(GL_ARRAY_BUFFER, tableVertices.size() * sizeof(Vertex), &tableVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboTable);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, tableIndices.size() * sizeof(unsigned int), &tableIndices[0], GL_STATIC_DRAW);

    // Ustawienia atrybutów
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); // Dodane UV
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    checkGLErrors("After setting up VAO Table");

    // Ustaw macierz projekcji
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    // Ustaw pocz¹tkow¹ macierz widoku
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

    checkGLErrors("After setting uniforms");

    bool running = true;

    sf::Clock clock;
    float deltaTime; // przechowuje czas w sekundach jaki up³yn¹³ od ostatniego odœwie¿enia klatki

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
            }
        }

        // Czyszczenie ekranów
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Aktualizacja kamery
        setCameraMouse(uniView, deltaTime, window);
        setCameraKeys(deltaTime);

        // Aktualizacja macierzy widoku po zmianie kamery
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

        // Renderowanie krzes³a
        glActiveTexture(GL_TEXTURE0);
        if (hasChairTexture)
        {
            glBindTexture(GL_TEXTURE_2D, chairTexture);
            glUniform1i(uniUseTexture, GL_TRUE);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0); // Odwi¹zanie tekstury
            glUniform1i(uniUseTexture, GL_FALSE);
            // Ustaw kolor krzes³a na czerwony
            glUniform4f(uniObjectColor, 1.0f, 0.0f, 0.0f, 1.0f);
        }

        // Ustaw macierz modelu dla krzes³a
        glm::mat4 chairModel = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, -5.0f));
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(chairModel));

        // Rysowanie krzes³a
        glBindVertexArray(vaoChair);
        glDrawElements(GL_TRIANGLES, chairIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        checkGLErrors("After drawing Chair");

        // Renderowanie sto³u
        glActiveTexture(GL_TEXTURE0);
        if (hasTableTexture)
        {
            glBindTexture(GL_TEXTURE_2D, tableTexture);
            glUniform1i(uniUseTexture, GL_TRUE);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
            glUniform1i(uniUseTexture, GL_FALSE);
            // Ustaw kolor sto³u na ¿ó³ty
            glUniform4f(uniObjectColor, 1.0f, 1.0f, 0.0f, 1.0f);
        }

        // Ustaw macierz modelu dla sto³u
        glm::mat4 tableModel = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, -5.0f));
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(tableModel));

        // Rysowanie sto³u
        glBindVertexArray(vaoTable);
        glDrawElements(GL_TRIANGLES, tableIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        checkGLErrors("After drawing Table");

        window.display();
    }

    // Sprz¹tanie
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteVertexArrays(1, &vaoChair);
    glDeleteBuffers(1, &vboChair);
    glDeleteBuffers(1, &eboChair);
    glDeleteVertexArrays(1, &vaoTable);
    glDeleteBuffers(1, &vboTable);
    glDeleteBuffers(1, &eboTable);

    if (hasChairTexture)
        glDeleteTextures(1, &chairTexture);
    if (hasTableTexture)
        glDeleteTextures(1, &tableTexture);

    window.close();
    return 0;
}
