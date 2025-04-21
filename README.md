# 🎮 Projeto: RelembrandoBitDogLab

Este projeto embarcado foi desenvolvido para a placa **BitDogLab com Raspberry Pi Pico W (RP2040)** e demonstra como integrar múltiplos periféricos, como joystick analógico, display OLED, matriz de LEDs WS2812, botões e buzzer. Ele permite movimentar um quadrado com o joystick no display SSD1306, reproduzir melodias com botões e alterar as cores dos LEDs conforme o movimento.

## 🧰 Funcionalidades

- 🎮 Leitura de eixo X e Y do joystick analógico via ADC
- 🟦 Exibição da posição do joystick em um display OLED SSD1306 com um quadrado que se move conforme o movimento
- 🔘 Botões com interrupção para tocar melodias (Super Mario e Pac-Man)
- 🎵 Saída de som via buzzer PWM com reprodução de melodias
- 🌈 Controle de LEDs WS2812 com cores dinâmicas baseadas na posição do joystick
- 💡 LED RGB controlado por botão do joystick

## 📦 Componentes Utilizados

- Placa **BitDogLab** com RP2040
- Display **OLED SSD1306** (I2C)
- **Joystick analógico** (2 eixos + botão)
- **Botões físicos** (A e B)
- **LED RGB SMD 5050**
- **Matriz de LEDs WS2812**
- **Buzzer piezoelétrico**
- Cabos e conexões

## 🔌 Conexões de Hardware

| Componente         | GPIO       |
|--------------------|------------|
| Joystick X         | GPIO 26 (ADC1) |
| Joystick Y         | GPIO 27 (ADC0) |
| Joystick botão     | GPIO 22    |
| Botão A            | GPIO 5     |
| Botão B            | GPIO 6     |
| Display SSD1306    | SDA: GPIO 14, SCL: GPIO 15 |
| LED RGB discreto   | GPIOs definidos como `LED_BLUE_PIN`, `LED_RED_PIN`, `LED_GREEN_PIN` |
| Matriz WS2812      | GPIO definido como `MATRIZ_LED_PIN` |
| Buzzer             | GPIO definido como `BUZZER_PIN_A` |

## 🧠 Lógica do Programa

- O joystick é lido por ADC e seus valores são mapeados para coordenadas da tela.
- Um quadrado de 8x8 pixels é desenhado no display e se move conforme o joystick.
- Se os botões A ou B forem pressionados, melodias específicas são tocadas no buzzer.
- Se o joystick for pressionado, os LEDs RGB são ativados com cor branca.
- Dependendo do eixo extremo do joystick (superior, inferior, esquerda, direita), a matriz WS2812 muda de cor.

## 🎼 Melodias

- Botão **A**: toca a melodia do tema do **Super Mario Bros**
- Botão **B**: toca a melodia do **Pac-Man**

