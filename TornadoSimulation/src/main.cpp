#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#pragma region Constants
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;
const int INFO_LOG_BUFFER_SIZE = 512;
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
	int success;
	char infoLog[INFO_LOG_BUFFER_SIZE];

#pragma region Initialization
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
#pragma endregion

#pragma region Create Mesh
	float vertices[] = {
		 1.0f,  1.0f, 0.0f,  // top right
		 1.0f, -1.0f, 0.0f,  // bottom right
		-1.0f, -1.0f, 0.0f,  // bottom left
		-1.0f,  1.0f, 0.0f   // top left 
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

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
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

#pragma region Simulation Parameter
	float backgroundColor[] = { 0.0f, 0.0f, 0.0f };
#pragma endregion

#pragma region Main Loop
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
			ImGui::Text("Test test");
			ImGui::ColorPicker3("backgroundColor", backgroundColor);
			ImGui::End();
		}
#pragma endregion

		glfwPollEvents();

#pragma region Clear Screen
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
#pragma endregion

#pragma region Update Uniforms
		glUseProgram(program);
		GLint backgroundColorLocation = glGetUniformLocation(program, "backgroundColor");
		glUniform3f(backgroundColorLocation, backgroundColor[0], backgroundColor[1], backgroundColor[2]);
#pragma endregion

#pragma region Render Mesh
		glBindVertexArray(VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#pragma endregion

#pragma region Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion

		// swap buffer
		glfwSwapBuffers(window);
	}
#pragma endregion

#pragma region Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(program);
	glfwTerminate();
#pragma endregion

	return EXIT_SUCCESS;
}
#pragma endregion
