# PGCCHIB_TilemapIsometrico_MarceloOrellana

Uma demo de **tilemap isométrico** em OpenGL 3.3: gera um mapa fixo (staggered ou diamond), renderiza cada tile via spritesheet e permite navegar de forma isométrica em 8 direções, destacando o tile ativo.

> **Ambiente Testado**  
> macOS (Homebrew + CMake + GLFW + GLM + GLAD)  
> Pode ser adaptado para Linux e Windows, bastando instalar as bibliotecas correspondentes.

---

## 📂 Estrutura do Repositório

```plaintext
PGCCHIB_TilemapIsometrico_MarceloOrellana/
├── include/
│   └── glad/
│       ├── glad.h
│       └── KHR/
│           └── khrplatform.h
├── common/
│   └── glad.c
├── resources/
│   ├── tileset.png       # atlas isométrico: 7 sprites (2:1) em linha
│   └── shaders/
│       ├── tilemap.vert
│       └── tilemap.frag
├── src/
│   └── IsometricTilemap.cpp   # código‐fonte principal
├── build/
├── CMakeLists.txt
├── README.md
└── GettingStarted.md
````

---

## ⚠️ GLAD (OpenGL Loader)

Antes de compilar, baixe e extraia manualmente via [GLAD Generator](https://glad.dav1d.de/):

* **API:** OpenGL
* **Version:** 3.3+
* **Profile:** Core
* **Language:** C/C++

Depois copie:

```text
glad.h           → include/glad/
khrplatform.h    → include/glad/KHR/
glad.c           → common/
```

---

## 🚀 Como Compilar e Executar

### 1. Instalar dependências (macOS)

```bash
brew install cmake glfw
```

### 2. Build com CMake

```bash
git clone <este-repo>
cd PGCCHIB_TilemapIsometrico_MarceloOrellana
mkdir build && cd build
cmake ..
cmake --build .
```

### 3. Executar

```bash
./IsometricTilemap
```

---

## 🎯 Sobre a Demo “Tilemap Isométrico”

O objetivo é revisar tilemaps isométricos (staggered ou diamond), desenhando e navegando em oito direções:

1. **Tileset**

   * `tileset.png` contém 7 tiles em linha, cada um com proporção **2:1** (largura = 2× altura).
   * No código, subdividimos o atlas em `nCols = 7`, `nRows = 1` para obter UVs.

2. **Mapa Fixo**

   * Um array 2D `int map[H][W]` inicializado manualmente define qual índice do tileset usar.
   * Por exemplo:

     ```cpp
     const int MAP_H = 5, MAP_W = 5;
     int map[MAP_H][MAP_W] = {
       {1, 1, 4, 0, 2},
       {4, 1, 4, 3, 4},
       {4, 4, 1, 2, 1},
       {0, 3, 2, 1, 4},
       {2, 2, 3, 1, 0}
     };
     ```

3. **Desenho em Isométrico**

   * Para cada célula `(i,j)`:

     1. Calcular posição no mundo isométrico:

        ```
        float x = (i - j) * (tileWidth  / 2);
        float y = (i + j) * (tileHeight / 2);
        ```
     2. Aplicar `model = translate + scale` e enviar UV correspondente ao índice `map[i][j]`.
   * Usamos projeção ortográfica (`glm::ortho(0,800,0,600,-1,1)`) para que 1 unidade = 1 pixel.

4. **Navegação**

   * Um “cursor” (sprite rosa ou retângulo wireframe) destaca a célula `(ci,cj)` atual.
   * Controles via teclado:

     ```
     N  (↑) : ci--, cj--      NE (→↑) : ci--, cj++
     E  (→) :      cj++
     SE (→↓) : ci++, cj++
     S  (↓) : ci++, cj--      
     SW (←↓) : ci++, cj--
     W  (←) :      cj--      
     NW (←↑) : ci--, cj--
     ```
   * A cada tecla, atualizamos `(ci,cj)` respeitando limites `[0..H-1],[0..W-1]` e reposicionamos o cursor.

5. **Shaders**

   * **Vertex Shader** (`tilemap.vert`): recebe `in vec2 aPos; in vec2 aUV;` e `uniform mat4 projection, model; uniform vec2 texScale, texOffset;` → `gl_Position = projection * model * vec4(aPos,0,1); UV=aUV*texScale+texOffset;`
   * **Fragment Shader** (`tilemap.frag`): sample da textura ou, se `u_outline==true`, colore o cursor em branco.

6. **Fluxo de Render**

   1. `glClear(...)`
   2. `glUseProgram(tilemapProg)`
   3. Para cada célula, configurar `model`, `texScale`, `texOffset` e `DrawQuad()`
   4. Mudar `polygonMode` para `GL_LINE`, ligar `u_outline`, desenhar o retângulo wireframe do cursor em volta do tile ativo.

---

## 🔧 Parâmetros Ajustáveis

* **Dimensões do mapa:** `MAP_H`, `MAP_W`
* **Tamanho do tile:** `tileWidth`, `tileHeight` (em pixels)
* **Spritesheet:** `nCols` (7) e `nRows` (1)
* **Velocidade de navegação:** taxa de repetição de teclas, se desejar animação contínua

---

## 📖 GettingStarted.md

Consulte [GettingStarted.md](GettingStarted.md) para:

* Instalar e configurar CMake, GLFW e GLM
* Solução de problemas comuns de build
* Dicas de integração com VS Code
