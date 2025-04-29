#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "pico/bootrom.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "ws2812.pio.h"

// definições para I2C e ADC
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define ADC_PIN 28  // GPIO para o ohmímetro
#define BUTTON_B_PIN 6  // GPIO para ser utilizado o modo BOOTSEL com botão B

// definições para a matriz de LEDs
#define WS2812_PIN 7  // pino para a matriz WS2812
#define NUM_PIXELS 25 // 5x5 LEDs
#define LEDS_PER_COL 5 // LEDs por coluna

// variáveis globais
int R_conhecido = 10000;   // resistor de 10k ohm p/ utilizar como referência (marrom, preto, laranja, ouro)
float R_x = 0.0;           // resistor desconhecido
float ADC_VREF = 3.31;     // tensão de referência do ADC
int ADC_RESOLUTION = 4095; // resolução do ADC (12 bits)

// funções para a matriz de LEDs
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// definir cores para as faixas (nomes abreviados para caber no display)
typedef struct {
    const char* nome;
    uint8_t r, g, b;
} Cor;

Cor cores[] = {
    {"Pret", 0, 0, 0},         // 0
    {"Marr", 165, 42, 42},     // 1
    {"Verm", 255, 0, 0},       // 2
    {"Lara", 255, 50, 0},      // 3 
    {"Amar", 255, 255, 0},     // 4 
    {"Verd", 0, 128, 0},       // 5
    {"Azul", 0, 0, 255},       // 6
    {"Viol", 128, 0, 128},     // 7
    {"Cinz", 128, 128, 128},   // 8
    {"Bran", 255, 255, 255},   // 9
    {"Ouro", 230, 80, 10}      // tolerância (5%)
};

// função para exibir as cores na matriz de LEDs (cada coluna = uma faixa)
void display_colors(int faixa1, int faixa2, int multiplicador, int tolerancia) {
    // ordem dos LEDs na matriz 5x5 
    int pixel_map[5][5] = {
        {0, 1, 2, 3, 4},      // linha 0: esquerda → direita
        {9, 8, 7, 6, 5},      // linha 1: direita → esquerda
        {10, 11, 12, 13, 14}, // linha 2: esquerda → direita
        {19, 18, 17, 16, 15}, // linha 3: direita → esquerda
        {20, 21, 22, 23, 24}  // linha 4: esquerda → direita
    };

    // inicializa todos os LEDs como apagados
    uint32_t pixels[NUM_PIXELS];
    for (int i = 0; i < NUM_PIXELS; i++) {
        pixels[i] = 0;
    }

    // preenche as colunas com as cores correspondentes 
    // coluna 1: faixa 1
    for (int i = 0; i < LEDS_PER_COL; i++) {
        pixels[pixel_map[i][4]] = urgb_u32(cores[faixa1].r / 8, cores[faixa1].g / 8, cores[faixa1].b / 8);
    }

    // coluna 2: faixa 2
    for (int i = 0; i < LEDS_PER_COL; i++) {
        pixels[pixel_map[i][3]] = urgb_u32(cores[faixa2].r / 8, cores[faixa2].g / 8, cores[faixa2].b / 8);
    }

    // coluna 3: multiplicador
    for (int i = 0; i < LEDS_PER_COL; i++) {
        pixels[pixel_map[i][2]] = urgb_u32(cores[multiplicador].r / 8, cores[multiplicador].g / 8, cores[multiplicador].b / 8);
    }

    // coluna 4: tolerância
    for (int i = 0; i < LEDS_PER_COL; i++) {
        pixels[pixel_map[i][1]] = urgb_u32(cores[tolerancia].r / 8, cores[tolerancia].g / 8, cores[tolerancia].b / 8);
    }

    // coluna 5: toda apagaada
    for (int i = 0; i < LEDS_PER_COL; i++) {
        pixels[pixel_map[i][0]] = urgb_u32(0, 0, 0);
    }

    // enviar pixels para a matriz
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(pixels[i]);
    }
}

// função para apagar todos os LEDs
void apagar_leds() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(0);
    }
}

// função para encontrar o valor E24 mais próximo
float encontrar_E24(float valor) {
    float e24[] = {510, 560, 620, 680, 750, 820, 910, 1000, 1100, 1200, 1300, 1500, 1600, 1800, 
                   2000, 2200, 2400, 2700, 3000, 3300, 3600, 3900, 4300, 4700, 5100, 5600, 
                   6200, 6800, 7500, 8200, 9100, 10000, 11000, 12000, 13000, 15000, 16000, 
                   18000, 20000, 22000, 24000, 27000, 30000, 33000, 36000, 39000, 43000, 
                   47000, 51000, 56000, 62000, 68000, 75000, 82000, 91000, 100000};
    float closest = e24[0];
    float min_diff = fabs(valor - e24[0]);
    for (int i = 1; i < sizeof(e24)/sizeof(e24[0]); i++) {
        float diff = fabs(valor - e24[i]);
        if (diff < min_diff) {
            min_diff = diff;
            closest = e24[i];
        }
    }
    return closest;
}

// função para formatar o valor do resistor (ex: "Resistor: 910", "Resistor: 24k", "Resistor: 1.0M")
void formatar_resistor(float valor, char* str) {
    if (valor >= 1000000) {
        sprintf(str, "Resistor: %.1fM", valor / 1000000);
    } else if (valor >= 1000) {
        sprintf(str, "Resistor: %.0fk", valor / 1000);
    } else {
        sprintf(str, "Resistor: %.0f", valor);
    }
}

// função para determinar as faixas de cor
void determinar_faixas(float resistencia, int* faixa1, int* faixa2, int* multiplicador, int* tolerancia) {
    int valor = (int)resistencia; // ex: 24000
    int digitos = floor(log10(valor)); // ex: log10(24000) = 4
    int mult = digitos - 1; // ex: 4 - 1 = 3 (para 10^3)
    if (mult < 0) mult = 0; // evitar multiplicador negativo
    int base = valor / pow(10, mult); // ex: 24000 / 10^3 = 24
    *faixa1 = base / 10; // ex: 24 / 10 = 2 (vermelho)
    *faixa2 = base % 10; // ex: 24 % 10 = 4 (amarelo)
    *multiplicador = mult; // ex: 3 (laranja, para 10^3)
    *tolerancia = 10; // ouro (±5%)

    // garantir que as faixas estejam entre 0 e 9
    if (*faixa1 > 9) *faixa1 = 9;
    if (*faixa2 > 9) *faixa2 = 0;
    if (*multiplicador > 9) *multiplicador = 0;
}

// função de interrupção para o botão B (modo BOOTSEL)
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

int main() {
    stdio_init_all();

    // inicialização do I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // inicialização do display OLED
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // inicialização do ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(2); // GPIO 28

    // inicialização do botão B (modo BOOTSEL)
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // inicialização da matriz de LEDs
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    while (true) {
        float soma = 0.0f;
        for (int i = 0; i < 500; i++) {
            soma += adc_read();
            sleep_ms(1);
        }
        float media = soma / 500.0f;

        // cálculo da resistência em tempo real
        R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
        if (R_x < 0 || isnan(R_x) || isinf(R_x)) R_x = 0; // evitar valores inválidos

        // verificar se a resistência excede 100k
        if (R_x > 100000) {
            // limpar display e mostrar mensagem de erro
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "Resistor", 26, 4);
            ssd1306_draw_string(&ssd, "excedeu", 28, 16);
            ssd1306_draw_string(&ssd, "os limites", 18, 28);
            ssd1306_draw_string(&ssd, ":(", 44, 55);
            ssd1306_send_data(&ssd);

            // apagar todos os LEDs
            apagar_leds();
        } else {
            // determinar faixas de cor com base no valor E24 mais próximo
            float R_e24 = encontrar_E24(R_x);
            int faixa1, faixa2, multiplicador, tolerancia;
            determinar_faixas(R_e24, &faixa1, &faixa2, &multiplicador, &tolerancia);

            // formatar strings para o display
            char str_adc[10];
            char str_res[10];
            char str_nome[20];
            char str_faixas[40];
            sprintf(str_adc, "%1.0f", media);
            sprintf(str_res, "%1.0f", R_x);
            formatar_resistor(R_e24, str_nome); // usar R_e24 para a parte superior
            sprintf(str_faixas, "Faixa: %s,   %s, %s", 
                    cores[faixa1].nome, cores[faixa2].nome, cores[multiplicador].nome);

            // atualizar o display
            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 122, 60, true, false); // borda fixa
            ssd1306_line(&ssd, 3, 37, 123, 37, true);       // linha horizontal
            ssd1306_draw_string(&ssd, str_nome, 8, 6);      // ex: "Resistor: 24k"
            ssd1306_draw_string(&ssd, str_faixas, 10, 16);  // faixas das cores, ex: "Faixa: Verm,   Amar, Lara"
            ssd1306_line(&ssd, 3, 3, 3, 60, true);          // borda vertical esquerda
            ssd1306_draw_string(&ssd, "ADC", 13, 41);
            ssd1306_draw_string(&ssd, "Resisten.", 50, 41);
            ssd1306_line(&ssd, 44, 37, 44, 60, true);       // linha vertical
            ssd1306_draw_string(&ssd, str_adc, 8, 52);
            ssd1306_draw_string(&ssd, str_res, 59, 52);     // valor bruto de R_x
            ssd1306_send_data(&ssd);

            // atualizar a matriz de LEDs
            display_colors(faixa1, faixa2, multiplicador, tolerancia);
        }

        sleep_ms(700);
    }

    return 0;
}