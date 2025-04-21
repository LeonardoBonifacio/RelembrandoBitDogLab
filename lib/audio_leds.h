#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h" 
#include <math.h>
#include <stdlib.h>  // para rand() e srand()
#include <time.h>    // para time()

// Configuração do pino do buzzer
#define BUZZER_PIN_A 21
#define TEMPO_MARIO_BROS 200
#define TEMPO_PAC_MAN 105

// Definição do número de LEDs e pinos.
#define LED_COUNT 25
#define MATRIZ_LED_PIN 7

// Constantes dos pinos dos leds
#define LED_GREEN_PIN 11                    
#define LED_BLUE_PIN 12                   
#define LED_RED_PIN 13       

const int* melodia;

const int melody_mario_bros[] = {
    659, 8, 659, 8, 0, 8, 659, 8, 0, 8, 523, 8, 659, 8,
    784, 4, 0, 4, 392, 8, 0, 4, 523, -4, 392, 8, 0, 4,
    330, -4, 440, 4, 494, 4, 466, 8, 440, 4, 392, -8,
    659, -8, 784, -8, 880, 4, 698, 8, 784, 8, 0, 8,
    659, 4, 523, 8, 587, 8, 494, -4, 523, -4, 392, 8,
    0, 4, 330, -4, 440, 4, 494, 4, 466, 8, 440, 4,
    392, -8, 659, -8, 784, -8, 880, 4, 698, 8, 784, 8,
    0, 8, 659, 4, 523, 8, 587, 8, 494, -4
};



int melody_pac_man[] = {
    // Pacman
    494, 16, 988, 16, 740, 16, 622, 16, 
    988, 32, 740, -16, 622, 8, 523, 16,
    1047, 16, 1568, 16, 1319, 16, 1047, 32, 1568, -16, 1319, 8,
  
    494, 16, 988, 16, 740, 16, 622, 16, 988, 32, 
    740, -16, 622, 8, 622, 32, 659, 32, 698, 32,
    698, 32, 740, 32, 784, 32, 784, 32, 831, 32, 880, 16, 988, 8
  };
  


// Calcula a duração de uma semibreve em milissegundos
const int WHOLE_NOTE_MARIO_BROS  = (60000 * 4) / TEMPO_MARIO_BROS; // Usado para calcular a duração de cada nota 
const int WHOLE_NOTE_PAC_MAN = (60000 * 4) / TEMPO_PAC_MAN; // Usado para calcular a duração de cada nota 

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 0
bool leds_preenchidos[LED_COUNT] = {
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1
};


static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}
void set_one_led(uint8_t r, uint8_t g, uint8_t b, bool numero_a_ser_desenhado[])
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < LED_COUNT; i++)
    {
        if (numero_a_ser_desenhado[i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

void configure_buzzer(int pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM); // Define o pino 21 para função pwm
    uint slice_num = pwm_gpio_to_slice_num(pin); // Pega o slice correspondente a este pino
    pwm_config config = pwm_get_default_config(); // Peega um conjunto de valores padrões para a configuração do pwm
    pwm_config_set_clkdiv(&config, 4.0); // Define o divisor de clock
    pwm_init(slice_num, &config, true); // Inicializa o pwm naquele slice
    pwm_set_gpio_level(pin, 0); // Define o duty cycle pra 0
}

void play_note(int pin, int freq, int duracao) {
    uint8_t r,g,b;
    // Inicializa o gerador de números aleatórios com uma semente baseada no tempo
    srand(time(NULL)); 
    r = rand() % 256;
    g = rand() % 256;
    b = rand() % 256;
    if (freq == 0) { // Se não houver frequência so espere pela duração fornecida
        set_one_led(0,0,0,leds_preenchidos);
        gpio_put(LED_RED_PIN,0);
        sleep_ms(duracao);
        return;
    }
    uint slice_num = pwm_gpio_to_slice_num(pin); // Pega o slice correspondete ao pino
    uint32_t clock_hz = clock_get_hz(clk_sys); // Pega o clock do sistema que é 125Mhz
    uint32_t wrap = 12500;// Configura o wrap a 12500
    uint32_t clkdiv = clock_hz / (freq * wrap); //  Calcula o divisor de clock
    pwm_set_wrap(slice_num, wrap - 1); // Define o wrap no slice correspondente
    pwm_set_clkdiv(slice_num, clkdiv); // Define o divisor de clock no slice correspondente
    pwm_set_gpio_level(pin, wrap / 2);// Define o duty cicle a 50%
    set_one_led(r,g,b,leds_preenchidos);
    sleep_ms(duracao * 0.9); // Toca a nota por 90% da duração
    pwm_set_gpio_level(pin, 0); // Define o duty cicle a 0%
    set_one_led(0,0,0,leds_preenchidos);
    sleep_ms(duracao * 0.1); // Pausa por 10% da durção
}


int play_melody(char melody) {
    configure_buzzer(BUZZER_PIN_A); // Configura o pino do buzzer

    const int* melodia;
    int num_notas;
    int whole_note_duration;

    // Escolhe qual melodia usar com base no parâmetro
    switch (melody) {
        case 'A':
            melodia = melody_mario_bros;
            num_notas = sizeof(melody_mario_bros) / sizeof(melody_mario_bros[0]) / 2;
            whole_note_duration = WHOLE_NOTE_MARIO_BROS;
            break;
        case 'B':
            melodia = melody_pac_man;
            num_notas = sizeof(melody_pac_man) / sizeof(melody_pac_man[0]) / 2;
            whole_note_duration = WHOLE_NOTE_PAC_MAN;
            break;
        default:
            return -1; // Código de erro: melodia inválida
    }

    for (int i = 0; i < num_notas * 2; i += 2) {
        int freq = melodia[i];
        int divisor = melodia[i + 1];
        int duracao;

        if (divisor > 0)
            duracao = whole_note_duration / divisor;
        else
            duracao = (whole_note_duration / abs(divisor)) * 1.5;

        play_note(BUZZER_PIN_A, freq, duracao);
    }

    return 0;
}
