#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <fstream>
#include <vector> // Para armazenar os dados do mapa dinamicamente se necessário, mas para este caso, o array fixo funciona

typedef unsigned int uint;
const uint SCR_W = 800, SCR_H = 600;
const int MAP_H = 15;
const int MAP_W = 15;
int mapData[MAP_H][MAP_W];

const int PINK_TILE_INDEX = 6; // Ainda pode ser útil para debug ou outros propósitos
const int IMPASSABLE_TILE_DEEP_WATER = 5;

// ===========================================
// Variáveis Globais do Jogador
// ===========================================
// Posição do jogador no grid do mapa
int playerGridX = MAP_W / 2;
int playerGridY = MAP_H / 2;

// Velocidade de movimento do jogador (em tiles por segundo ou pixels por frame, ajuste conforme necessário)
// Para movimento baseado em grid, isso pode ser menos relevante se o movimento for instantâneo de tile para tile.
// Se quiser um movimento suave entre tiles, precisaria de interpolação.
float playerMoveSpeed = 1.0f; // Exemplo: 1 tile por "passo"

// Estado da animação
int playerAnimationFrameX = 0; // Coluna atual na spritesheet
int playerAnimationFrameY = 0; // Linha atual na spritesheet (para diferentes direções, por exemplo)
double lastFrameTime = 0.0; // Tempo do último frame para controle da animação

// Variáveis para controle de tempo para animação do jogador
double g_lastFrameTime = 0.0;
float g_animationSpeed = 0.15f; // Segundos por quadro (quanto menor, mais rápida a animação)
int g_currentAnimationFrame = 0; // Índice do quadro atual na animação de corrida (0 a 6 para o vampiro)
int g_animationDirection = 0; // 0: para baixo, 1: para a esquerda, 2: para a direita, 3: para cima (ajustar conforme sua spritesheet)

// A spritesheet do Vampiro que você forneceu tem 7 frames por linha para a corrida
const int PLAYER_SPRITE_RUN_FRAMES = 7;
const int PLAYER_SPRITE_ROWS = 4; // Ajuste se houver mais linhas para diferentes direções

// ===========================================
// Controle de Entrada
// ===========================================
bool g_keysPressed[GLFW_KEY_LAST + 1] = {false};
bool g_keysHandled[GLFW_KEY_LAST + 1] = {false}; // Para evitar múltiplos movimentos por um único pressionar de tecla

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key >= 0 && key <= GLFW_KEY_LAST)
    {
        if (action == GLFW_PRESS)
        {
            g_keysPressed[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            g_keysPressed[key] = false;
            g_keysHandled[key] = false; // Resetar para permitir novo movimento ao soltar e pressionar novamente
        }
    }
}

// ===========================================
// Carregamento de Texturas e Inicialização de Geometria
// ===========================================
GLuint loadTexture(const char *path)
{
    stbi_set_flip_vertically_on_load(true); // Imagens OpenGL são geralmente carregadas de baixo para cima
    int w, h, n;
    unsigned char *data = stbi_load(path, &w, &h, &n, 0);
    if (!data)
    {
        std::cerr << "Failed to load " << path << "\n";
        return 0;
    }
    GLenum fmt = (n == 3 ? GL_RGB : GL_RGBA); // Assume RGB se 3 canais, RGBA se 4 canais
    if (n == 4) fmt = GL_RGBA; // Explicitamente definir para RGBA se houver canal alfa
    else if (n == 3) fmt = GL_RGB;
    else {
        std::cerr << "Formato de imagem nao suportado (canais: " << n << ") para " << path << "\n";
        stbi_image_free(data);
        return 0;
    }

    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return t;
}

GLuint quadVAO;
void initQuad()
{
    float V[] = {
            // Posição (x,y), Coordenadas de Textura (u,v)
            -0.5f,  0.5f,  0.0f, 1.0f, // Top-left
            0.5f, -0.5f,  1.0f, 0.0f, // Bottom-right
            -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left

            -0.5f,  0.5f,  0.0f, 1.0f, // Top-left
            0.5f,  0.5f,  1.0f, 1.0f, // Top-right
            0.5f, -0.5f,  1.0f, 0.0f  // Bottom-right
    };
    GLuint VBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(V), V, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
}

// ===========================================
// Shaders
// ===========================================
const char *vsSrc = R"glsl(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 projection;
uniform mat4 model;
uniform vec2 texScale;
uniform vec2 texOffset;
out vec2 UV;
void main(){
    UV = aUV * texScale + texOffset;
    gl_Position = projection * model * vec4(aPos,0,1);
}
)glsl";

const char *fsSrc = R"glsl(
#version 330 core
in vec2 UV;
out vec4 Frag;
uniform sampler2D spriteTex;
uniform bool u_outline;
uniform vec4 u_outlineColor;
void main(){
    // Para sprites com transparência, use discard para pixels totalmente transparentes
    vec4 texColor = texture(spriteTex, UV);
    if (texColor.a < 0.05) discard; // Descartar pixels quase transparentes (ajustado para 0.05 para maior robustez)

    if(u_outline) Frag = u_outlineColor;
    else          Frag = texColor;
}
)glsl";

static GLuint compileShader(GLenum t, const char *src)
{
    GLuint s = glCreateShader(t);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    char log[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << (t == GL_VERTEX_SHADER ? "VS" : "FS") << " error:\n"
                  << log;
    }
    return s;
}

static GLuint createProgram()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    char log[512];
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        glGetProgramInfoLog(p, 512, nullptr, log);
        std::cerr << "Link error:\n"
                  << log;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

GLuint outlineVAO;
void initOutline()
{
    float C[] = {
            -0.5f, 0.5f,
            0.5f, 0.5f,
            0.5f, -0.5f,
            -0.5f, -0.5f};
    GLuint vbo;
    glGenVertexArrays(1, &outlineVAO);
    glGenBuffers(1, &vbo);
    glBindVertexArray(outlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(C), C, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glBindVertexArray(0);
}

// ===========================================
// Carregamento do Mapa
// ===========================================
bool loadMapFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo do mapa: " << filename << std::endl;
        return false;
    }

    int fileMapH, fileMapW;
    file >> fileMapH >> fileMapW;

    if (fileMapH != MAP_H || fileMapW != MAP_W) {
        std::cerr << "Aviso: Dimensoes do mapa no arquivo (" << fileMapH << "x" << fileMapW
                  << ") nao correspondem as dimensoes esperadas (" << MAP_H << "x" << MAP_W << ")." << std::endl;
        // Se as dimensões não corresponderem, você pode ajustar MAP_H e MAP_W ou decidir sair.
        // Por enquanto, vamos usar as dimensões fixas e ler o máximo possível.
    }

    // Define a posição inicial do jogador no centro do mapa
    playerGridY = MAP_H / 2; // Linha (Y)
    playerGridX = MAP_W / 2; // Coluna (X)

    for (int i = 0; i < MAP_H; ++i) {
        for (int j = 0; j < MAP_W; ++j) {
            if (!(file >> mapData[i][j])) {
                std::cerr << "Erro: Falha ao ler o tile em [" << i << "][" << j << "] do arquivo." << std::endl;
                return false;
            }

            // Garante que a posição inicial do jogador não seja um tile intransitável
            if (i == playerGridY && j == playerGridX && mapData[i][j] == IMPASSABLE_TILE_DEEP_WATER) {
                std::cerr << "Erro: A agua profunda (tile " << IMPASSABLE_TILE_DEEP_WATER
                          << ") nao pode ficar no centro do mapa [" << i << "][" << j
                          << "], pois é onde o personagem da spawn." << std::endl;
                return false;
            }
        }
    }

    file.close();
    std::cout << "Mapa carregado com sucesso de: " << filename << std::endl;
    return true;
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *win = glfwCreateWindow(SCR_W, SCR_H, "IsometricTilemap - Player Control", nullptr, nullptr);
    if (!win)
    {
        std::cerr << "Failed to create GLFW window\n";
        return -1;
    }
    glfwMakeContextCurrent(win);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glfwSetKeyCallback(win, key_callback);

    // Tentar carregar o mapa. Se falhar, sair.
    if (!loadMapFromFile("map.txt")) {
        return -1;
    }

    glViewport(0, 0, SCR_W, SCR_H);
    GLuint shader = createProgram();
    glUseProgram(shader);

    // A projeção ortográfica será ajustada dinamicamente para seguir o jogador
    glm::mat4 proj = glm::ortho(0.0f, float(SCR_W), 0.0f, float(SCR_H), -1.0f, 1.0f);
    GLint locP = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(locP, 1, GL_FALSE, glm::value_ptr(proj));

    glUniform1i(glGetUniformLocation(shader, "spriteTex"), 0);

    initQuad();
    initOutline();

    GLuint tileset = loadTexture("resources/tileset.png"); // Textura dos tiles do mapa
    // Carregue a spritesheet do vampiro que você forneceu.
    // Certifique-se de que o caminho esteja correto em seu projeto.
    GLuint playerSpriteSheet = loadTexture("resources/Vampires2_Run_full.png"); // ALTERADO PARA full.png

    // Verificação de erro para o carregamento do sprite do jogador
    if (playerSpriteSheet == 0) {
        std::cerr << "Erro fatal: Nao foi possivel carregar a spritesheet do jogador (Vampires2_Run_full.png). Verifique o caminho e o ficheiro." << std::endl;
        glfwTerminate();
        return -1; // Sair do programa se a textura essencial não for carregada
    }

    // Dimensões de um único sprite na spritesheet do jogador
    // Baseado na imagem fornecida: Vampires2_Run_full.png
    // Esta spritesheet parece ter quadros de 64x64 pixels.
    const int playerSpriteSheetTotalW = 448; // 7 frames * 64px
    const int playerSpriteSheetTotalH = 256; // 4 rows * 64px

    // Largura e altura de um único sprite dentro da spritesheet
    const float playerSingleSpriteW = 64.0f;
    const float playerSingleSpriteH = 64.0f;

    // O tamanho do tile do mapa permanece o mesmo
    const float tileW = 128.0f, tileH = 64.0f;

    float halfW = tileW * 0.5f;
    float halfH = tileH * 0.5f;

    // Offset da origem do mapa para centralização ou ajuste visual
    glm::vec2 mapOriginOffset(0.0f, 0.0f);

    GLint locM = glGetUniformLocation(shader, "model");
    GLint locTS = glGetUniformLocation(shader, "texScale");
    GLint locTO = glGetUniformLocation(shader, "texOffset");
    GLint locOL = glGetUniformLocation(shader, "u_outline");
    GLint locCLR = glGetUniformLocation(shader, "u_outlineColor");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST); // Garante que sprites mais "altos" sejam desenhados sobre os mais "baixos"

    // Inicializa o tempo para a animação
    g_lastFrameTime = glfwGetTime();

    // Definição das dimensões do tileset do mapa
    const int nCols = 7; // Número de colunas no seu tileset.png
    const int nRows = 1; // Número de linhas no seu tileset.png

    while (!glfwWindowShouldClose(win))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - g_lastFrameTime;
        g_lastFrameTime = currentTime;

        glfwPollEvents();

        int newPlayerGridX = playerGridX;
        int newPlayerGridY = playerGridY;
        bool playerMoved = false; // Flag para saber se o jogador tentou se mover
        bool animating = false; // Flag para saber se a animação de corrida deve estar ativa

        // Lógica de Movimento e Animação
        // Note: Para movimento "por célula", você pode querer um pequeno atraso entre os movimentos,
        // ou usar g_keysHandled para que uma tecla só registre um movimento por pressionamento.
        // Se você quiser movimento suave, precisaria de interpolação e não apenas atualização de grid.
        if (g_keysPressed[GLFW_KEY_W] && !g_keysHandled[GLFW_KEY_W]) { // Mover para cima-direita no mapa isométrico (Nordeste)
            newPlayerGridY++;
            newPlayerGridX++;
            playerAnimationFrameY = 1; // Corresponde à linha Nordeste na spritesheet do vampiro
            playerMoved = true;
            g_keysHandled[GLFW_KEY_W] = true;
        }
        if (g_keysPressed[GLFW_KEY_S] && !g_keysHandled[GLFW_KEY_S]) { // Mover para baixo-esquerda no mapa isométrico (Sudoeste)
            newPlayerGridY--;
            newPlayerGridX--;
            playerAnimationFrameY = 2; // Corresponde à linha Sudoeste (frente) na spritesheet do vampiro
            playerMoved = true;
            g_keysHandled[GLFW_KEY_S] = true;
        }
        if (g_keysPressed[GLFW_KEY_A] && !g_keysHandled[GLFW_KEY_A]) { // Mover para baixo-direita no mapa isométrico (Sudeste)
            newPlayerGridY--;
            newPlayerGridX++;
            playerAnimationFrameY = 3; // Corresponde à linha Sudeste na spritesheet do vampiro
            playerMoved = true;
            g_keysHandled[GLFW_KEY_A] = true;
        }
        if (g_keysPressed[GLFW_KEY_D] && !g_keysHandled[GLFW_KEY_D]) { // Mover para cima-esquerda no mapa isométrico (Noroeste)
            newPlayerGridY++;
            newPlayerGridX--;
            playerAnimationFrameY = 0; // Corresponde à linha Noroeste (a sprite que você tinha antes) na spritesheet do vampiro
            playerMoved = true;
            g_keysHandled[GLFW_KEY_D] = true;
        }

        if (playerMoved) {
            animating = true;
            // Verificar limites do mapa e colisões
            if (newPlayerGridX >= 0 && newPlayerGridX < MAP_W &&
                newPlayerGridY >= 0 && newPlayerGridY < MAP_H)
            {
                if (mapData[newPlayerGridY][newPlayerGridX] != IMPASSABLE_TILE_DEEP_WATER)
                {
                    playerGridX = newPlayerGridX;
                    playerGridY = newPlayerGridY;
                }
            }
        }

        // Atualiza o frame da animação se o jogador estiver se movendo
        if (animating) {
            playerAnimationFrameX = (int)(currentTime / g_animationSpeed) % PLAYER_SPRITE_RUN_FRAMES;
        } else {
            // Se não estiver se movendo, usa o frame 0 da animação "de frente" (linha 2)
            playerAnimationFrameX = 0;
            playerAnimationFrameY = 2; // Linha Sudoeste para "idle" (frente para a câmera)
        }

        // ===========================================
        // Atualização da Câmera (Projeção) para seguir o jogador
        // ===========================================
        // Calcular a posição isométrica do jogador no mundo
        float playerWorldX = (playerGridY - playerGridX) * halfW + mapOriginOffset.x;
        float playerWorldY = (playerGridY + playerGridX) * halfH + mapOriginOffset.y;

        // Ajustar o offset da câmera para centralizar o jogador
        float cameraOffsetX = (SCR_W * 0.5f) - playerWorldX;
        float cameraOffsetY = (SCR_H * 0.5f) - playerWorldY;

        // Recriar a matriz de projeção com o offset da câmera
        proj = glm::ortho(0.0f - cameraOffsetX, float(SCR_W) - cameraOffsetX, 0.0f - cameraOffsetY, float(SCR_H) - cameraOffsetY, -1.0f, 1.0f);
        glUniformMatrix4fv(locP, 1, GL_FALSE, glm::value_ptr(proj));

        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(locOL, 0); // Desativa o contorno para tiles

        // ===========================================
        // Renderização dos Tiles do Mapa
        // ===========================================
        float dsx_tile = 1.0f / float(nCols);
        float dsy_tile = 1.0f / float(nRows);

        for (int i = 0; i < MAP_H; ++i)
        {
            for (int j = 0; j < MAP_W; ++j)
            {
                int idx = mapData[i][j];
                // Calcula as coordenadas de textura para o tile atual
                // Certifique-se de que `idx` é válido para as dimensões do seu tileset
                float offx_tile = (float)(idx % nCols) * dsx_tile;
                float offy_tile = (float)(idx / nCols) * dsy_tile;

                // Posição do tile no mundo isométrico
                float x = (i - j) * halfW + mapOriginOffset.x;
                float y = (i + j) * halfH + mapOriginOffset.y;

                // Aumenta o Z para que tiles mais "altos" (em termos de Y no mapa isométrico) sejam desenhados sobre os mais "baixos"
                // e para que o jogador seja desenhado sobre todos os tiles.
                // O valor Z para o tile pode ser baseado em sua posição (i+j) para ordenar corretamente.
                float tileZ = (i + j) * 0.001f; // Pequeno incremento para cada "linha" isométrica

                glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, tileZ)) * glm::scale(glm::mat4(1.0f), glm::vec3(tileW, tileH, 1));
                glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M));
                glUniform2f(locTS, dsx_tile, dsy_tile);
                glUniform2f(locTO, offx_tile, offy_tile); // Use offx_tile e offy_tile

                glBindTexture(GL_TEXTURE_2D, tileset);
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // ===========================================
        // Renderização do Personagem (Vampiro)
        // ===========================================
        // Posição isométrica do jogador
        float playerRenderX = (playerGridY - playerGridX) * halfW + mapOriginOffset.x;
        // Ajuste Y para que a base do sprite do jogador esteja no centro do tile ou levemente acima
        float playerRenderY = (playerGridY + playerGridX) * halfH + mapOriginOffset.y + (tileH * 0.25f);
        // Z do jogador deve ser maior que o Z dos tiles para ser desenhado por cima
        float playerZ = (playerGridY + playerGridX) * 0.001f + 0.5f; // Garante que o jogador está acima do tile atual

        // Com base na sua spritesheet (Vampires2_Run_full.png - 7x4 de 64x64px cada quadro):
        float dsx_player = playerSingleSpriteW / playerSpriteSheetTotalW; // Largura de um sprite / Largura total da spritesheet
        float dsy_player = playerSingleSpriteH / playerSpriteSheetTotalH; // Altura de um sprite / Altura total da spritesheet

        float offx_player = playerAnimationFrameX * dsx_player;
        float offy_player = playerAnimationFrameY * dsy_player;

        // Matriz de modelo para o jogador
        glm::mat4 M_player = glm::translate(glm::mat4(1.0f), glm::vec3(playerRenderX, playerRenderY, playerZ))
                             * glm::scale(glm::mat4(1.0f), glm::vec3(playerSingleSpriteW, playerSingleSpriteH, 1));
        glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_player));
        glUniform2f(locTS, dsx_player, dsy_player);
        glUniform2f(locTO, offx_player, offy_player);

        glBindTexture(GL_TEXTURE_2D, playerSpriteSheet);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);


        // ===========================================
        // Renderização do Contorno do Tile do Jogador
        // ===========================================
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Mudar para modo linha para o contorno
        glUniform1i(locOL, 1); // Ativar o contorno
        glUniform4f(locCLR, 1, 1, 1, 1); // Cor do contorno (branco)
        glLineWidth(2.0f); // Largura da linha do contorno

        glBindVertexArray(outlineVAO);
        {
            // Posição do contorno, no centro do tile do jogador
            float x_outline = (playerGridY - playerGridX) * halfW + mapOriginOffset.x;
            float y_outline = (playerGridY + playerGridX) * halfH + mapOriginOffset.y;
            // O Z para o contorno deve ser ligeiramente maior que o Z do tile para aparecer sobre ele, mas abaixo do jogador
            glm::mat4 M_outline = glm::translate(glm::mat4(1.0f), glm::vec3(x_outline, y_outline, (playerGridY + playerGridX) * 0.001f + 0.1f)) * glm::scale(glm::mat4(1.0f), glm::vec3(tileW, tileH, 1));
            glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_outline));
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }
        glUniform1i(locOL, 0); // Desativar o contorno novamente
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Voltar ao modo de preenchimento

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
