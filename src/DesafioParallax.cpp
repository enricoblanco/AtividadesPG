#include <iostream>
#include <string>
#include <cmath> // Para fmod

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.cpp"

using namespace std;
using namespace glm;

const GLint WIDTH = 800, HEIGHT = 600;

// Posição e velocidade do jogador
float playerX = 400.0f;
float playerY = 450.0f; // Perto da parte de baixo da tela
float playerSpeed = 5.0f;

// --- Shaders ---
const GLchar* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 model;

out vec2 TexCoord;

void main() {
    // Transforma a posição local do vértice para a tela
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const GLchar* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    vec4 texColor = texture(spriteTexture, TexCoord);
    // Descarta pixels transparentes (essencial para sprites PNG)
    if(texColor.a < 0.1) {
        discard;
    }
    FragColor = texColor;
}
)";

// Função de carregamento de textura adaptada para Core Profile
GLuint loadTexture(const char* file_name) {
    GLuint tex;
    int x, y, n;
    int force_channels = 4; // Forçamos RGBA
    
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);
    if (!image_data) {
        cerr << "ERRO: Não foi possivel carregar a textura " << file_name << endl;
        return 0;
    }
    
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(image_data);
    return tex;
}

// Cria a malha padrão de 1x1 unidade para os Sprites
GLuint setupQuad() {
    // Vértices de um retângulo 1x1 com coordenadas de textura (0 a 1)
    GLfloat vertices[] = {
        // Posição (X, Y)  // Textura (U, V)
        0.0f, 1.0f,        0.0f, 1.0f, // Top-Left
        1.0f, 0.0f,        1.0f, 0.0f, // Bottom-Right
        0.0f, 0.0f,        0.0f, 0.0f, // Bottom-Left

        0.0f, 1.0f,        0.0f, 1.0f, // Top-Left
        1.0f, 1.0f,        1.0f, 1.0f, // Top-Right
        1.0f, 0.0f,        1.0f, 0.0f  // Bottom-Right
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Atributo 0: Posição (2 floats)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Atributo 1: Coordenada de Textura (2 floats)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

// Função utilitária para desenhar qualquer Sprite na tela
void drawSprite(GLuint shader, GLuint quadVAO, GLuint texture, float x, float y, float w, float h) {
    mat4 model = mat4(1.0f);
    // Transladar primeiro e depois escalar (a ordem importa na matriz!)
    model = translate(model, vec3(x, y, 0.0f));
    model = scale(model, vec3(w, h, 1.0f));
    
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shader, "spriteTexture"), 0);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Tarefa: Parallax Scrolling", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -1;
    }

    // Habilita Blending (Transparência do Canal Alpha dos PNGs)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compilação dos Shaders
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    GLuint quadVAO = setupQuad();

    // --- CARREGAMENTO DAS TEXTURAS ---
    GLuint texFundo   = loadTexture("../assets/fundo.png");
    GLuint texNuvens  = loadTexture("../assets/nuvens.png"); // Nova camada!
    GLuint texMeio    = loadTexture("../assets/meio.png");
    GLuint texChao    = loadTexture("../assets/chao.png");
    
    // ATENÇÃO: Você precisa adicionar essa imagem na pasta assets!
    GLuint texPlayer  = loadTexture("../assets/personagem.png"); 

    // Projeção Ortográfica
    mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // --- ENTRADA DO TECLADO ---
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
            
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) playerX += playerSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  playerX -= playerSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    playerY -= playerSpeed; 
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  playerY += playerSpeed;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));

        // --- CÁLCULO DO PARALLAX (4 Níveis de Velocidade) ---
        float offsetFundo  = -fmod(playerX * 0.05f, (float)WIDTH); // Quase parado
        float offsetNuvens = -fmod(playerX * 0.20f, (float)WIDTH); // Lento
        float offsetMeio   = -fmod(playerX * 0.50f, (float)WIDTH); // Médio
        float offsetChao   = -fmod(playerX * 1.00f, (float)WIDTH); // Rápido

        // --- RENDERIZAÇÃO DAS CAMADAS (De trás para frente) ---
        
        // 1. Céu Fixo
        drawSprite(shaderProgram, quadVAO, texFundo, offsetFundo, 0, WIDTH, HEIGHT);
        drawSprite(shaderProgram, quadVAO, texFundo, offsetFundo + WIDTH, 0, WIDTH, HEIGHT);

        // 2. Nuvens
        drawSprite(shaderProgram, quadVAO, texNuvens, offsetNuvens, 0, WIDTH, HEIGHT);
        drawSprite(shaderProgram, quadVAO, texNuvens, offsetNuvens + WIDTH, 0, WIDTH, HEIGHT);

        // 3. Meio (Montanhas/Árvores)
        drawSprite(shaderProgram, quadVAO, texMeio, offsetMeio, 0, WIDTH, HEIGHT);
        drawSprite(shaderProgram, quadVAO, texMeio, offsetMeio + WIDTH, 0, WIDTH, HEIGHT);

        // 4. Chão
        drawSprite(shaderProgram, quadVAO, texChao, offsetChao, 0, WIDTH, HEIGHT);
        drawSprite(shaderProgram, quadVAO, texChao, offsetChao + WIDTH, 0, WIDTH, HEIGHT);

        // 5. Jogador
        drawSprite(shaderProgram, quadVAO, texPlayer, 350.0f, playerY, 100.0f, 100.0f);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}