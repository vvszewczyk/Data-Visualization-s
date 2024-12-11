#pragma once
#include <GL/glew.h>

const GLchar* vertexSource = R"glsl(
    #version 150 core
    in vec3 position;
    in vec3 normal;
    in vec2 texCoord; // Dodane UV

    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoord; // Przekazywanie UV do fragment shader

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;

    void main()
    {
        FragPos = vec3(model * vec4(position, 1.0));
        Normal = mat3(transpose(inverse(model))) * normal; // Poprawna transformacja normalnych
        TexCoord = texCoord; // Przekazanie UV
        gl_Position = proj * view * model * vec4(position, 1.0); 
    }
    )glsl";

const GLchar* fragmentSource = R"glsl(
    #version 150 core
    out vec4 outColor;

    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoord;

    uniform sampler2D texture1; // Sampler tekstury
    uniform vec4 objectColor;    // Kolor obiektu
    uniform bool useTexture;     // Flaga u¿ycia tekstury

    void main()
    {
        if (useTexture)
        {
            outColor = texture(texture1, TexCoord);
        }
        else
        {
            outColor = objectColor;
        }
    }
    )glsl";
