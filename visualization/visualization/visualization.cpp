//#include "stdafx.h"
#define _USE_MATH_DEFINES
#define STB_IMAGE_IMPLEMENTATION
#include <cmath>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
in vec2 aTexCoord;
in vec3 aNormal;

out vec3 Color;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

// Macierz modelu, widoku, projekcji
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;


void main()
{
    Normal = aNormal;
    Color = color;
    TexCoord = aTexCoord;
    FragPos = vec3(model * vec4(position, 1.0));
    // Okreœlenie po³o¿enia punktów
    gl_Position = proj * view * model * vec4(position, 1.0); 
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
out vec4 outColor;

uniform sampler2D texture1;
uniform int lightingType; // 0: Directional, 1: Point, 2: Spotlight
uniform bool lightingEnabled;

// Wspólne zmienne dla œwiate³
uniform vec3 lightPos;   // Pozycja dla Point/Spotlight
uniform vec3 lightDir;   // Kierunek dla Directional/Spotlight
uniform vec3 viewPos;    // Pozycja kamery (do specular)
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0); // Kolor œwiat³a

// Parametry t³umienia (dla Point/Spotlight)
uniform float constant = 1.0;
uniform float linear = 0.09;
uniform float quadratic = 0.032;

// Parametry dla reflektora
uniform float cutoff = 12.5;
uniform float outerCutoff = 15.0;

// Parametry oœwietlenia
uniform float ambientStrength = 0.1;
uniform float diffuseStrength = 1.0;
uniform float specularStrength = 0.5;
uniform float shininess = 32.0;

void main()
{
    if (!lightingEnabled) 
    {
        outColor = texture(texture1, TexCoord);
        return;
    }

    vec3 ambient = ambientStrength * lightColor; // Ambient - wspólne dla wszystkich

    vec3 norm = normalize(Normal); // Normalizacja normalnej
    vec3 lightDirection;
    float attenuation = 1.0;      // T³umienie, domyœlnie brak
    float spotlightEffect = 1.0;  // Efekt sto¿ka reflektora, domyœlnie brak

    if (lightingType == 0) // Œwiat³o kierunkowe
    {
        lightDirection = normalize(-lightDir);
    }
    else if (lightingType == 1 || lightingType == 2) // Punktowe lub reflektor
    {
        lightDirection = normalize(lightPos - FragPos);

        // Obliczenie t³umienia dla œwiat³a punktowego
        float distance = length(lightPos - FragPos);
        attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);
    }

    if (lightingType == 2) // Reflektor
    {
        float theta = dot(lightDirection, normalize(-lightDir)); // K¹t miêdzy kierunkiem œwiat³a a obiektem
        if(theta > cutoff)
        {
            float epsilon = cos(radians(cutoff)) - cos(radians(outerCutoff));
            spotlightEffect = clamp((theta - cos(radians(outerCutoff))) / epsilon, 0.0, 1.0);
            attenuation *= spotlightEffect;
        }
        else
        {
            attenuation = 0.0;
        }
    }

    // Diffuse
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Po³¹czenie wszystkich komponentów
    vec3 lighting = ambient + attenuation * spotlightEffect * (diffuse + specular);

    outColor = vec4(lighting, 1.0) * texture(texture1, TexCoord);
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

void ustawKamereMysz(GLint uniView, float deltaTime, sf::Window& window) 
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

    // Aktualizacja k¹tów obrotu kamery
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

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
}


void ustawKamereKlawisze(GLint uniView, float deltaTime) 
{
    float cameraSpeed = 0.5f * deltaTime;

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
    GLenum prymityw = GL_TRIANGLE_FAN;

    // Okno renderingu
    sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);
    window.setFramerateLimit(360);
    window.setMouseCursorGrabbed(true);
    window.setMouseCursorVisible(false);
    //window.setKeyRepeatEnabled(false);


    // Inicjalizacja GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    // W³¹czenie z-bufora
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Utworzenie VAO i VBO dla wielok¹ta i szeœcianu (Vertex Array/Buffer Object)
    GLuint vaoCube, vboCube;
    glGenVertexArrays(1, &vaoCube);
    glGenBuffers(1, &vboCube);

    unsigned int texture1;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    // Parametry tekstury
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("icecube.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // Wierzcho³ki dla szeœcianu
    float verticesCube[] =
    {
    // x     y      z     nx    ny    nz    u     v 
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,

     0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
     0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,

    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f
    };

    // Przesy³anie wierzcho³ków szeœcianu do VBO
    glBindVertexArray(vaoCube);
    glBindBuffer(GL_ARRAY_BUFFER, vboCube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCube), verticesCube, GL_STATIC_DRAW);

    // Kompilacja shaderów
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

    // Pobieranie atrybutów dla pozycji i koloru
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");

    // Atrybut normalnych
    GLint NorAttrib = glGetAttribLocation(shaderProgram, "aNormal");
    glEnableVertexAttribArray(NorAttrib);
    glVertexAttribPointer(NorAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    // Pozycja œwiat³a
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    GLint uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
    glUniform3fv(uniLightPos, 1, glm::value_ptr(lightPos));
    glUniform1i(glGetUniformLocation(shaderProgram, "lightingType"), 0);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"), -0.2f, -1.0f, -0.3f);

    glUniform1i(glGetUniformLocation(shaderProgram, "lightingType"), 1);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 1.2f, 1.0f, 2.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "constant"), 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "linear"), 0.09f);
    glUniform1f(glGetUniformLocation(shaderProgram, "quadratic"), 0.032f);

    glUniform1i(glGetUniformLocation(shaderProgram, "lightingType"), 2);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), cameraPos.x, cameraPos.y, cameraPos.z); // Reflektor w kamerze
    glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"), cameraFront.x, cameraFront.y, cameraFront.z);
    glUniform1f(glGetUniformLocation(shaderProgram, "cutoff"), 12.5f);
    glUniform1f(glGetUniformLocation(shaderProgram, "outerCutoff"), 15.0f);


    float cameraSpeed = 0.05f;
    float obrot = 0.0f;

    // Macierz projekcji
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    bool running = true;
    points = 8;
    prymityw = GL_TRIANGLE_FAN;
    oldPosY = 0;

    bool lightingEnabled = true;
    int lightingType = 0;        // 0: Ambient, 1: Diffuse
    float ambientStrength = 0.1f;
    float diffuseStrength = 1.0f;

    GLint uniView = glGetUniformLocation(shaderProgram, "view");

    sf::Clock clock;
    float deltaTime; // przechowuje czas w sekundach jaki up³yn¹³ od ostatniego odœwie¿enia klatki

    GLint uniLightingType = glGetUniformLocation(shaderProgram, "lightingType");
    glUniform1i(uniLightingType, lightingType);

    GLint uniAmbientStrength = glGetUniformLocation(shaderProgram, "ambientStrength");
    glUniform1f(uniAmbientStrength, ambientStrength);

    GLint uniDiffuseStrength = glGetUniformLocation(shaderProgram, "diffuseStrength");
    glUniform1f(uniDiffuseStrength, diffuseStrength);

    GLint uniLightingEnabled = glGetUniformLocation(shaderProgram, "lightingEnabled");


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

                // W³¹czanie/wy³¹czanie œwiat³a
                if (windowEvent.key.code == sf::Keyboard::Space) 
                {
                    lightingEnabled = !lightingEnabled;
                    glUniform1i(uniLightingEnabled, lightingEnabled);
                    std::cout << (lightingEnabled ? "LIGHT ENABLED\n" : "LIGHT DISABLED\n");
                }
                // Zmiana mocy œwiat³a otoczenia
                else if (windowEvent.key.code == sf::Keyboard::Up) 
                {
                    ambientStrength = std::min(ambientStrength + 0.1f, 5.0f);
                    std::cout << "AMBIENT STRENGTH: " << ambientStrength << "\n";
                    GLint uniAmbientStrength = glGetUniformLocation(shaderProgram, "ambientStrength");
                    glUniform1f(uniAmbientStrength, ambientStrength);
                }
                else if (windowEvent.key.code == sf::Keyboard::Down) 
                {
                    ambientStrength = std::max(ambientStrength - 0.1f, 0.0f);
                    std::cout << "AMBIENT STRENGTH: " << ambientStrength << "\n";
                    GLint uniAmbientStrength = glGetUniformLocation(shaderProgram, "ambientStrength");
                    glUniform1f(uniAmbientStrength, ambientStrength);
                }
                // Zmiana mocy œwiat³a kierunkowego
                else if (windowEvent.key.code == sf::Keyboard::Right) 
                {
                    std::cout << "DIFFUSE STRENGTH: " << diffuseStrength << "\n";
                    diffuseStrength = std::min(diffuseStrength + 0.1f, 5.0f);
                    GLint uniDiffuseStrength = glGetUniformLocation(shaderProgram, "diffuseStrength");
                    glUniform1f(uniDiffuseStrength, diffuseStrength);
                }
                else if (windowEvent.key.code == sf::Keyboard::Left)
                {
                    std::cout << "DIFFUSE STRENGTH: " << diffuseStrength << "\n";
                    diffuseStrength = std::max(diffuseStrength - 0.1f, 0.0f);
                    GLint uniDiffuseStrength = glGetUniformLocation(shaderProgram, "diffuseStrength");
                    glUniform1f(uniDiffuseStrength, diffuseStrength);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) 
                {
                    std::cout << "LIGHTING TYPE: DIRECTIONAL\n";
                    glUniform1i(glGetUniformLocation(shaderProgram, "lightingType"), 0); // Directional
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) 
                {
                    std::cout << "LIGHTING TYPE: POINT\n";
                    glUniform1i(glGetUniformLocation(shaderProgram, "lightingType"), 1); // Point
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) 
                {
                    std::cout << "LIGHTING TYPE: SPOTLIGHT\n";
                    glUniform1i(glGetUniformLocation(shaderProgram, "lightingType"), 2); // Spotlight
                }

            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Macierz widoku
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint uniView = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

        ustawKamereMysz(uniView, deltaTime, window); // Ustawienie widoku kamery na podstawie ruchu myszy
        ustawKamereKlawisze(uniView, deltaTime);     // Obs³uga klawiszy do poruszania siê

            // Macierz modelu                                                              x     y     z
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(obrot), glm::vec3(0.0f, 1.0f, 0.0f));
            
            // Wys³anie do shadera
            GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(vaoCube);

            // W³¹czenie atrybutu pozycji wierzcho³ka
            glEnableVertexAttribArray(posAttrib);
            glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);

            // W³¹czenie atrybutu koloru
            glEnableVertexAttribArray(colAttrib);
            glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

            // W³¹czenie wspó³rzêdnych tekstury
            GLint texAttrib = glGetAttribLocation(shaderProgram, "aTexCoord");
            glEnableVertexAttribArray(texAttrib);
            glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

            // Renderowanie szeœcianu jako zbiór trójk¹tów
            glBindTexture(GL_TEXTURE_2D, texture1);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        window.display();
    }

    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteBuffers(1, &vboCube);
    glDeleteVertexArrays(1, &vaoCube);
    window.close();
    return 0;
}