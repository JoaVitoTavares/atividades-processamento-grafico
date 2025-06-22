/*
 * Hello Transform - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle 
 *
 * Adaptado por: Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização: 13/08/2024
 *
 */

 #include <iostream>
 #include <string>
 #include <assert.h>
 
 using namespace std;
 
 // GLAD
 #include <glad/glad.h>
 
 // GLFW
 #include <GLFW/glfw3.h>
 
 // GLM (header-only)
 #include <glm/glm.hpp> 
 #include <glm/gtc/matrix_transform.hpp>
 #include <glm/gtc/type_ptr.hpp>
 
 using namespace glm;
 
 // Protótipo da função de callback de teclado
 void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
 
 // Protótipos das funções
 int setupShader();
 GLuint setupGeometry(); // Função que cria um VBO (sem VAO)
 
 const GLuint WIDTH = 800, HEIGHT = 600;
 
 // Shaders adaptados para GLSL 120 (compatível com OpenGL 2.1)
 const GLchar* vertexShaderSource = "#version 120\n"
 "attribute vec3 position;\n"
 "uniform mat4 projection;\n"
 "uniform mat4 model;\n"
 "void main()\n"
 "{\n"
 "    gl_Position = projection * model * vec4(position, 1.0);\n"
 "}\0";
 
 const GLchar* fragmentShaderSource = "#version 120\n"
 "uniform vec4 inputColor;\n"
 "void main()\n"
 "{\n"
 "    gl_FragColor = inputColor;\n"
 "}\n\0";
 
 int main()
 {
	 // Inicializa GLFW
	 if (!glfwInit())
	 {
		 cerr << "Erro ao inicializar o GLFW" << endl;
		 return -1;
	 }
 
	 // Para OpenGL 2.1 não precisamos definir hints de versão; usamos o contexto padrão
	 GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Triangulo! -- Rossana", nullptr, nullptr);
	 if (!window)
	 {
		 cerr << "Falha ao criar a janela GLFW" << endl;
		 glfwTerminate();
		 return -1;
	 }
	 glfwMakeContextCurrent(window);
	 glfwSetKeyCallback(window, key_callback);
 
	 // Inicializa o GLAD
	 if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	 {
		 cerr << "Failed to initialize GLAD" << endl;
		 return -1;
	 }
 
	 const GLubyte* renderer = glGetString(GL_RENDERER);
	 const GLubyte* version = glGetString(GL_VERSION);
	 cout << "Renderer: " << renderer << endl;
	 cout << "OpenGL version supported " << version << endl;
 
	 int fbWidth, fbHeight;
	 glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	 glViewport(0, 0, fbWidth, fbHeight);
 
	 // Compila e linka os shaders
	 GLuint shaderID = setupShader();
	 glUseProgram(shaderID);
 
	 // Configura a matriz de projeção (ortográfica)
	 mat4 projection = ortho(0.0, 800.0, 0.0, 600.0, -1.0, 1.0);
	 GLint projLoc = glGetUniformLocation(shaderID, "projection");
	 glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));
 
	 // Inicializa a matriz de modelo (vai ser atualizada dinamicamente)
	 GLint modelLoc = glGetUniformLocation(shaderID, "model");
	 mat4 model = mat4(1.0f);
	 glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
 
	 // Cria o VBO com a geometria do triângulo
	 GLuint vbo = setupGeometry();
 
	 // Localização da variável uniform "inputColor"
	 GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
 
	 // Loop principal
	 while (!glfwWindowShouldClose(window))
	 {
		 glfwPollEvents();
 
		 // Atualiza a matriz de modelo: translação, rotação e escala dinamicamente
		 model = mat4(1.0f);
		 model = translate(model, vec3(400.0f, 300.0f, 0.0f));
		 model = rotate(model, (float)glfwGetTime(), vec3(0.0f, 0.0f, 1.0f));
		 model = scale(model, vec3(abs(cos(glfwGetTime())) * 300.0f, abs(cos(glfwGetTime())) * 300.0f, 1.0f));
		 glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
 
		 glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		 glClear(GL_COLOR_BUFFER_BIT);
 
		 glLineWidth(10);
		 glPointSize(20);
 
		 // Como não usamos VAO, precisamos configurar o VBO e os vertex attribute pointers a cada frame
		 glBindBuffer(GL_ARRAY_BUFFER, vbo);
		 glEnableVertexAttribArray(0);
		 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
 
		 // Envia a cor (varia com o tempo)
		 glUniform4f(colorLoc, 0.0f, 0.0f, abs(cos(glfwGetTime())), 1.0f);
 
		 // Desenha o triângulo
		 glDrawArrays(GL_TRIANGLES, 0, 3);
 
		 glDisableVertexAttribArray(0);
		 glBindBuffer(GL_ARRAY_BUFFER, 0);
 
		 glfwSwapBuffers(window);
	 }
 
	 // Libera recursos
	 glDeleteBuffers(1, &vbo);
	 glfwTerminate();
	 return 0;
 }
 
 // Callback de teclado para fechar a janela ao pressionar ESC
 void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
 {
	 if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		 glfwSetWindowShouldClose(window, GL_TRUE);
 }
 
 // Função que compila os shaders e cria o programa
 int setupShader()
 {
	 GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	 glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	 glCompileShader(vertexShader);
	 GLint success;
	 GLchar infoLog[512];
	 glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	 if (!success)
	 {
		 glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		 cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
	 }
 
	 GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	 glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	 glCompileShader(fragmentShader);
	 glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	 if (!success)
	 {
		 glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		 cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
	 }
 
	 GLuint shaderProgram = glCreateProgram();
	 glAttachShader(shaderProgram, vertexShader);
	 glAttachShader(shaderProgram, fragmentShader);
	 // Liga manualmente o atributo "position" à localização 0
	 glBindAttribLocation(shaderProgram, 0, "position");
	 glLinkProgram(shaderProgram);
	 glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	 if (!success)
	 {
		 glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		 cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
	 }
	 glDeleteShader(vertexShader);
	 glDeleteShader(fragmentShader);
 
	 return shaderProgram;
 }
 
 // Função que cria um VBO com a geometria de um triângulo
 GLuint setupGeometry()
 {
	 GLfloat vertices[] = {
		 // x, y, z
		 -0.5f, -0.5f, 0.0f,  // v0
		  0.5f, -0.5f, 0.0f,  // v1
		  0.0f,  0.5f, 0.0f   // v2
	 };
 
	 GLuint VBO;
	 glGenBuffers(1, &VBO);
	 glBindBuffer(GL_ARRAY_BUFFER, VBO);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
 
	 return VBO;
 }
 