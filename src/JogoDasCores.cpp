#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

const GLuint WIDTH = 800, HEIGHT = 600;

// Estrutura para manter o estado de cada retângulo na grade
struct Retangulo {
    vec2 pos;
    vec4 cor;
    bool ativo;
};

vector<Retangulo> grade;
GLuint VAO;
int pontuacao = 0;
int tentativas = 0;

// Shaders conforme exigido pelo material (VS e FS utilizando Uniforms)
const GLchar* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 position;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(position, 0.0, 1.0);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 330 core
uniform vec4 fc; // Cor recebida via uniform conforme PDF
out vec4 frg;
void main() {
    frg = fc;
}
)";

// Função para calcular a similaridade de cores (Distância Euclidiana RGB)
bool coresSaoSimilares(vec4 c1, vec4 c2) {
    float distancia = sqrt(pow(c1.r - c2.r, 2) + 
                           pow(c1.g - c2.g, 2) + 
                           pow(c1.b - c2.b, 2));
    // Limiar de tolerância. Ajuste para deixar o jogo mais fácil (maior) ou difícil (menor)
    return distancia < 0.35f; 
}

// Inicializa ou reinicia a grade de retângulos
void inicializarGrade() {
    grade.clear();
    pontuacao = 0;
    tentativas = 0;
    
    float dy = 50.0f; // Margem inicial do topo
    // Laço Y para as linhas
    for (int linha = 0; linha < 15; linha++) {
        float dx = 50.0f; // Margem inicial da esquerda
        // Laço X para as colunas
        for (int coluna = 0; coluna < 7; coluna++) {
            Retangulo ret;
            ret.pos = vec2(dx, dy);
            // Sorteia cor RGB
            ret.cor = vec4(
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                1.0f
            );
            ret.ativo = true;
            grade.push_back(ret);
            
            dx += 100.0f; // Incremento horizontal 
        }
        dy += 35.0f; // Incremento vertical
    }
    cout << "\n=== JOGO DAS CORES INICIADO ===" << endl;
    cout << "Clique em um retangulo para eliminar cores similares." << endl;
    cout << "Pontuacao Inicial: " << pontuacao << endl;
}

// Callback de Clique do Mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        // Verifica em qual retângulo o usuário clicou (Hitbox)
        vec4 corSelecionada(-1.0f);
        bool acertouAlgum = false;

        for (auto& ret : grade) {
            if (ret.ativo && 
                mx >= ret.pos.x && mx <= ret.pos.x + 90.0f && // Largura 90
                my >= ret.pos.y && my <= ret.pos.y + 25.0f) { // Altura 25
                corSelecionada = ret.cor;
                acertouAlgum = true;
                break;
            }
        }

        // Se clicou em um retângulo válido, avalia a jogada
        if (acertouAlgum) {
            tentativas++;
            pontuacao -= 15; // Custo por tentativa
            int removidos = 0;

            for (auto& ret : grade) {
                if (ret.ativo && coresSaoSimilares(ret.cor, corSelecionada)) {
                    ret.ativo = false; // Remove o retângulo da tela
                    pontuacao += 10;   // Bônus por acerto
                    removidos++;
                }
            }
            
            cout << "Tentativa " << tentativas << ": " << removidos 
                 << " retangulos removidos! Pontuacao atual: " << pontuacao << endl;
        }
    }
}

// Callback de Teclado (Para Reiniciar)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
        
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        inicializarGrade(); // Permite ao usuário reiniciar o jogo
}

// Configuração do VAO único (Formato Base)
void setupGeometry() {
    // Um retângulo simples ancorado na origem, largura 90, altura 25.
    GLfloat vertices[] = {
        0.0f,  0.0f,  // Top-Left
        90.0f, 0.0f,  // Top-Right
        0.0f,  25.0f, // Bottom-Left

        90.0f, 0.0f,  // Top-Right
        90.0f, 25.0f, // Bottom-Right
        0.0f,  25.0f  // Bottom-Left
    };

    GLuint VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Compilação dos Shaders
GLuint setupShader() {
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

    glDeleteShader(vs);
    glDeleteShader(fs);
    return shaderProgram;
}

int main() {
    srand(static_cast<unsigned>(time(0)));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo das Cores", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    GLuint shaderID = setupShader();
    setupGeometry();
    inicializarGrade();

    glUseProgram(shaderID);

    // Projeção Ortográfica - Facilita o mapeamento do mouse direto para a tela
    mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);

        // Desenha apenas os retângulos que ainda estão ativos (não foram destruídos)
        for (const auto& ret : grade) {
            if (ret.ativo) {
                mat4 model = mat4(1.0f);
                model = translate(model, vec3(ret.pos.x, ret.pos.y, 0.0f));
                
                glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
                glUniform4f(glGetUniformLocation(shaderID, "fc"), ret.cor.r, ret.cor.g, ret.cor.b, ret.cor.a);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    // Ao fechar, mostra o resultado final
    cout << "\n=== FIM DE JOGO ===" << endl;
    cout << "Tentativas totais: " << tentativas << endl;
    cout << "Pontuacao Final: " << pontuacao << endl;

    glfwTerminate();
    return 0;
}