#pragma comment (lib, "opengl32.lib")

#include <windows.h>

#include <GL/glew.h>
#include <GL/GL.h>
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ogl.h"
#include "workerw.h"

#define ulong unsigned long
#define uint unsigned int

int main(int argc, char* argv[])
{
#if !_DEBUG 
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#else
	ShowWindow(GetConsoleWindow(), SW_NORMAL);
#endif
	
	// Input
	ulong deviceNumber = 0;		// Improve: Configurable
	
	// State
	ulong resolutionX = 0;
	ulong resolutionY = 0;
	HWND  wallpaperHandle  = nullptr;
	HDC   wallpaperContext = nullptr;

	// Get display resolution
	{
		DISPLAY_DEVICEA device = { 0 };
		DEVMODEA        devmode = { 0 };

		device.cb = sizeof(device);

		int success = EnumDisplayDevicesA(NULL, deviceNumber, &device, 0);

		if (success == 0)
		{
			return 1;
		}

		success = EnumDisplaySettingsA(device.DeviceName, ENUM_CURRENT_SETTINGS, &devmode);

		if (success == 0)
		{
			return 1;
		}

		resolutionX = devmode.dmPelsWidth;
		resolutionY = devmode.dmPelsHeight;
	}

	// Spawn window between wallpaper and icons
	{
		wallpaperHandle  = SpawnWorkerW();
		wallpaperContext = GetDCEx(wallpaperHandle, nullptr, DCX_WINDOW | DCX_LOCKWINDOWUPDATE);

		MoveWindow(wallpaperHandle, 0, 0, 3440, 1440, 1);
		ShowWindow(wallpaperHandle, SW_NORMAL);
	}

	// Set pixel format ready for OpenGL
	{
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    
			PFD_TYPE_RGBA,            
			32,                       
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                       
			8,                        
			0,                        
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int pixelFormat = ChoosePixelFormat(wallpaperContext, &pfd);
		SetPixelFormat(wallpaperContext, pixelFormat, &pfd);
	}

	// Create OpenGL context & initialize 
	{
		HGLRC context = wglCreateContext(wallpaperContext);
		wglMakeCurrent(wallpaperContext, context);

		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
			return 1;
		}
	}

	GLuint program = LoadProgram(
		R"(#version 330

layout (location = 0) in vec3 Position;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(Position, 1.0);
})", 
		R"(#version 330

uniform float Frame;
uniform unsigned int ResX;

out vec4 FragColor;

// https://web.archive.org/web/20200808153903/http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// https://shader-tutorial.dev/advanced/color-banding-dithering/
const float NOISE_GRANULARITY = 0.5/255.0;

float random(vec2 coords) {
   return fract(sin(dot(coords.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
	vec4 pos = gl_FragCoord;
	
	float ratio = pos.x / ResX;
    float hueU = (ratio * 0.2f) + (Frame / 360.0f);
	float hue = hueU - floor(hueU);

	vec3 currentColor = hsv2rgb(vec3(hue, 0.5f, 0.85f));
	
	vec4 fragmentColor = vec4(currentColor.r, currentColor.g, currentColor.b, 1.0f);

	fragmentColor += mix(-NOISE_GRANULARITY, NOISE_GRANULARITY, random(vec2(pos.x, pos.y)));
	
	FragColor = fragmentColor;
}
)");

	glm::vec3 pos	 {  0.0f , 0.0f , 3.0f };
	glm::vec3 center {  0.0f , 0.0f	, 0.0f };
	glm::vec3 up	 {  0.0f , 1.0f	, 0.0f };

	glm::mat4 ortho = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	glm::mat4 look  = glm::lookAt(pos, center, up);
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 mvp   = ortho * look * model;

	float vertices[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,

		-1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f
	};

	GLfloat frame = 0;	// Improve: Frame count and frametime uniform

	GLuint uniformMVP   = glGetUniformLocation(program, "MVP");
	GLuint uniformFrame = glGetUniformLocation(program, "Frame");
	GLuint uniformResX  = glGetUniformLocation(program, "ResX");

	GLuint VBO;
	GLuint VAO;

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);

	while (true)
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glUniformMatrix4fv(uniformMVP, 1, GL_FALSE, &mvp[0][0]);
		glUniform1f(uniformFrame, frame);
		glUniform1ui(uniformResX, resolutionX);

		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3 * 2);

		frame = frame > 360 ? 0 : frame + 1;

		Sleep(33); // Improve: Frametime / wait

		SwapBuffers(wallpaperContext);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(program);
}
