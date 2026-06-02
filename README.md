# M5 — Câmera em Primeira Pessoa
**Júlia Oliveira | Computação Gráfica**

---

Cena interativa com três objetos 3D texturizados, iluminação Phong com técnica de 3 luzes pontuais e câmera em primeira pessoa controlada por teclado e mouse.

---

## Compilação

Requisitos: **CMake 3.10+** e compilador **C++17** (MSVC recomendado).

```powershell
cd build
cmake ..
cmake --build . --config Debug
cd Debug
.\Objeto3D.exe
```

> GLFW, GLM e stb_image são obtidos automaticamente pelo CMake na primeira compilação.

> **Textura desatualizada?** Copie manualmente para a pasta do executável:
> ```powershell
> cp ..\assets\texture.png assets\texture.png
> ```

---

## Como usar

O título da janela mostra o objeto selecionado, o modo ativo e o estado das luzes.

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

**Ações com as setas (conforme o modo ativo)**

| Modo | `←` `→` | `↑` `↓` |
|------|---------|---------|
| Girar | Eixo Y | Eixo X |
| Transladar | Eixo X | Eixo Y |
| Escalar | — | Aumentar / Diminuir |

No modo **Girar**, as teclas `X` `Y` `Z` rotacionam no eixo correspondente.  
No modo **Escalar**, `+` e `-` também funcionam.

### Luzes
| Tecla | Ação |
|---|---|
| `1` | Liga/desliga luz principal (key) |
| `2` | Liga/desliga luz de preenchimento (fill) |
| `3` | Liga/desliga luz de fundo (back) |

`ESC` fecha a aplicação.

---

## Stack

| | |
|---|---|
| Renderização | OpenGL 4.5 / GLSL 450 |
| Janela e input | GLFW 3.4 |
| Matemática 3D | GLM |
| Loader OpenGL | GLAD |
| Texturas | stb_image |
