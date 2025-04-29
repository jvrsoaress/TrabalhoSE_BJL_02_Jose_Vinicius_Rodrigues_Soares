<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=02A6F4&height=120&section=header"/>
<h1 align="center">Embarcatech - Projeto Integrado - BitDogLab </h1>

## Objetivo do Projeto

Desenvolver um ohmímetro que mede resistências de 510Ω a 100kΩ, identifica o valor comercial E24 mais próximo, exibe o valor e as faixas de cor no display OLED, e representa as faixas visualmente em uma matriz de LEDs 5x5. O sistema inclui um botão para modo BOOTSEL, facilitando o upload de firmware, e uma interface amigável com mensagem de erro para resistências acima de 100kΩ.

## 🗒️ Lista de requisitos

- **Leitura de botão B (push-buttons)**
- **Utilização da matriz de LEDs**: como saídas para feedback visual;
- **Exibição de informações em tempo real no display gráfico 128x64 (SSD1306)**;
- **Estruturação do projeto no ambiente VS Code**: previamente configurado para o desenvolvimento com o RP2040.

## 🛠 Tecnologias

1. **Microcontrolador**: Raspberry Pi Pico W (na BitDogLab).
2. **Display OLED SSD1306**: 128x64 pixels, conectado via I2C (GPIO 14 - SDA, GPIO 15 - SCL).
3. **Botão B: GPIO 6**.
4. **Matriz de LEDs: WS2812** (GPIO 7).
5. **Linguagem de Programação:** C  
6. **Frameworks:** Pico SDK


## 🔧 Funcionalidades Implementadas:

**Funções dos Componentes**
   
- Display: Exibe o valor do do ADC, do resistor calculado em tempo real, da aproximação do resistor a partir do resistor em tempo real, e as faixas de cores de um determinado resistor calculado.
- Matriz de LEDs: Exibe as faixas de cores de um determinado resistor calculado.
- Botões: Botão B entra no modo de upload de firmware (BOOTSEL).

## 🔧 Fluxograma Geral:

- **Display OLED:** Exibe o valor da resistência de um determinado resistor calculado e as faixas de cor desse resistor.
- **Matriz de LEDs: WS2812:** Representa visualmente as faixas de cores de um determinado resistor. (exp: "Faixa: Vermelho, Amarelo, Laranaja").
- **Botão B: GPIO 6:** Ativa o modo BOOTSEL via interrupção, facilitando o upload de firmware, exibindo o drive USB.


## 🚀 Passos para Compilação e Upload do projeto Ohmímetro com Matriz de LEDs

1. **Instale o Pico SDK**
2. **Crie uma pasta `build`**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make

3. **Transferir o firmware para a placa**

- Conectar a placa BitDogLab ao computador
- Copiar o arquivo .uf2 gerado para o drive da placa

4. **Testar o projeto**

🛠🔧🛠🔧🛠🔧


## 🎥 Demonstração: 

- Para ver o funcionamento do projeto, acesse o vídeo de demonstração gravado por José Vinicius em: https://youtu.be/PKZBUCtdCLs

## 💻 Desenvolvedor
 
<table>
  <tr>
    <td align="center"><img style="" src="https://avatars.githubusercontent.com/u/191687774?v=4" width="100px;" alt=""/><br /><sub><b> José Vinicius </b></sub></a><br />👨‍💻</a></td>
  </tr>
</table>
