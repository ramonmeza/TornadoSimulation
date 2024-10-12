#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <random>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#pragma region Constants
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;
const int INFO_LOG_BUFFER_SIZE = 512;
const int TEXTURE_WIDTH = 512;
const int TEXTURE_HEIGHT = 512;
const int NUM_PASSES = 1;
#pragma endregion

#pragma region Callbacks
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
#pragma endregion

#pragma region Entry Point
int main(int argc, char* argv[])
{
#pragma region Initialization
	srand(static_cast<unsigned int>(time(0)));

	int success;
	char infoLog[INFO_LOG_BUFFER_SIZE];

#pragma region GLFW Initialization
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return EXIT_FAILURE;
	}
#pragma endregion

#pragma region Window Creation
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tornado Simulation", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cerr << "Failed to create window." << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwSwapInterval(1);
	glfwMakeContextCurrent(window);

	// configure callbacks
	glfwSetKeyCallback(window, keyCallback);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
#pragma endregion

#pragma region GLAD Initialization
	if (!gladLoadGL(glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD." << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
#pragma endregion

	// create viewport
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

#pragma region ImGui Initialization
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
#pragma endregion

#pragma region Create Mesh
	float vertices[] = {
		// x, y, z           // u, v
		 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // top right
		 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // bottom right
		-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // bottom left
		-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  // top left
	};
	unsigned int indices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// x, y, z
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// u, v
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
#pragma endregion

#pragma region Create Framebuffers
	GLuint initFBO;
	GLuint initTexture;
	glGenFramebuffers(1, &initFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, initFBO);

	glGenTextures(1, &initTexture);
	glBindTexture(GL_TEXTURE_2D, initTexture);

	std::vector<float> initialData(TEXTURE_WIDTH * TEXTURE_HEIGHT * 4, 1.0f);
	for (unsigned int i = 0; i < initialData.size(); i += 4)
	{
		float value = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		initialData[i]     = value;  // R
		initialData[i + 1] = value;  // G
		initialData[i + 2] = value;  // B
		initialData[i + 3] = 1.0f;  // A
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, initialData.data());

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, initTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	unsigned int currentFBO = 0;
	GLuint FBO[2];
	glGenFramebuffers(2, FBO);

	GLuint FBOTexture[2];
	glGenTextures(2, FBOTexture);

	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO[i]);
		glBindTexture(GL_TEXTURE_2D, FBOTexture[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, initialData.data());

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOTexture[i], 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "Framebuffer " << i << " is not complete" << std::endl;

			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();

			glDeleteTextures(2, FBOTexture);
			glDeleteFramebuffers(2, FBO);
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
			glDeleteBuffers(1, &EBO);
			glfwTerminate();

			return EXIT_FAILURE;
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ImTextureID initTextureId = reinterpret_cast<ImTextureID>(initTexture);
	ImTextureID FBOTextureId0 = reinterpret_cast<ImTextureID>(FBOTexture[0]);
	ImTextureID FBOTextureId1 = reinterpret_cast<ImTextureID>(FBOTexture[1]);
#pragma endregion

#pragma region Create Shaders
	const char* vertexShaderFilePath = "shaders/default.vert";
	const char* fragmentShaderFilePath = "shaders/fluid.frag";
	std::string vertexShaderCode;
	std::string fragmentShaderCode;
	std::ifstream vertexShaderFile;
	std::ifstream fragmentShaderFile;

	vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		vertexShaderFile.open(vertexShaderFilePath);
		fragmentShaderFile.open(fragmentShaderFilePath);

		std::stringstream vss, fss;
		vss << vertexShaderFile.rdbuf();
		fss << fragmentShaderFile.rdbuf();

		vertexShaderFile.close();
		fragmentShaderFile.close();

		vertexShaderCode = vss.str();
		fragmentShaderCode = fss.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cerr << "Failed to load shaders from files\n" << vertexShaderFilePath << "\n" << fragmentShaderFilePath << std::endl;

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	const char* vertexShaderSource = vertexShaderCode.c_str();
	const char* fragmentShaderSource = fragmentShaderCode.c_str();

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, INFO_LOG_BUFFER_SIZE, nullptr, infoLog);
		std::cerr << "Failed to compile vertex shader\n" << infoLog << std::endl;
		glDeleteShader(vertexShader);
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, INFO_LOG_BUFFER_SIZE, nullptr, infoLog);
		std::cerr << "Failed to compile fragment shader\n" << infoLog << std::endl;
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	// create shader program
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, INFO_LOG_BUFFER_SIZE, nullptr, infoLog);
		std::cerr << "Failed to initialize link shader program\n" << infoLog << std::endl;
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
		glDeleteProgram(program);
		glfwTerminate();
		return EXIT_FAILURE;
	}
#pragma endregion
#pragma endregion

#pragma region Simulation Parameter
#pragma endregion

#pragma region Main Loop
	double prevTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
#pragma region Create new ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
#pragma endregion

#pragma region ImGui Editor
		if (!ImGui::Begin("Simulation Parameters"))
		{
			ImGui::End();
		}
		else
		{
			if (ImGui::CollapsingHeader("Textures"))
			{
				ImGui::Text("Initial Texture");
				ImGui::Image(initTextureId, ImVec2(TEXTURE_WIDTH * 0.5f, TEXTURE_HEIGHT * 0.5f));
			}

			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Framebuffers"))
			{
				ImGui::Text("Framebuffer 0");
				ImGui::Image(FBOTextureId0, ImVec2(TEXTURE_WIDTH * 0.5f, TEXTURE_HEIGHT * 0.5f));

				ImGui::Spacing();

				ImGui::Text("Framebuffer 1");
				ImGui::Image(FBOTextureId1, ImVec2(TEXTURE_WIDTH * 0.5f, TEXTURE_HEIGHT * 0.5f));
			}


			ImGui::End();
		}
#pragma endregion

		glfwPollEvents();

#pragma region Calculate deltaTime
		double currTime = glfwGetTime();
		float deltaTime = static_cast<float>(currTime - prevTime) * 1000.0f;
		prevTime = currTime;
#pragma endregion

#pragma region Update Uniforms
		glUseProgram(program);
		glUniform1f(glGetUniformLocation(program, "deltaTime"), deltaTime);
#pragma endregion

#pragma region Ping-Pong Rendering
		for (unsigned int i = 0; i < NUM_PASSES; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FBO[currentFBO]);
			glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(program);

			// this sets other FBO as the texture used as input to the simulation's previous state
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, FBO[1 - currentFBO]);
			glUniform1i(glGetUniformLocation(program, "previousStateTexture"), 0);

			// draw to this framebuffer
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			currentFBO = 1 - currentFBO;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Clear Screen
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
#pragma endregion

#pragma region Render Simulation
		glUseProgram(program);
		glBindTexture(GL_TEXTURE_2D, FBOTexture[1 - currentFBO]);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#pragma endregion

#pragma region Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion

		// swap buffer
		glfwSwapBuffers(window);

#pragma region Handle Errors
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cerr << "OpenGL error: " << err << std::endl;
		}
#pragma endregion
	}
#pragma endregion

#pragma region Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteTextures(1, &initTexture);
	glDeleteFramebuffers(1, &initFBO);
	glDeleteTextures(2, FBOTexture);
	glDeleteFramebuffers(2, FBO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(program);
	glfwTerminate();
#pragma endregion

	return EXIT_SUCCESS;
}
#pragma endregion
