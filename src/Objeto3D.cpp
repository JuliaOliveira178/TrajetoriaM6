/* Objeto 3D - Atividade Acadêmica Computação Gráfica - Módulo 6
 * Júlia Oliveira
 * Câmera em primeira pessoa com classe Camera.
 * Inclui: View Matrix (lookAt), Projection Matrix (perspective),
 *         movimento WASD com deltaTime, mouse look (yaw/pitch), zoom com scroll.
 *         Trajetórias cíclicas por waypoints para cada objeto.
 *
 * Controles:
 *   TAB       - alterna objeto selecionado: 0 -> 1 -> 2 -> 0
 *   R         - modo Girar    -> setas giram o objeto
 *   T         - modo Transladar -> setas transladam o objeto
 *   P         - modo Escalar  -> setas/+/- escalam o objeto (S reservado para WASD)
 *   C         - adiciona ponto de controle na posição atual do objeto selecionado
 *   G         - inicia/pausa animação de trajetória do objeto selecionado
 *   U         - remove todos os pontos de controle do objeto selecionado
 *   1/2/3     - liga/desliga luz principal / preenchimento / fundo
 *   W/A/S/D   - move câmera (frente/esquerda/trás/direita)
 *   Mouse     - orienta câmera (yaw/pitch)
 *   Scroll    - zoom (altera FOV)
 *   ESC       - fecha a janela
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ---------------------------------------------------------------------------
// Classe Camera — encapsula posição, orientação, movimento e rotação
// ---------------------------------------------------------------------------
class Camera {
public:
    glm::vec3 posicao;
    glm::vec3 frente;
    glm::vec3 cima;
    glm::vec3 direita;

    float yaw;
    float pitch;
    float fov;
    float velocidade;
    float sensibilidade;

    bool primeiraMouse;
    float ultimoX, ultimoY;

    Camera(float larguraTela, float alturaTela,
           glm::vec3 posInicial = glm::vec3(0.0f, 0.0f, 3.0f))
        : posicao(posInicial),
          frente(glm::vec3(0.0f, 0.0f, -1.0f)),
          cima(glm::vec3(0.0f, 1.0f, 0.0f)),
          yaw(-90.0f), pitch(0.0f),
          fov(45.0f), velocidade(2.5f), sensibilidade(0.05f),
          primeiraMouse(true),
          ultimoX(larguraTela / 2.0f), ultimoY(alturaTela / 2.0f)
    {
        atualizarVetores();
    }

    glm::mat4 getMatrizView() const {
        return glm::lookAt(posicao, posicao + frente, cima);
    }

    glm::mat4 getMatrizProjecao(float aspectRatio) const {
        return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
    }

    // Move a câmera com base nas teclas WASD pressionadas no frame
    void processarTeclado(GLFWwindow* janela, float deltaTime) {
        float vel = velocidade * deltaTime;
        if (glfwGetKey(janela, GLFW_KEY_W) == GLFW_PRESS)
            posicao += frente * vel;
        if (glfwGetKey(janela, GLFW_KEY_S) == GLFW_PRESS)
            posicao -= frente * vel;
        if (glfwGetKey(janela, GLFW_KEY_A) == GLFW_PRESS)
            posicao -= direita * vel;
        if (glfwGetKey(janela, GLFW_KEY_D) == GLFW_PRESS)
            posicao += direita * vel;
    }

    // Orienta a câmera a partir do deslocamento do mouse
    void processarMouse(double xpos, double ypos) {
        if (primeiraMouse) {
            ultimoX = (float)xpos;
            ultimoY = (float)ypos;
            primeiraMouse = false;
        }
        float xoffset = ((float)xpos - ultimoX) * sensibilidade;
        float yoffset = (ultimoY - (float)ypos) * sensibilidade; // y cresce para baixo na tela
        ultimoX = (float)xpos;
        ultimoY = (float)ypos;

        yaw   += xoffset;
        pitch += yoffset;
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        atualizarVetores();
    }

    // Zoom via scroll — altera o FOV
    void processarScroll(double yoffset) {
        fov -= (float)yoffset;
        if (fov <  1.0f) fov =  1.0f;
        if (fov > 45.0f) fov = 45.0f;
    }

private:
    void atualizarVetores() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        frente  = glm::normalize(f);
        direita = glm::normalize(glm::cross(frente, glm::vec3(0.0f, 1.0f, 0.0f)));
        cima    = glm::normalize(glm::cross(direita, frente));
    }
};

// ---------------------------------------------------------------------------
// Structs auxiliares
// ---------------------------------------------------------------------------
struct Material {
    string arquivoTextura;
    glm::vec3 ka = {0.2f, 0.2f, 0.2f};
    glm::vec3 kd = {0.8f, 0.8f, 0.8f};
    glm::vec3 ks = {0.5f, 0.5f, 0.5f};
    float ns = 32.0f;
};

struct Luz {
    glm::vec3 posicao;
    glm::vec3 cor;
    float intensidade;
    bool ativa = true;
};

struct Objeto {
    GLuint vao;
    GLuint textura;
    int nVertices;
    glm::vec3 posicao;
    float escala;
    float anguloX, anguloY, anguloZ;
    Material material;

    // Trajetória: lista de pontos de controle e estado de animação
    vector<glm::vec3> pontosControle;
    bool animando   = false;
    int   idxPonto  = 0;
    float tPonto    = 0.0f;
    float tGlobal   = 0.0f;  // t global para Bézier cíclica

    Objeto(GLuint v, GLuint tex, int nv, glm::vec3 pos, float s, Material m)
        : vao(v), textura(tex), nVertices(nv), posicao(pos), escala(s), material(m),
          anguloX(0.0f), anguloY(0.0f), anguloZ(0.0f) {}
};

// ---------------------------------------------------------------------------
// Protótipos
// ---------------------------------------------------------------------------
void key_callback(GLFWwindow* janela, int tecla, int scancode, int acao, int modo);
void mouse_callback(GLFWwindow* janela, double xpos, double ypos);
void scroll_callback(GLFWwindow* janela, double xoffset, double yoffset);
GLuint carregarOBJ(const string& caminho, int& nVertices);
Material lerMTL(const string& caminhomtl);
GLuint carregarTextura(const string& caminho);
int inicializarShader();
void atualizarPosicaoLuzes();

// ---------------------------------------------------------------------------
// Constantes e globais
// ---------------------------------------------------------------------------
const GLuint LARGURA = 1000, ALTURA = 1000;

// Vertex shader: posição (0), UV (1), normal (2); aplica model/view/projection
const GLchar* fonteVertice =
    "#version 450\n"
    "layout (location = 0) in vec3 posicao;\n"
    "layout (location = 1) in vec2 coordUV;\n"
    "layout (location = 2) in vec3 normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec2 uvFragmento;\n"
    "out vec3 posFragmento;\n"
    "out vec3 normalFragmento;\n"
    "void main()\n"
    "{\n"
    "    vec4 posMundo = model * vec4(posicao, 1.0);\n"
    "    gl_Position = projection * view * posMundo;\n"
    "    posFragmento = vec3(posMundo);\n"
    "    normalFragmento = mat3(transpose(inverse(model))) * normal;\n"
    "    uvFragmento = coordUV;\n"
    "}\0";

// Fragment shader: Phong com 3 luzes pontuais e atenuação na difusa
const GLchar* fonteFragmento =
    "#version 450\n"
    "in vec2 uvFragmento;\n"
    "in vec3 posFragmento;\n"
    "in vec3 normalFragmento;\n"
    "out vec4 corSaida;\n"
    "uniform sampler2D amostradorTextura;\n"
    "uniform vec3 posCamera;\n"
    "uniform vec3 ka;\n"
    "uniform vec3 kd;\n"
    "uniform vec3 ks;\n"
    "uniform float ns;\n"
    "struct LuzPontual {\n"
    "    vec3 posicao;\n"
    "    vec3 cor;\n"
    "    float intensidade;\n"
    "    bool ativa;\n"
    "};\n"
    "uniform LuzPontual luzes[3];\n"
    "void main()\n"
    "{\n"
    "    vec3 corTex = vec3(texture(amostradorTextura, uvFragmento));\n"
    "    vec3 norm = normalize(normalFragmento);\n"
    "    vec3 dirCamera = normalize(posCamera - posFragmento);\n"
    "    vec3 resultado = ka * 0.15 * corTex;\n"
    "    for (int i = 0; i < 3; i++) {\n"
    "        if (!luzes[i].ativa) continue;\n"
    "        vec3 vetorLuz = luzes[i].posicao - posFragmento;\n"
    "        float dist = length(vetorLuz);\n"
    "        vec3 dirLuz = vetorLuz / dist;\n"
    "        float atenuacao = 1.0 / (1.0 + 1.0 * dist + 0.5 * dist * dist);\n"
    "        float difusa = max(dot(norm, dirLuz), 0.0);\n"
    "        resultado += kd * corTex * luzes[i].cor * luzes[i].intensidade * difusa * atenuacao;\n"
    "        vec3 dirReflexao = reflect(-dirLuz, norm);\n"
    "        float especular = pow(max(dot(dirCamera, dirReflexao), 0.0), ns);\n"
    "        resultado += ks * luzes[i].cor * luzes[i].intensidade * especular;\n"
    "    }\n"
    "    corSaida = vec4(resultado, 1.0);\n"
    "}\n\0";

vector<Objeto> objetos;
Luz luzes[3];
Camera* camara = nullptr;

int modoAtivo = 0;

enum Modo { GIRAR, TRANSLADAR, ESCALAR };
Modo modoTransformacao = TRANSLADAR;

const float PASSO_TRANSLACAO = 0.08f;
const float PASSO_ESCALA     = 0.04f;
const float ESCALA_MIN       = 0.04f;
const float PASSO_ROTACAO    = glm::radians(7.0f);
const float VELOCIDADE_ANIM  = 0.8f; // unidades de espaço por segundo

float deltaTime = 0.0f;
float ultimoFrame = 0.0f;

template<typename Fn>
void aplicarAoSelecionado(Fn fn) { fn(objetos[modoAtivo]); }

// Avalia uma curva de Bézier cúbica pelo algoritmo de De Casteljau
glm::vec3 bezierCubico(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    glm::vec3 q0 = glm::mix(p0, p1, t);
    glm::vec3 q1 = glm::mix(p1, p2, t);
    glm::vec3 q2 = glm::mix(p2, p3, t);
    glm::vec3 r0 = glm::mix(q0, q1, t);
    glm::vec3 r1 = glm::mix(q1, q2, t);
    return glm::mix(r0, r1, t);
}

// Avalia posição na trajetória Bézier cíclica dado tGlobal em [0, n)
glm::vec3 avaliarTrajetoria(const vector<glm::vec3>& pts, float tGlobal) {
    int n   = (int)pts.size();
    int seg = (int)tGlobal % n;
    float t = tGlobal - (int)tGlobal;
    glm::vec3 p0 = pts[ seg        % n];
    glm::vec3 p1 = pts[(seg + 1)   % n];
    glm::vec3 p2 = pts[(seg + 2)   % n];
    glm::vec3 p3 = pts[(seg + 3)   % n];
    return bezierCubico(p0, p1, p2, p3, t);
}

void atualizarPosicaoLuzes() {
    float e = objetos[0].escala;
    glm::vec3 p = objetos[0].posicao;
    luzes[0].posicao = p + glm::vec3( e*2.0f,  e*2.0f,  e*3.0f);
    luzes[1].posicao = p + glm::vec3(-e*2.0f,  e*0.5f,  e*2.0f);
    luzes[2].posicao = p + glm::vec3( e*0.0f,  e*1.5f, -e*3.0f);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    glfwInit();

    GLFWwindow* janela = glfwCreateWindow(LARGURA, ALTURA, "Objeto 3D - Camera FPS", nullptr, nullptr);
    glfwMakeContextCurrent(janela);
    glfwSetKeyCallback(janela, key_callback);
    glfwSetCursorPosCallback(janela, mouse_callback);
    glfwSetScrollCallback(janela, scroll_callback);
    glfwSetInputMode(janela, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;

    int larg, alt;
    glfwGetFramebufferSize(janela, &larg, &alt);
    glViewport(0, 0, larg, alt);

    camara = new Camera((float)LARGURA, (float)ALTURA);

    GLuint programaShader = inicializarShader();
    glUseProgram(programaShader);

    GLint locModelo    = glGetUniformLocation(programaShader, "model");
    GLint locView      = glGetUniformLocation(programaShader, "view");
    GLint locProjecao  = glGetUniformLocation(programaShader, "projection");
    GLint locCamera    = glGetUniformLocation(programaShader, "posCamera");
    GLint locKa        = glGetUniformLocation(programaShader, "ka");
    GLint locKd        = glGetUniformLocation(programaShader, "kd");
    GLint locKs        = glGetUniformLocation(programaShader, "ks");
    GLint locNs        = glGetUniformLocation(programaShader, "ns");
    glUniform1i(glGetUniformLocation(programaShader, "amostradorTextura"), 0);

    GLint locLuzPos[3], locLuzCor[3], locLuzInt[3], locLuzAtiva[3];
    for (int i = 0; i < 3; i++) {
        string b = "luzes[" + to_string(i) + "].";
        locLuzPos[i]   = glGetUniformLocation(programaShader, (b + "posicao").c_str());
        locLuzCor[i]   = glGetUniformLocation(programaShader, (b + "cor").c_str());
        locLuzInt[i]   = glGetUniformLocation(programaShader, (b + "intensidade").c_str());
        locLuzAtiva[i] = glGetUniformLocation(programaShader, (b + "ativa").c_str());
    }

    glEnable(GL_DEPTH_TEST);

    // Carrega cena a partir do arquivo de configuração (assets/cena.txt)
    // O carregarOBJ da Julia usa VAO compartilhado — carrega uma vez e reutiliza
    {
        // Carrega geometria e material base uma vez
        Material matBase = lerMTL("assets/Suzanne.mtl");
        GLuint idTexBase = carregarTextura(matBase.arquivoTextura.empty() ? "" : "assets/" + matBase.arquivoTextura);
        int nv; GLuint vaoBase = carregarOBJ("assets/modelo.obj", nv);

        ifstream cenaFile("assets/cena.txt");
        if (!cenaFile.is_open())
            cerr << "cena.txt não encontrado em assets/ — usando cena padrão." << endl;
        string linha;
        while (getline(cenaFile, linha)) {
            if (linha.empty() || linha[0] == '#') continue;
            istringstream ss(linha); string token; ss >> token;
            if (token == "obj") {
                string caminhoObj; float x, y, z, s;
                ss >> caminhoObj >> x >> y >> z >> s;
                // Carrega OBJ próprio se diferente do base, senão compartilha VAO
                int nv2; GLuint vao2, tex2; Material mat2;
                if (caminhoObj == "assets/modelo.obj") {
                    vao2 = vaoBase; nv2 = nv; tex2 = idTexBase; mat2 = matBase;
                } else {
                    string mtlPath = caminhoObj.substr(0, caminhoObj.rfind('.')) + ".mtl";
                    mat2 = lerMTL(mtlPath);
                    tex2 = carregarTextura(mat2.arquivoTextura.empty() ? "" : "assets/" + mat2.arquivoTextura);
                    vao2 = carregarOBJ(caminhoObj, nv2);
                }
                if (vao2 != (GLuint)-1)
                    objetos.push_back(Objeto(vao2, tex2, nv2, glm::vec3(x, y, z), s, mat2));
            }
        }

        // Fallback se cena.txt não carregou nada
        if (objetos.empty()) {
            if (vaoBase == (GLuint)-1) {
                cerr << "Falha ao carregar modelo OBJ." << endl;
                glfwTerminate(); return -1;
            }
            objetos.push_back(Objeto(vaoBase, idTexBase, nv, glm::vec3(-0.45f, -0.3f, 0.0f), 0.2f, matBase));
            objetos.push_back(Objeto(vaoBase, idTexBase, nv, glm::vec3( 0.45f, -0.3f, 0.0f), 0.2f, matBase));
            objetos.push_back(Objeto(vaoBase, idTexBase, nv, glm::vec3( 0.00f,  0.5f, 0.0f), 0.2f, matBase));
        }
    }

    luzes[0].cor = glm::vec3(1.0f, 1.0f, 0.95f); luzes[0].intensidade = 1.0f;
    luzes[1].cor = glm::vec3(0.8f, 0.9f, 1.0f);  luzes[1].intensidade = 0.5f;
    luzes[2].cor = glm::vec3(1.0f, 0.9f, 0.8f);  luzes[2].intensidade = 0.7f;

    while (!glfwWindowShouldClose(janela))
    {
        float frameAtual = (float)glfwGetTime();
        deltaTime = frameAtual - ultimoFrame;
        ultimoFrame = frameAtual;

        glfwPollEvents();
        camara->processarTeclado(janela, deltaTime);

        string nomeObjeto = "Objeto " + to_string(modoAtivo);
        string nomeModo = (modoTransformacao == GIRAR)      ? "GIRAR"
                        : (modoTransformacao == TRANSLADAR) ? "TRANSLADAR" : "ESCALAR";
        string estadoLuzes = string(luzes[0].ativa?"1":"_") + (luzes[1].ativa?"2":"_") + (luzes[2].ativa?"3":"_");
        string animSt = objetos[modoAtivo].animando ? " [ANIMANDO]" : "";
        glfwSetWindowTitle(janela,
            ("Objeto 3D | Julia Oliveira | Camera FPS | [" + nomeObjeto + "] [" + nomeModo + "]"
             + " | C=ponto G=animar U=limpar | [luzes:" + estadoLuzes + "]" + animSt).c_str());

        glClearColor(0.08f, 0.12f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Atualiza trajetórias: Bézier cíclica (≥4 pontos) ou linear (2-3 pontos)
        for (auto& obj : objetos) {
            if (!obj.animando || obj.pontosControle.size() < 2) continue;
            int n = (int)obj.pontosControle.size();
            if (n >= 4) {
                obj.tGlobal += VELOCIDADE_ANIM * deltaTime;
                if (obj.tGlobal >= (float)n) obj.tGlobal -= (float)n;
                obj.posicao = avaliarTrajetoria(obj.pontosControle, obj.tGlobal);
            } else {
                glm::vec3 anterior = obj.pontosControle[(obj.idxPonto - 1 + n) % n];
                glm::vec3 proximo  = obj.pontosControle[obj.idxPonto];
                float distSeg    = glm::length(proximo - anterior);
                float incremento = (distSeg > 0.0001f) ? VELOCIDADE_ANIM * deltaTime / distSeg : 1.0f;
                obj.tPonto += incremento;
                if (obj.tPonto >= 1.0f) {
                    obj.tPonto   = 0.0f;
                    obj.posicao  = proximo;
                    obj.idxPonto = (obj.idxPonto + 1) % n;
                } else {
                    obj.posicao = glm::mix(anterior, proximo, obj.tPonto);
                }
            }
        }

        // Atualiza view e projection a cada frame (FOV pode mudar com scroll)
        glm::mat4 view     = camara->getMatrizView();
        glm::mat4 projecao = camara->getMatrizProjecao((float)LARGURA / ALTURA);
        glUniformMatrix4fv(locView,     1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProjecao, 1, GL_FALSE, glm::value_ptr(projecao));
        glUniform3fv(locCamera, 1, glm::value_ptr(camara->posicao));

        atualizarPosicaoLuzes();
        for (int i = 0; i < 3; i++) {
            glUniform3fv(locLuzPos[i],   1, glm::value_ptr(luzes[i].posicao));
            glUniform3fv(locLuzCor[i],   1, glm::value_ptr(luzes[i].cor));
            glUniform1f (locLuzInt[i],      luzes[i].intensidade);
            glUniform1i (locLuzAtiva[i],    luzes[i].ativa ? 1 : 0);
        }

        glActiveTexture(GL_TEXTURE0);

        for (int i = 0; i < (int)objetos.size(); i++)
        {
            // Passa material e textura específicos de cada objeto
            glUniform3fv(locKa, 1, glm::value_ptr(objetos[i].material.ka));
            glUniform3fv(locKd, 1, glm::value_ptr(objetos[i].material.kd));
            glUniform3fv(locKs, 1, glm::value_ptr(objetos[i].material.ks));
            glUniform1f (locNs,    objetos[i].material.ns);
            glBindTexture(GL_TEXTURE_2D, objetos[i].textura);

            glm::mat4 matriz = glm::mat4(1.0f);
            matriz = glm::translate(matriz, objetos[i].posicao);
            matriz = glm::rotate(matriz, objetos[i].anguloX, glm::vec3(1.0f, 0.0f, 0.0f));
            matriz = glm::rotate(matriz, objetos[i].anguloY, glm::vec3(0.0f, 1.0f, 0.0f));
            matriz = glm::rotate(matriz, objetos[i].anguloZ, glm::vec3(0.0f, 0.0f, 1.0f));
            matriz = glm::scale(matriz, glm::vec3(objetos[i].escala));
            glUniformMatrix4fv(locModelo, 1, GL_FALSE, glm::value_ptr(matriz));
            glBindVertexArray(objetos[i].vao);
            glDrawArrays(GL_TRIANGLES, 0, objetos[i].nVertices);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(janela);
    }

    delete camara;
    glfwTerminate();
    return 0;
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------
void mouse_callback(GLFWwindow* janela, double xpos, double ypos) {
    if (camara) camara->processarMouse(xpos, ypos);
}

void scroll_callback(GLFWwindow* janela, double xoffset, double yoffset) {
    if (camara) camara->processarScroll(yoffset);
}

void key_callback(GLFWwindow* janela, int tecla, int scancode, int acao, int modo)
{
    if (tecla == GLFW_KEY_ESCAPE && acao == GLFW_PRESS)
        glfwSetWindowShouldClose(janela, GL_TRUE);

    if (tecla == GLFW_KEY_TAB && acao == GLFW_PRESS)
        modoAtivo = (modoAtivo + 1) % (int)objetos.size();

    if (acao == GLFW_PRESS) {
        if (tecla == GLFW_KEY_R) modoTransformacao = GIRAR;
        if (tecla == GLFW_KEY_T) modoTransformacao = TRANSLADAR;
        if (tecla == GLFW_KEY_P) modoTransformacao = ESCALAR;
        if (tecla == GLFW_KEY_1) luzes[0].ativa = !luzes[0].ativa;
        if (tecla == GLFW_KEY_2) luzes[1].ativa = !luzes[1].ativa;
        if (tecla == GLFW_KEY_3) luzes[2].ativa = !luzes[2].ativa;

        // C: registra a posição atual do objeto como novo ponto de controle
        if (tecla == GLFW_KEY_C) {
            Objeto& obj = objetos[modoAtivo];
            obj.pontosControle.push_back(obj.posicao);
            cout << "Ponto de controle adicionado: ("
                 << obj.posicao.x << ", "
                 << obj.posicao.y << ", "
                 << obj.posicao.z << ") — total: "
                 << obj.pontosControle.size() << endl;
        }

        // G: inicia ou pausa a animação do objeto selecionado
        if (tecla == GLFW_KEY_G) {
            Objeto& obj = objetos[modoAtivo];
            if (obj.pontosControle.size() < 2) {
                cout << "Adicione ao menos 2 pontos de controle antes de animar (tecla C)." << endl;
            } else {
                obj.animando = !obj.animando;
                if (obj.animando) {
                    obj.idxPonto = 1;
                    obj.tPonto   = 0.0f;
                    obj.tGlobal  = 0.0f;
                    cout << "Animação iniciada (" << obj.pontosControle.size() << " pontos)." << endl;
                } else {
                    cout << "Animação pausada." << endl;
                }
            }
        }

        // U: remove todos os pontos de controle e interrompe a animação
        if (tecla == GLFW_KEY_U) {
            Objeto& obj = objetos[modoAtivo];
            obj.pontosControle.clear();
            obj.animando  = false;
            obj.idxPonto  = 0;
            obj.tPonto    = 0.0f;
            obj.tGlobal   = 0.0f;
            cout << "Pontos de controle removidos." << endl;
        }
    }

    if (acao == GLFW_PRESS || acao == GLFW_REPEAT)
    {
        if (modoTransformacao == TRANSLADAR) {
            if (tecla == GLFW_KEY_LEFT)  aplicarAoSelecionado([](Objeto& o){ o.posicao.x -= PASSO_TRANSLACAO; });
            if (tecla == GLFW_KEY_RIGHT) aplicarAoSelecionado([](Objeto& o){ o.posicao.x += PASSO_TRANSLACAO; });
            if (tecla == GLFW_KEY_UP)    aplicarAoSelecionado([](Objeto& o){ o.posicao.y += PASSO_TRANSLACAO; });
            if (tecla == GLFW_KEY_DOWN)  aplicarAoSelecionado([](Objeto& o){ o.posicao.y -= PASSO_TRANSLACAO; });
        }
        if (modoTransformacao == ESCALAR) {
            if (tecla == GLFW_KEY_EQUAL || tecla == GLFW_KEY_UP)
                aplicarAoSelecionado([](Objeto& o){ o.escala += PASSO_ESCALA; });
            if (tecla == GLFW_KEY_MINUS || tecla == GLFW_KEY_DOWN)
                aplicarAoSelecionado([](Objeto& o){
                    o.escala -= PASSO_ESCALA;
                    if (o.escala < ESCALA_MIN) o.escala = ESCALA_MIN;
                });
        }
        if (modoTransformacao == GIRAR) {
            if (tecla == GLFW_KEY_LEFT)  aplicarAoSelecionado([](Objeto& o){ o.anguloY -= PASSO_ROTACAO; });
            if (tecla == GLFW_KEY_RIGHT) aplicarAoSelecionado([](Objeto& o){ o.anguloY += PASSO_ROTACAO; });
            if (tecla == GLFW_KEY_UP)    aplicarAoSelecionado([](Objeto& o){ o.anguloX -= PASSO_ROTACAO; });
            if (tecla == GLFW_KEY_DOWN)  aplicarAoSelecionado([](Objeto& o){ o.anguloX += PASSO_ROTACAO; });
            if (tecla == GLFW_KEY_X)     aplicarAoSelecionado([](Objeto& o){ o.anguloX += PASSO_ROTACAO; });
            if (tecla == GLFW_KEY_Y)     aplicarAoSelecionado([](Objeto& o){ o.anguloY += PASSO_ROTACAO; });
            if (tecla == GLFW_KEY_Z)     aplicarAoSelecionado([](Objeto& o){ o.anguloZ += PASSO_ROTACAO; });
        }
    }
}

// ---------------------------------------------------------------------------
// inicializarShader, lerMTL, carregarTextura, carregarOBJ
// ---------------------------------------------------------------------------
int inicializarShader()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &fonteVertice, NULL);
    glCompileShader(vs);
    GLint ok; GLchar log[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vs, 512, NULL, log); cout << "ERRO::VS: " << log << endl; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fonteFragmento, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(fs, 512, NULL, log); cout << "ERRO::FS: " << log << endl; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, NULL, log); cout << "ERRO::PROG: " << log << endl; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

Material lerMTL(const string& caminhomtl)
{
    Material mat;
    ifstream arq(caminhomtl);
    if (!arq.is_open()) { cerr << "MTL nao encontrado: " << caminhomtl << endl; return mat; }
    string linha;
    while (getline(arq, linha)) {
        istringstream ss(linha); string palavra; ss >> palavra;
        if      (palavra == "map_Kd") ss >> mat.arquivoTextura;
        else if (palavra == "Ka")     ss >> mat.ka.r >> mat.ka.g >> mat.ka.b;
        else if (palavra == "Kd")     ss >> mat.kd.r >> mat.kd.g >> mat.kd.b;
        else if (palavra == "Ks")     ss >> mat.ks.r >> mat.ks.g >> mat.ks.b;
        else if (palavra == "Ns")     ss >> mat.ns;
    }
    return mat;
}

GLuint carregarTextura(const string& caminho)
{
    GLuint idTex;
    glGenTextures(1, &idTex);
    glBindTexture(GL_TEXTURE_2D, idTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int larg, alt, canais;
    unsigned char* dados = stbi_load(caminho.c_str(), &larg, &alt, &canais, 0);
    if (dados) {
        GLenum fmt = (canais == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, larg, alt, 0, fmt, GL_UNSIGNED_BYTE, dados);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(dados);
        cout << "Textura carregada: " << caminho << " (" << larg << "x" << alt << ")" << endl;
    } else {
        unsigned char px[8*8*3];
        for (int i=0;i<8;i++) for (int j=0;j<8;j++) {
            int c=((i/4)+(j/4))%2;
            px[(i*8+j)*3+0]=c?0:40; px[(i*8+j)*3+1]=c?210:40; px[(i*8+j)*3+2]=c?210:40;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
        glGenerateMipmap(GL_TEXTURE_2D);
        cerr << "Textura nao encontrada (" << caminho << "), usando xadrez." << endl;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return idTex;
}

GLuint carregarOBJ(const string& caminho, int& nVertices)
{
    vector<glm::vec3> vertices;
    vector<glm::vec2> coordUV;
    vector<glm::vec3> normais;
    vector<GLfloat>   buffer;

    ifstream arq(caminho);
    if (!arq.is_open()) { cerr << "Erro ao abrir: " << caminho << endl; return (GLuint)-1; }

    string linha;
    while (getline(arq, linha)) {
        istringstream ss(linha); string palavra; ss >> palavra;
        if      (palavra == "v")  { glm::vec3 v; ss>>v.x>>v.y>>v.z; vertices.push_back(v); }
        else if (palavra == "vt") { glm::vec2 vt; ss>>vt.s>>vt.t; coordUV.push_back(vt); }
        else if (palavra == "vn") { glm::vec3 vn; ss>>vn.x>>vn.y>>vn.z; normais.push_back(vn); }
        else if (palavra == "f") {
            string token;
            while (ss >> token) {
                int vi=0, ti=0, ni=0;
                istringstream sf(token); string idx;
                if (getline(sf,idx,'/')) vi=!idx.empty()?stoi(idx)-1:0;
                if (getline(sf,idx,'/')) ti=!idx.empty()?stoi(idx)-1:0;
                if (getline(sf,idx))    ni=!idx.empty()?stoi(idx)-1:0;
                buffer.push_back(vertices[vi].x); buffer.push_back(vertices[vi].y); buffer.push_back(vertices[vi].z);
                buffer.push_back(coordUV.empty()?0.0f:coordUV[ti].s);
                buffer.push_back(coordUV.empty()?0.0f:coordUV[ti].t);
                buffer.push_back(normais.empty()?0.0f:normais[ni].x);
                buffer.push_back(normais.empty()?0.0f:normais[ni].y);
                buffer.push_back(normais.empty()?1.0f:normais[ni].z);
            }
        }
    }
    arq.close();

    GLuint vbo, vao;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer.size()*sizeof(GLfloat), buffer.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(5*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(buffer.size() / 8);
    return vao;
}
