# M6 — Trajetórias com Curvas de Bézier
**Júlia Oliveira | Computação Gráfica**

---

Visualizador 3D interativo com três objetos texturizados, iluminação Phong com técnica de 3 luzes pontuais, câmera em primeira pessoa e trajetórias cíclicas por curvas de Bézier cúbicas (algoritmo De Casteljau).

---

## Setup — Compilação e execução

Requisitos: **CMake 3.10+**, compilador **C++17** (MSVC recomendado), **Git**.

```powershell
# Na pasta raiz do projeto (Cubo3D/)
mkdir build
cd build
cmake ..
cmake --build . --config Debug
cp -r ..\assets .\Debug\assets
cd Debug
.\Objeto3D.exe
```

> GLFW, GLM e stb_image são obtidos automaticamente pelo CMake na primeira compilação.

A cena é configurada pelo arquivo `assets/cena.txt`:
```
# formato: obj <arquivo_obj> <x> <y> <z> <escala>
obj assets/modelo.obj -0.45 -0.3 0.0 0.2
```

---

## Como usar

O título da janela mostra o objeto selecionado, o modo ativo, o estado das luzes e se a animação está em curso.

### Câmera
| Tecla / Input | Ação |
|---|---|
| `W` / `S` / `A` / `D` | Move câmera frente / trás / esquerda / direita |
| Mouse | Orienta câmera (yaw/pitch) |
| Scroll | Zoom (FOV entre 1° e 45°) |

### Seleção e transformações
- `TAB` — alterna objeto selecionado (0 → 1 → 2 → 0)
- `R` — modo Girar
- `T` — modo Transladar
- `P` — modo Escalar (`S` reservado para câmera)

| Modo | `←` `→` | `↑` `↓` |
|------|---------|---------|
| Girar | Eixo Y | Eixo X |
| Transladar | Eixo X | Eixo Y |
| Escalar | — | Aumentar / Diminuir |

No modo **Girar**, `X` `Y` `Z` rotacionam no eixo correspondente.  
No modo **Escalar**, `+` e `-` também funcionam.

### Trajetórias (Bézier cúbica)
| Tecla | Ação |
|---|---|
| `C` | Adiciona ponto de controle na posição atual do objeto selecionado |
| `G` | Inicia / pausa animação (mínimo 2 pontos; ≥4 usa Bézier cúbica) |
| `U` | Remove todos os pontos de controle do objeto selecionado |

**Como usar:** modo `T` → mova com setas → `C` (repita para cada ponto) → `G` para animar.

### Luzes
| Tecla | Ação |
|---|---|
| `1` | Liga/desliga luz principal (key light) |
| `2` | Liga/desliga luz de preenchimento (fill light) |
| `3` | Liga/desliga luz de fundo (back light) |

`ESC` fecha a aplicação.

---

## Assets

| Asset | Procedência |
|---|---|
| `modelo.obj` | [Repositório de exemplos da disciplina](https://github.com/fellowsheep/FCG2025-1) |
| `texture.png` | [Poly Haven](https://polyhaven.com/textures) — licença CC0 |

---

## Referências

- **OpenGL:** [learnopengl.com](https://learnopengl.com) — iluminação Phong, mapeamento de textura, câmera FPS
- **GLFW:** [glfw.org/docs](https://www.glfw.org/docs/latest/)
- **GLM:** [glm.g-truc.net](https://glm.g-truc.net/)
- **stb_image:** [github.com/nothings/stb](https://github.com/nothings/stb)
- **Curvas de Bézier (De Casteljau):** slides da disciplina — Módulo 6, Rossana B. Queiroz
- **Formato OBJ/MTL:** [paulbourke.net/dataformats/obj](http://paulbourke.net/dataformats/obj/)
