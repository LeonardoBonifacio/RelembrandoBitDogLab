//Bibliotecas padrões de C
#include <stdio.h>
#include <stdlib.h>
// Bibliotecas usadas no projeto
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/audio_leds.h"
#include "hardware/pio.h" 
#include "pico/bootrom.h"
#include "hardware/pwm.h"


// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2812.pio.h"

// Constantes para display ssd1306 e comunicação i2c
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C // Endereço do display ssd1306
ssd1306_t ssd; // Inicializa a estrutura do display

// Tamanho do quadrado e tela SSD1306
#define QUAD_SIZE 8  
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Posição inicial do quadrado (centro da tela)
int posX = SCREEN_WIDTH / 2 - QUAD_SIZE / 2;
int posY = SCREEN_HEIGHT / 2 - QUAD_SIZE / 2;


// Constantes e variáveis para botões, joystick e valores adc x e y
#define JOYSTICK_X_PIN 26  // GPIO para eixo X
#define JOYSTICK_Y_PIN 27  // GPIO para eixo Y
#define JOYSTICK_PUSHBUTTON 22 // GPIO para botão do Joystick
#define Botao_A 5 // GPIO para botão A
#define Botao_B 6// GPIO para botão B para entrar no modo de gravação
// Ajuste do centro real do joystick
#define CENTER_X 1939
#define CENTER_Y 2180
// Valores lidos pelos adc de x e y
uint16_t adc_value_x = 0;
uint16_t adc_value_y = 0;
volatile uint32_t ultimo_tempo_buttons = 0;// Para armazenar o tempo da última interrupção acionada pelos botôes
// flags para tocar as melodias
volatile bool tocar_melodia_A = false;
volatile bool tocar_melodia_B = false;


void init_i2c_and_display_ssd1306(){
    // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line

  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_fill(&ssd, false);// Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_send_data(&ssd); // Envia os dados para o display

}


// Função que recebe um endereço de memória onde serão armazenados os valores x e y de um joystick 
void joystick_read_axis(uint16_t *adc_value_x, uint16_t *adc_value_y) {
    adc_select_input(1); // Seleciona o canal do adc que lê o eixo x do joystick
    sleep_us(2);// tempo para estabilizar após a troca de canal
    *adc_value_x = adc_read(); // acesso do valor guardado neste endereço de memória e gravação da leitura do eixo x
  
    adc_select_input(0);  // Seleciona o canal do adc que lê o eixo x do joystick
    sleep_us(2);// tempo para estabilizar após a troca de canal
    *adc_value_y = adc_read();// acesso do valor guardado neste endereço de memória e gravação da leitura do eixo y
}


// Mapeia valores do ADC para a tela SSD1306
int posicao_adc_pra_displayssd1306(int adc_value, int center_value, int screen_max) {
  // center value = joystick parado
  int range_min = center_value; // de onde o valor começa na tela , neste caso com o joystick parado é no centro dela
  int range_max = 4095 - center_value; // ate que valor ele pode ir baseando-se no centro da dela com o joystick parado
  
  // offset representa quanto você se afastou do centro da tela
  // valores negativos indicam movimentos para esquerda e baixo
  // valores positivos indicam movimentos para direita e cima
  int offset = adc_value - center_value; 

  int mapped_value;
  // screen_max / 2 dá o valor do centro da tela
  // se o offset for negativo o valor mapeado é da metade esquerda da tela
  // se o offset for positivo o valor mapeado é da metade direita da tela
  if (offset < 0) {
      mapped_value = ((offset * (screen_max / 2)) / range_min) + (screen_max / 2);
  } else {
      mapped_value = ((offset * (screen_max / 2)) / range_max) + (screen_max / 2);
  }

  // garante que os valores não ultrapassem os limites da tela  
  if (mapped_value < 0) mapped_value = 0;
  if (mapped_value > screen_max) mapped_value = screen_max;

  return mapped_value;
}


// Atualiza a posição do quadrado no display
void update_square_position(uint16_t *adc_value_x, uint16_t *adc_value_y) {
  // lê os valores x e y do adc no endereços de memoria passados
  uint16_t adc_x = *adc_value_x;
  uint16_t adc_y = *adc_value_y;
  // garante uma pequena margem da borda da tela
  int limite_borda = 1;

  // calcula a posição  de x passando os valores do adc_x e onde é o centro com o joystick parado no eixo x
  posX = posicao_adc_pra_displayssd1306(adc_x, CENTER_X, SCREEN_WIDTH - QUAD_SIZE - limite_borda);
  // posy é invertido , quuanto maior o adc_y , mais para cima  vai o quadrado na tela (por que 0 é o topo da tela)
  posY = SCREEN_HEIGHT - QUAD_SIZE - posicao_adc_pra_displayssd1306(adc_y, CENTER_Y, SCREEN_HEIGHT - QUAD_SIZE - limite_borda);

  // garante que o quadrado não saia da tela ou passe da "borda"
  if (posX < limite_borda) posX = limite_borda;
  if (posY < limite_borda) posY = limite_borda;
}

// Atualiza o display
void desenha_display(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);// Limpa a tela do display
    ssd1306_rect(ssd, posY, posX, QUAD_SIZE, QUAD_SIZE, true, true);// Desenha o quadrado na nova posição
    ssd1306_send_data(ssd);// Envia os dados para o display
}

void gpio_irq_handler(uint gpio, uint32_t events){
  uint32_t tempo_atual = time_us_32() / 1000;  // Obtém o tempo atual em milissegundos e o armazena
  if (tempo_atual - ultimo_tempo_buttons < 350) return;// Se o tempo passado for menor que o atraso  de debounce(350ms) retorne imediatamente
  ultimo_tempo_buttons = tempo_atual;// O tempo atual corresponde ao último tempo que o botão foi pressionado, ja que ele passou pela verificação acima

  if (gpio == Botao_A)tocar_melodia_A = true; // altera flag para tocar a melodia do mario
  
  if (gpio == Botao_B)tocar_melodia_B = true; // altera flag para tocar melodia do pac man

  if (gpio == JOYSTICK_PUSHBUTTON){ // acende todos os leds do led rgb SMD5050 formando a cor branca
    gpio_put(LED_BLUE_PIN,1);
    gpio_put(LED_RED_PIN,1);
    gpio_put(LED_GREEN_PIN,1);
    set_one_led(255,255,255,leds_preenchidos);
  }

}

// Função de inicialização geral de gpio, adc,buzzer e interrupções
void init_gpios_and_adc_and_leds_and_buzzer(){
  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);
  gpio_set_irq_enabled_with_callback(Botao_A,GPIO_IRQ_EDGE_FALL,true,&gpio_irq_handler);
  
  gpio_init(Botao_B);
  gpio_set_dir(Botao_B, GPIO_IN);
  gpio_pull_up(Botao_B);
  gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  
  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN); 

  gpio_init(JOYSTICK_PUSHBUTTON);
  gpio_set_dir(JOYSTICK_PUSHBUTTON, GPIO_IN);
  gpio_pull_up(JOYSTICK_PUSHBUTTON); 
  gpio_set_irq_enabled_with_callback(JOYSTICK_PUSHBUTTON,GPIO_IRQ_EDGE_FALL,true,&gpio_irq_handler);

  gpio_init(LED_BLUE_PIN);
  gpio_init(LED_GREEN_PIN);
  gpio_init(LED_RED_PIN);
  
  gpio_set_dir(LED_BLUE_PIN,GPIO_OUT);
  gpio_set_dir(LED_RED_PIN,GPIO_OUT);
  gpio_set_dir(LED_GREEN_PIN,GPIO_OUT);

  // Configuração do GPIO para o buzzer como saída
  gpio_init(BUZZER_PIN_A);
  gpio_set_dir(BUZZER_PIN_A, GPIO_OUT);
  // Inicializar o PWM no pino do buzzer
  configure_buzzer(BUZZER_PIN_A);

}

// Função para mudar as cores dos led ou desliga-los da matriz de leds ws2812 
// Ao atingir qualquer extremidade do joystick(cima,baixo,direita,esquerda)
void muda_cores_leds_conforme_adc(uint16_t *adc_value_x, uint16_t *adc_value_y){
  uint16_t adc_x = *adc_value_x;
  uint16_t adc_y = *adc_value_y;
  if (adc_y < 30){
    gpio_put(LED_BLUE_PIN,0);
    gpio_put(LED_RED_PIN,0);
    gpio_put(LED_GREEN_PIN,0);
    set_one_led(0,0,0,leds_preenchidos);
  }

  if(adc_y > 4080){
    gpio_put(LED_BLUE_PIN,0);
    gpio_put(LED_RED_PIN,1);
    gpio_put(LED_GREEN_PIN,0);
    set_one_led(255,0,0,leds_preenchidos);
  }

  if (adc_x < 30)
  {
    gpio_put(LED_BLUE_PIN,1);
    gpio_put(LED_RED_PIN,0);
    gpio_put(LED_GREEN_PIN,0);
    set_one_led(0,0,255,leds_preenchidos);
  }

  if(adc_x > 4080){
    gpio_put(LED_BLUE_PIN,0);
    gpio_put(LED_RED_PIN,0);
    gpio_put(LED_GREEN_PIN,1);
    set_one_led(0,255,0,leds_preenchidos);
  }
  
  
}




int main()
{
    stdio_init_all();
     // Váriaveis e configurações PIO
     PIO pio = pio0;
     int sm = 0;
     uint offset = pio_add_program(pio, &ws2812_program);
     ws2812_program_init(pio, sm, offset, MATRIZ_LED_PIN, 800000, false);
    init_gpios_and_adc_and_leds_and_buzzer();
    init_i2c_and_display_ssd1306();

    while (true) {
      joystick_read_axis(&adc_value_x, &adc_value_y);
      update_square_position(&adc_value_x, &adc_value_y);
      desenha_display(&ssd);
      muda_cores_leds_conforme_adc(&adc_value_x, &adc_value_y);
      if (tocar_melodia_A) {
        tocar_melodia_A = false;
        printf("Tocando mario");
        play_melody('A');
      }
      if (tocar_melodia_B)
      {
        printf("Tocando Pac-Man");
        tocar_melodia_B = false;
        play_melody('B');
      }
      
      printf("Valor adc X : %d    Valor adc Y: %d\n", adc_value_x,adc_value_y);
    }
}
