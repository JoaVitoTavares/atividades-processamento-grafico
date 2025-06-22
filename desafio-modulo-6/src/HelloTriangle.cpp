/*
 * Hello Triangle - Código adaptado de:
 *   - https://learnopengl.com/#!Getting-started/Hello-Triangle
 *   - https://antongerdelan.net/opengl/glcontext2.html
 *
 * Adaptado por: Rossana Baptista Queiroz
 *
 * Disciplinas:
 *   - Processamento Gráfico (Ciência da Computação - Híbrido)
 *   - Processamento Gráfico: Fundamentos (Ciência da Computação - Presencial)
 *   - Fundamentos de Computação Gráfica (Jogos Digitais)
 *
 * Descrição:
 *   Este código é o "Olá Mundo" da Computação Gráfica, utilizando OpenGL Moderna.
 *   No pipeline programável, o desenvolvedor pode implementar as etapas de
 *   Processamento de Geometria e Processamento de Pixel utilizando shaders.
 *   Um programa de shader precisa ter, obrigatoriamente, um Vertex Shader e um Fragment Shader,
 *   enquanto outros shaders, como o de geometria, são opcionais.
 *
 * Histórico:
 *   - Versão inicial: 07/04/2017
 *   - Última atualização: 18/03/2025
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
 
 // Protótipo da função de callback de teclado
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
 
 // Protótipos das funções
 int setupShader();
 int setupGeometry();
 
 // Dimensões da janela (pode ser alterado em tempo de execução)
 const GLuint WIDTH = 800, HEIGHT = 600;
 
 // Código fonte do Vertex Shader (em GLSL): ainda hardcoded
 const GLchar *vertexShaderSource = R"(
  #version 400
  layout (location = 0) in vec3 position;
  void main()
  {
	  gl_Position = vec4(position.x, position.y, position.z, 1.0);
  }
  )";
 
 // Código fonte do Fragment Shader (em GLSL): ainda hardcoded
 const GLchar *fragmentShaderSource = R"(
  #version 400
  uniform vec4 inputColor;
  out vec4 color;
  void main()
  {
	  color = inputColor;
  }
  )";
 
 // Função de callback para erros do GLFW (opcional, mas recomendado)
 void error_callback(int error, const char* description) {
	 fprintf(stderr, "Erro GLFW (%d): %s\n", error, description);
 }
 
 // Função MAIN
 int main()
 {
	 // Registra a callback de erros
	 glfwSetErrorCallback(error_callback);
 
	 // Inicialização da GLFW
	 if (!glfwInit()){
		 std::cerr << "Erro ao inicializar o GLFW" << std::endl;
		 return -1;
	 }
 
	 // Configuração dos Window Hints conforme o sistema operacional
	 #ifdef __APPLE__
		 // macOS suporta até OpenGL 4.1 (ou 3.3)
		 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		 glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	 #else
		 // Em outros sistemas, utilizar OpenGL 4.6 conforme desejado
		 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	 #endif
 
	 // Ativa a suavização de serrilhado (MSAA) com 8 amostras por pixel
	 glfwWindowHint(GLFW_SAMPLES, 8);
 
	 // Criação da janela GLFW
	 GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Triangulo! -- Rossana", nullptr, nullptr);
	 if (!window)
	 {
		 std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		 glfwTerminate();
		 return -1;
	 }
	 glfwMakeContextCurrent(window);
 
	 // Fazendo o registro da função de callback para a janela GLFW
	 glfwSetKeyCallback(window, key_callback);
 
	 // GLAD: carrega todos os ponteiros das funções da OpenGL
	 if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	 {
		 std::cerr << "Falha ao inicializar GLAD" << std::endl;
		 return -1;
	 }
 
	 // Obtendo as informações de versão
	 const GLubyte *renderer = glGetString(GL_RENDERER);
	 const GLubyte *version = glGetString(GL_VERSION);
	 cout << "Renderer: " << renderer << endl;
	 cout << "OpenGL version supported " << version << endl;
 
	 // Definindo as dimensões da viewport conforme o tamanho da janela
	 int width, height;
	 glfwGetFramebufferSize(window, &width, &height);
	 glViewport(0, 0, width, height);
 
	 // Compilando e buildando o programa de shader
	 GLuint shaderID = setupShader();
 
	 // Gerando um buffer simples, com a geometria de um triângulo
	 GLuint VAO = setupGeometry();
 
	 // Obtendo a localização da variável uniform "inputColor" no shader
	 GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
 
	 // Usa o shader corrente
	 glUseProgram(shaderID);
 
	 double prev_s = glfwGetTime();     // Tempo anterior para o cálculo do FPS.
	 double title_countdown_s = 0.1;     // Intervalo para atualizar o título com o FPS.
 
	 // Loop da aplicação - "game loop"
	 while (!glfwWindowShouldClose(window))
	 {
		 // Cálculo opcional do FPS para exibição no título
		 {
			 double curr_s = glfwGetTime();
			 double elapsed_s = curr_s - prev_s;
			 prev_s = curr_s;
			 title_countdown_s -= elapsed_s;
			 if (title_countdown_s <= 0.0 && elapsed_s > 0.0)
			 {
				 double fps = 1.0 / elapsed_s;
				 char tmp[256];
				 sprintf(tmp, "Ola Triangulo! -- Rossana\tFPS %.2lf", fps);
				 glfwSetWindowTitle(window, tmp);
				 title_countdown_s = 0.1;
			 }
		 }
 
		 // Processa os eventos de input
		 glfwPollEvents();
 
		 // Limpa o buffer de cor
		 glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		 glClear(GL_COLOR_BUFFER_BIT);
 
		 glLineWidth(10);
		 glPointSize(20);
 
		 // Conecta o VAO com a geometria do triângulo
		 glBindVertexArray(VAO);
 
		 // Envia a cor azul para o fragment shader
		 glUniform4f(colorLoc, 0.0f, 0.0f, 1.0f, 1.0f);
 
		 // Chamada de desenho - renderiza o triângulo
		 glDrawArrays(GL_TRIANGLES, 0, 3);
 
		 // Troca os buffers para exibir o novo frame
		 glfwSwapBuffers(window);
	 }
 
	 // Libera os recursos alocados
	 glDeleteVertexArrays(1, &VAO);
	 glfwTerminate();
	 return 0;
 }
 
 // Função de callback de teclado
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
 {
	 if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		 glfwSetWindowShouldClose(window, GL_TRUE);
 }
 
 // Configura e compila os shaders, retornando o ID do programa criado
 int setupShader()
 {
	 // Criação e compilação do Vertex Shader
	 GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	 glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	 glCompileShader(vertexShader);
 
	 GLint success;
	 GLchar infoLog[512];
	 glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	 if (!success)
	 {
		 glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		 std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	 }
 
	 // Criação e compilação do Fragment Shader
	 GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	 glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	 glCompileShader(fragmentShader);
	 glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	 if (!success)
	 {
		 glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		 std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	 }
 
	 // Linkagem dos shaders em um programa
	 GLuint shaderProgram = glCreateProgram();
	 glAttachShader(shaderProgram, vertexShader);
	 glAttachShader(shaderProgram, fragmentShader);
	 glLinkProgram(shaderProgram);
	 glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	 if (!success)
	 {
		 glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		 std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	 }
 
	 // Os shaders individuais já não são mais necessários após a linkagem
	 glDeleteShader(vertexShader);
	 glDeleteShader(fragmentShader);
 
	 return shaderProgram;
 }
 
 // Configura a geometria do triângulo e retorna o ID do VAO
 int setupGeometry()
 {
	 GLfloat vertices[] = {
		 // x      y      z
		 -0.5f, -0.5f, 0.0f, // v0
		  0.5f, -0.5f, 0.0f, // v1
		  0.0f,  0.5f, 0.0f  // v2
	 };
 
	 GLuint VBO, VAO;
	 glGenBuffers(1, &VBO);
	 glBindBuffer(GL_ARRAY_BUFFER, VBO);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
 
	 glGenVertexArrays(1, &VAO);
	 glBindVertexArray(VAO);
 
	 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	 glEnableVertexAttribArray(0);
 
	 // Desvincula o VBO e VAO para evitar efeitos indesejados
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 glBindVertexArray(0);
 
	 return VAO;
 }
 