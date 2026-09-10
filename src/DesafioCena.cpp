#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.cpp"

using namespace std;
using namespace glm;

const GLint WIDTH = 800, HEIGHT = 600;

// --- Variáveis do Jogador ---
float playerX = 100.0f;
float playerY = 100.0f; 
float playerW = 80.0f, playerH = 80.0f;
float playerSpeed = 5.0f;

// Física do Pulo
float velocityY = 0.0f;
float gravity = 0.8f;
float jumpForce = -16.0f; 
bool isJumping = true;
float floorY = 450.0f; 

// --- Estrutura de Múltiplos Itens ---
struct Item {
    float x, y, w, h;
    bool active;
};

// Criando uma lista com 3 itens espalhados pela fase
vector<Item> itensFase = {
    { 400.0f, 470.0f, 50.0f, 50.0f, true }, // Item 1 (No chão)
    { 800.0f, 320.0f, 50.0f, 50.0f, true }, // Item 2 (No ar, exige pulo)
    { 1200.0f, 470.0f, 50.0f, 50.0f, true } // Item 3 (Mais a frente no chão)
};

int score = 0;

// Shaders básicos com suporte a transparência (discard)
const GLchar* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 projection;
uniform mat4 model;
out vec2 TexCoord;
void main() {
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
    if(texColor.a < 0.1) discard; 
    FragColor = texColor;
}
)";

GLuint loadTexture(const char* file_name) {
    GLuint tex;
    int x, y, n;
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, 4);
    if (!image_data) {
        cerr << "ERRO: Textura " << file_name << " nao encontrada!" << endl;
        return 0;
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    return tex;
}

GLuint setupQuad() {
    GLfloat vertices[] = {
        0.0f, 1.0f,  0.0f, 1.0f, 
        1.0f, 0.0f,  1.0f, 0.0f, 
        0.0f, 0.0f,  0.0f, 0.0f, 
        0.0f, 1.0f,  0.0f, 1.0f, 
        1.0f, 1.0f,  1.0f, 1.0f, 
        1.0f, 0.0f,  1.0f, 0.0f  
    };
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    return VAO;
}

void drawSprite(GLuint shader, GLuint quadVAO, GLuint texture, float x, float y, float w, float h) {
    mat4 model = mat4(1.0f);
    model = translate(model, vec3(x, y, 0.0f));
    model = scale(model, vec3(w, h, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(model));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Algoritmo AABB - Verifica Colisao
bool checkCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    bool colX = x1 + w1 >= x2 && x2 + w2 >= x1;
    bool colY = y1 + h1 >= y2 && y2 + h2 >= y1;
    return colX && colY;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Cena de Jogo - Colisao e Fisca", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint shaderProgram = glCreateProgram();
    GLuint vs = glCreateShader(GL_VERTEX_SHADER); glShaderSource(vs, 1, &vertexShaderSource, NULL); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs, 1, &fragmentShaderSource, NULL); glCompileShader(fs);
    glAttachShader(shaderProgram, vs); glAttachShader(shaderProgram, fs); glLinkProgram(shaderProgram);

    GLuint quadVAO = setupQuad();

    // Texturas na pasta assets (agora com o .png no personagem)
    GLuint texFundo = loadTexture("../assets/fundo.png");
    GLuint texMeio = loadTexture("../assets/meio.png");
    GLuint texChao = loadTexture("../assets/chao.png");
    GLuint texPlayer = loadTexture("../assets/personagem.png"); 
    GLuint texItem = loadTexture("../assets/item.png"); 

    mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    
    float worldOffset = 0.0f; 

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));

        // --- CONTROLES ---
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) worldOffset -= playerSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  worldOffset += playerSpeed;
        
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && !isJumping) {
            velocityY = jumpForce;
            isJumping = true;
        }

        // --- FÍSICA ---
        velocityY += gravity; 
        playerY += velocityY;
        
        if (playerY >= floorY) { 
            playerY = floorY;
            velocityY = 0.0f;
            isJumping = false;
        }

        // --- PARALLAX ---
        float parallaxFundo = -fmod(-worldOffset * 0.1f, (float)WIDTH);
        float parallaxMeio  = -fmod(-worldOffset * 0.5f, (float)WIDTH);
        float parallaxChao  = -fmod(-worldOffset * 1.0f, (float)WIDTH);

        drawSprite(shaderProgram, quadVAO, texFundo, parallaxFundo, 0, WIDTH, HEIGHT);
        drawSprite(shaderProgram, quadVAO, texFundo, parallaxFundo + WIDTH, 0, WIDTH, HEIGHT);

        drawSprite(shaderProgram, quadVAO, texMeio, parallaxMeio, 0, WIDTH, HEIGHT);
        drawSprite(shaderProgram, quadVAO, texMeio, parallaxMeio + WIDTH, 0, WIDTH, HEIGHT);

        drawSprite(shaderProgram, quadVAO, texChao, parallaxChao, 0, WIDTH, HEIGHT);
        drawSprite(shaderProgram, quadVAO, texChao, parallaxChao + WIDTH, 0, WIDTH, HEIGHT);

        // --- INTERAÇÃO E RENDERIZAÇÃO DOS ITENS ---
        for (auto& item : itensFase) {
            float screenItemX = item.x + worldOffset; 
            
            // Verifica colisão apenas se o item ainda não foi coletado
            if (item.active && checkCollision(playerX, playerY, playerW, playerH, screenItemX, item.y, item.w, item.h)) {
                item.active = false; 
                score += 10;
                cout << "Evidencia coletada! Pontuacao total: " << score << endl;
            }

            // Só desenha se estiver ativo
            if (item.active) {
                drawSprite(shaderProgram, quadVAO, texItem, screenItemX, item.y, item.w, item.h);
            }
        }

        // --- RENDERIZA O JOGADOR ---
        drawSprite(shaderProgram, quadVAO, texPlayer, playerX, playerY, playerW, playerH);

        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}