#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

#include "hardware/i2c.h"
#include "inc/ssd1306.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

#define JOYSTICK_X_PIN 26  // GPIO para eixo X
#define JOYSTICK_Y_PIN 27  // GPIO para eixo Y
#define JOYSTICK_PB 22 // GPIO para botão do Joystick
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
struct pixel_t {
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
   };
   typedef struct pixel_t pixel_t;
   typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.
   
   // Declaração do buffer de pixels que formam a matriz.
   npLED_t leds[LED_COUNT];
   
   // Variáveis para uso da máquina PIO.
   PIO np_pio;
   uint sm;
   
   /**
   * Inicializa a máquina PIO para controle da matriz de LEDs.
   */
   void npInit(uint pin) {
   
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
   
    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
      np_pio = pio1;
      sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }
   
    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
   
    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i) {
      leds[i].R = 0;
      leds[i].G = 0;
      leds[i].B = 0;
    }
   }
   
   /**
   * Atribui uma cor RGB a um LED.
   */
   void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
   }
   
   /**
   * Limpa o buffer de pixels.
   */
   void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
      npSetLED(i, 0, 0, 0);
   }
   
   /**
   * Escreve os dados do buffer nos LEDs.
   */
   void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
      pio_sm_put_blocking(np_pio, sm, leds[i].G);
      pio_sm_put_blocking(np_pio, sm, leds[i].R);
      pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
   }


#define botao_pin 5
#define botaob_pin 6
static void gpio_irq_handler(uint gpio, uint32_t events);
volatile int x = 0;
#define LEDA 12
#define LEDB 11

volatile int pos_x = 60;
volatile int pos_y = 28;


#define BUZ 21
// Configuração da frequência do buzzer (em Hz)
#define BUZZER_FREQUENCY 100


// Variáveis para debouncing
volatile uint64_t ultimo_tempo_ponto = 0;
volatile uint64_t ultimo_tempo_traco = 0;
const uint64_t debounce_time_us = 100000; // 100 ms para debouncing

void gpio_callback(uint gpio, uint32_t events);

// Definição de uma função para inicializar o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(pin, 0);
}


// Definição de uma função para emitir um beep com duração especificada
void beep(uint pin, uint duration_ms) {
    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o duty cycle para 50% (ativo)
    pwm_set_gpio_level(pin, 2048);

    // Temporização
    sleep_ms(duration_ms);

    // Desativar o sinal PWM (duty cycle 0)
    pwm_set_gpio_level(pin, 0);

    // Pausa entre os beeps
    sleep_ms(100); // Pausa de 100ms
}

int main()
{
    stdio_init_all();

    // Inicializa matriz de LEDs NeoPixel.
    npInit(LED_PIN);
    npClear();

    gpio_init(LEDA);              // Inicializa o pino do LED
    gpio_set_dir(LEDA, GPIO_OUT);


    gpio_init(LEDB);              // Inicializa o pino do LED
    gpio_set_dir(LEDB, GPIO_OUT);


    gpio_init(botao_pin);                    // Inicializa o botão
    gpio_set_dir(botao_pin, GPIO_IN);        // Configura o pino como entrada
    gpio_pull_up(botao_pin);                 // Habilita o pull-up interno
    // Configuração da interrupção com callback
    
    gpio_init(botaob_pin);                    // Inicializa o botão
    gpio_set_dir(botaob_pin, GPIO_IN);        // Configura o pino como entrada
    gpio_pull_up(botaob_pin); 

    gpio_set_irq_enabled_with_callback(botao_pin, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(botaob_pin, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB); 

    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);  
  


    uint16_t adc_value_x;
    uint16_t adc_value_y;  
    char str_x[5];  // Buffer para armazenar a string
    char str_y[5];  // Buffer para armazenar a string  
  
    bool cor = true;
    

     // Inicializar o PWM no pino do buzzer
     pwm_init_buzzer(BUZ);
    
     // I2C Initialisation. Using it at 400Khz.
     i2c_init(I2C_PORT, 400*1000);
  
     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
     gpio_pull_up(I2C_SDA);
     gpio_pull_up(I2C_SCL);

     ssd1306_t ssd;

     ssd1306_init(&ssd, WIDTH, HEIGHT, false, 0x3C, I2C_PORT);
     ssd1306_config(&ssd);

     ssd1306_send_data(&ssd);
 

     while (1) {
        //printf("x = %d\n", x);
        //sleep_ms(1000);



        //LED
        while (x == 0 || x<=0) {
            gpio_put(LEDA, true);
            sleep_ms(500);
            gpio_put(LEDA, false);
            sleep_ms(500);
        }//Buzzer
         while (x == 1) {
            beep(BUZ, 500);
        } //LED 2
        while (x == 2) {
            gpio_put(LEDB, true);
            sleep_ms(500);
            gpio_put(LEDB, false);
            sleep_ms(500);
        }// Matriz de leds
        while (x == 3){
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);
            // Aqui, você desenha nos LEDs.
            npSetLED(0, 255, 0, 0); // Define o LED de índice 0 para vermelho.
            sleep_ms(200);
            npSetLED(8, 255, 0, 0); // Define o LED de índice 12 (centro da matriz) para verde.
            sleep_ms(200);
            npSetLED(12, 255, 0, 0);
            sleep_ms(200);
            npSetLED(16, 255, 0, 0);
            sleep_ms(200);
            npSetLED(24, 255, 0, 0);
            sleep_ms(200);
            npSetLED(4, 255, 0, 0);
            sleep_ms(200);
            npSetLED(6, 255, 0, 0);
            sleep_ms(200);
            npSetLED(18, 255, 0, 0);
            sleep_ms(200);
            npSetLED(20, 255, 0, 0);
            sleep_ms(200);
            npWrite(); // Escreve os dados nos LEDs.
            sleep_ms(1000);

            npClear();
            npWrite();


        }
        /*while (x==4){
            ssd1306_fill(&ssd, false);
            ssd1306_hline(&ssd, 0, 127, 32, true);
            ssd1306_vline(&ssd, 63, 0, 63, true);
            //ssd1306_draw_string(&ssd, 0, 0, "Bem-vindos!");
            //ssd1306_draw_string(&ssd, 0, 10, "Embarcatech");
            ssd1306_send_data(&ssd); // Atualiza o display

            ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
            ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16); // Desenha uma string
            ssd1306_send_data(&ssd); // Atualiza o display

            ssd1306_send_data(&ssd);
            
        }*/
        while (x==4){
            adc_select_input(0); // Seleciona o ADC para eixo X. O pino 26 como entrada analógica
            adc_value_x = adc_read();
            adc_select_input(1); // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
            adc_value_y = adc_read();    
            sprintf(str_x, "%d", adc_value_x);  // Converte o inteiro em string
            sprintf(str_y, "%d", adc_value_y);  // Converte o inteiro em string
            
            //cor = !cor;
            // Atualiza o conteúdo do display com animações
            ssd1306_fill(&ssd, !cor); // Limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
            ssd1306_line(&ssd, 3, 25, 123, 25, cor); // Desenha uma linha
            ssd1306_line(&ssd, 3, 37, 123, 37, cor); // Desenha uma linha   
            ssd1306_draw_string(&ssd, "CEPEDI   LUIZ", 8, 6); // Desenha uma string
            ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16); // Desenha uma string
            ssd1306_draw_string(&ssd, "ADC   Joystick", 10, 28); // Desenha uma string 
            ssd1306_draw_string(&ssd, "Y    X    PB", 20, 41); // Desenha uma string    
            ssd1306_line(&ssd, 44, 37, 44, 60, cor); // Desenha uma linha vertical         
            ssd1306_draw_string(&ssd, str_x, 8, 52); // Desenha uma string     
            ssd1306_line(&ssd, 84, 37, 84, 60, cor); // Desenha uma linha vertical      
            ssd1306_draw_string(&ssd, str_y, 49, 52); // Desenha uma string   
            ssd1306_rect(&ssd, 52, 90, 8, 8, cor, !gpio_get(JOYSTICK_PB)); // Desenha um retângulo  
            ssd1306_rect(&ssd, 52, 102, 8, 8, cor, !gpio_get(botao_pin)); // Desenha um retângulo    
            ssd1306_rect(&ssd, 52, 114, 8, 8, cor, !cor); // Desenha um retângulo       
            ssd1306_send_data(&ssd); // Atualiza o display
        
        
            sleep_ms(100);
        }
        while (x== 5 || x>=5){
            adc_select_input(0); // X
    adc_value_x = adc_read();
    adc_select_input(1); // Y
    adc_value_y = adc_read();

    // Mapeamento simples (de 0-4095 para 0-112 e 0-56)
    pos_x = (adc_value_x * (128 - 8)) / 2047;
    pos_y = (adc_value_y * (64 - 8)) / 2047;;

    ssd1306_fill(&ssd, false); // Limpa a tela
    ssd1306_rect(&ssd, pos_x, pos_y, 8, 8, true, true); // Quadrado 8x8 preenchido
    ssd1306_send_data(&ssd); // Atualiza a tela

    sleep_ms(50);
        }
    }


}


// Função de interrupção
void gpio_callback(uint gpio, uint32_t events) {
    uint64_t agora = time_us_64(); // Obtém o tempo atual em microssegundos

    if (gpio == botao_pin && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((agora - ultimo_tempo_ponto) > debounce_time_us) {
            x++; // Alternar para a letra A
           
        }
        ultimo_tempo_ponto = agora; // Atualiza o tempo da última interrupção
    }

    if (gpio == botaob_pin && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((agora - ultimo_tempo_traco) > debounce_time_us) {
            x--; // Alternar para a letra B
           
        }
        ultimo_tempo_traco = agora; // Atualiza o tempo da última interrupção
    }
}