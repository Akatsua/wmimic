#pragma once

#include <gl/glew.h>
#include <stdio.h>

GLuint LoadShader(const char * shaderCode, GLenum shaderType)
{
    GLuint shader = glCreateShader(shaderType);

    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) 
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);

        fprintf(stderr, "Error: %s\n", infoLog);

        return 0;
    }

    return shader;
}

GLuint LoadProgram(const char* vertexCode, const char* fragmentCode)
{
    GLuint program = glCreateProgram();

    if (program == 0)
    {
        return 0;
    }

    GLuint vertexShader = LoadShader(vertexCode, GL_VERTEX_SHADER);
    GLuint fragmentShader = LoadShader(fragmentCode, GL_FRAGMENT_SHADER);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    GLint success = 0;
    GLchar infoLog[1024] = { 0 };

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == 0) 
    {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);

        fprintf(stderr, "Error: %s\n", infoLog);

        return 0;
    }

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);

    if (!success) 
    {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);

        fprintf(stderr, "Error: %s\n", infoLog);

        return 0;
    }

    return program;
}