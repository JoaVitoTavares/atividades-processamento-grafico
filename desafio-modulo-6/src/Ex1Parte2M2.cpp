/*
 * Hello Triangle - Adaptado para OpenGL 2.1
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 07/04/2017
 * Última atualização: 13/08/2024
 *
 */

 #include <iostream>
 #include <string>
 #include <vector>
 #include <cmath>
 #include <assert.h>
 
 // GLAD
 #include <glad/glad.h>
 
 // GLFW
 #include <GLFW/glfw3.h>
 
 // GLM (para matrizes e transformações)
 #include <glm/glm.hpp>
 #include <glm/gtc/matrix_transform.hpp>
 #include <glm/gtc/type_ptr.hpp>
 
 using namespace std;
 using namespace glm;
 
 // Estrutura que define um triângulo (posição, dimensões e cor)
 struct Triangle {
	 vec3 position;
	 vec3 dimensions;
	 vec3 color;
 };
 
 vector<Triangle> triangles;
 vector<vec3> colors;
 int iColor = 0;
 
 // Protótipos dos callbacks
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
 void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
 
 // Protótipos das funções de setup
 // Nesta versão, não usamos VAOs, apenas VBOs.
 GLuint createTriangleVBO(float x0, float y0, float x1, float y1, float x2, float y2);
 int setupShader();
 
 // Dimensões da janela
 const GLuint WIDTH = 800, HEIGHT = 600;
 
 // Shaders atualizados para GLSL 120 (compatível com OpenGL 2.1)
 // No vertex shader, usamos "attribute" para o vértice; no fragment shader, usamos "gl_FragColor".
 const GLchar *vertexShaderSource =
 "#version 120\n"
 "attribute vec3 position;\n"
 "uniform mat4 projection;\n"
 "uniform mat4 model;\n"
 "void main()\n"
 "{\n"
 "   gl_Position = projection * model * vec4(position, 1.0);\n"
 "}\n";
 
 const GLchar *fragmentShaderSource =
 "#version 120\n"
 "uniform vec4 inputColor;\n"
 "void main()\n"
 "{\n"
 "   gl_FragColor = inputColor;\n"
 "}\n";
 
 int main()
 {
	 // Inicializa a GLFW (não definindo hints de versão para usar o contexto padrão OpenGL 2.1)
	 if (!glfwInit()) {
		 cerr << "Erro ao inicializar GLFW." << endl;
		 return -1;
	 }
	 
	 GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Triangulo! -- Rossana", nullptr, nullptr);
	 if (!window) {
		 cerr << "Falha ao criar a janela GLFW" << endl;
		 glfwTerminate();
		 return -1;
	 }
	 glfwMakeContextCurrent(window);
	 
	 // Registra callbacks de teclado e mouse
	 glfwSetKeyCallback(window, key_callback);
	 glfwSetMouseButtonCallback(window, mouse_button_callback);
	 
	 // Inicializa o GLAD para carregar as funções da OpenGL
	 if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		 cerr << "Failed to initialize GLAD" << endl;
		 return -1;
	 }
	 
	 // Exibe informações do renderer e da versão da OpenGL
	 const GLubyte *rendererStr = glGetString(GL_RENDERER);
	 const GLubyte *versionStr  = glGetString(GL_VERSION);
	 cout << "Renderer: " << rendererStr << endl;
	 cout << "OpenGL version supported: " << versionStr << endl;
	 
	 // Ajusta a viewport conforme o tamanho do framebuffer
	 int fbWidth, fbHeight;
	 glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	 glViewport(0, 0, fbWidth, fbHeight);
	 
	 // Inicializa a paleta de cores (valores originais normalizados para 0-1)
	 colors.push_back(vec3(200,191,231));
	 colors.push_back(vec3(174,217,224));
	 colors.push_back(vec3(181,234,215));
	 colors.push_back(vec3(255,241,182));
	 colors.push_back(vec3(255,188,188));
	 colors.push_back(vec3(246,193,199));
	 colors.push_back(vec3(255,216,190));
	 colors.push_back(vec3(220,198,224));
	 colors.push_back(vec3(208,230,165));
	 colors.push_back(vec3(183,201,226));
	 for (size_t i = 0; i < colors.size(); i++) {
		 colors[i].r /= 255.0;
		 colors[i].g /= 255.0;
		 colors[i].b /= 255.0;
	 }
	 
	 // Compila e linka os shaders
	 GLuint shaderID = setupShader();
	 glUseProgram(shaderID);
	 
	 // Cria um triângulo padrão e obtém seu VBO
	 GLuint triangleVBO = createTriangleVBO(-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f);
	 
	 // Cria o primeiro triângulo (objeto) e adiciona à lista
	 Triangle tri;
	 tri.position = vec3(400.0f, 300.0f, 0.0f);
	 tri.dimensions = vec3(100.0f, 100.0f, 1.0f);
	 tri.color = vec3(colors[iColor].r, colors[iColor].g, colors[iColor].b);
	 iColor = (iColor + 1) % colors.size();
	 triangles.push_back(tri);
	 
	 // Localiza a variável uniform "inputColor" no shader
	 GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
	 
	 // Configura a matriz de projeção ortográfica (origem no canto superior esquerdo)
	 mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
	 GLint projLoc = glGetUniformLocation(shaderID, "projection");
	 glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));
	 
	 // Loop principal (game loop)
	 while (!glfwWindowShouldClose(window))
	 {
		 glfwPollEvents();
		 
		 glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		 glClear(GL_COLOR_BUFFER_BIT);
		 
		 glLineWidth(10);
		 glPointSize(20);
		 
		 // Para cada triângulo armazenado, atualiza a matriz de modelo e a cor e desenha
		 for (size_t i = 0; i < triangles.size(); i++) {
			 mat4 model = mat4(1.0f);
			 model = translate(model, vec3(triangles[i].position.x, triangles[i].position.y, 0.0f));
			 model = rotate(model, radians(180.0f), vec3(0.0f, 0.0f, 1.0f));
			 model = scale(model, vec3(triangles[i].dimensions.x, triangles[i].dimensions.y, 1.0f));
			 GLint modelLoc = glGetUniformLocation(shaderID, "model");
			 glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
			 
			 glUniform4f(colorLoc, triangles[i].color.r, triangles[i].color.g, triangles[i].color.b, 1.0f);
			 
			 // Configura o VBO e o ponteiro do atributo antes de desenhar (sem VAO)
			 glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
			 glEnableVertexAttribArray(0);
			 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			 
			 glDrawArrays(GL_TRIANGLES, 0, 3);
			 
			 glDisableVertexAttribArray(0);
			 glBindBuffer(GL_ARRAY_BUFFER, 0);
		 }
		 
		 glfwSwapBuffers(window);
	 }
	 
	 // Libera o VBO e finaliza
	 glDeleteBuffers(1, &triangleVBO);
	 glfwTerminate();
	 return 0;
 }
 
 
 // Callback de teclado: fecha a janela quando ESC é pressionado
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
 {
	 if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		 glfwSetWindowShouldClose(window, GL_TRUE);
 }
 
 // Função que compila os shaders e cria o programa (usando GLSL 120 para OpenGL 2.1)
 int setupShader()
 {
	 // Vertex Shader
	 GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	 glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	 glCompileShader(vertexShader);
	 
	 GLint success;
	 GLchar infoLog[512];
	 glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	 if (!success) {
		 glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		 cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
	 }
	 
	 // Fragment Shader
	 GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	 glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	 glCompileShader(fragmentShader);
	 
	 glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	 if (!success) {
		 glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		 cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
	 }
	 
	 // Programa de Shader
	 GLuint shaderProgram = glCreateProgram();
	 glAttachShader(shaderProgram, vertexShader);
	 glAttachShader(shaderProgram, fragmentShader);
	 // Opcional: glBindAttribLocation(shaderProgram, 0, "position");
	 glLinkProgram(shaderProgram);
	 
	 glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	 if (!success) {
		 glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		 cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
	 }
	 
	 glDeleteShader(vertexShader);
	 glDeleteShader(fragmentShader);
	 
	 return shaderProgram;
 }
 
 // Função que cria um VBO com a geometria (vértices) de um triângulo (sem VAO)
 GLuint createTriangleVBO(float x0, float y0, float x1, float y1, float x2, float y2)
 {
	 GLfloat vertices[] = {
		 x0, y0, 0.0f,
		 x1, y1, 0.0f,
		 x2, y2, 0.0f
	 };
	 
	 GLuint VBO;
	 glGenBuffers(1, &VBO);
	 glBindBuffer(GL_ARRAY_BUFFER, VBO);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 
	 return VBO;
 }
 
 void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
 {
	 if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		 double xpos, ypos;
		 glfwGetCursorPos(window, &xpos, &ypos);
		 cout << "Clique em: " << xpos << " " << ypos << endl;
		 
		 Triangle tri;
		 tri.position = vec3(xpos, ypos, 0.0f);
		 tri.dimensions = vec3(100.0f, 100.0f, 1.0f);
		 tri.color = vec3(colors[iColor].r, colors[iColor].g, colors[iColor].b);
		 iColor = (iColor + 1) % colors.size();
		 triangles.push_back(tri);
	 }
 }
 