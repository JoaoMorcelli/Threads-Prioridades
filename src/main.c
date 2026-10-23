#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

// LEDs e Botão
#define LED_C_NODE DT_ALIAS(led1)
#define BUTTON_NODE DT_NODELABEL(user_button_0)
#define LED_A_NODE DT_ALIAS(led0)  // LED verde
#define LED_B_NODE DT_ALIAS(led2)  // LED vermelho

static const struct gpio_dt_spec ledA = GPIO_DT_SPEC_GET(LED_A_NODE, gpios);
static const struct gpio_dt_spec ledB = GPIO_DT_SPEC_GET(LED_B_NODE, gpios);
static const struct gpio_dt_spec ledc = GPIO_DT_SPEC_GET(LED_C_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

#define PRIO_THREAD_A 4   // Maior prioridade (número menor)
#define PRIO_THREAD_B 0   // Menor prioridade
#define TEMPO_A_MS   500   // Thread A dorme
#define TEMPO_B_MS  1200   // Thread B dorme


// ISR - Toggle LED
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_set_dt(&ledc, 1);
    gpio_pin_set_dt(&ledA, 0);
    gpio_pin_set_dt(&ledB, 0);
    for (volatile int i = 0; i < 10000000; i++) {
        }
    gpio_pin_set_dt(&ledc, 0);
}

void thread_A(void *p1, void *p2, void *p3)
{
    uint64_t start_ms, end_ms, start, end;
    while (1) {
        start_ms = k_uptime_get();  // tempo atual em ms
        start = k_uptime_get();
        gpio_pin_set_dt(&ledA, 1);  // Liga LED verde

        // Simula processamento rápido 
        for (volatile int i = 0; i < 200000; i++) {}

        gpio_pin_set_dt(&ledA, 0);  // Desliga LED verde
        end_ms = k_uptime_get();  // tempo após o delay
        printk("Thread A (LED Verde) - Incompleto: %llu ms\n", end_ms - start_ms);

        k_msleep(TEMPO_A_MS);       // Dorme — libera CPU
        end = k_uptime_get();
        printk("Thread A (LED Verde) - Completo: %llu ms\n", end - start);
    }
}

void thread_B(void *p1, void *p2, void *p3)
{
    uint64_t start_msa, end_msa, starta, enda;
    while (1) {
        start_msa = k_uptime_get();  // tempo atual em ms
        starta = k_uptime_get();
        gpio_pin_set_dt(&ledB, 1);  // Liga LED vermelho

        // Simula processamento mais longo 
        for (volatile int i = 0; i < 10000000; i++) {
            if (i % 100000 == 0) {
                // Aqui não imprimimos nada — apenas ocupamos a CPU
            }
        }

        gpio_pin_set_dt(&ledB, 0);  // Desliga LED vermelho
        end_msa = k_uptime_get();  // tempo após o delay
        printk("Thread B - Incompleto: %llu ms\n", end_msa - start_msa);

        k_msleep(TEMPO_B_MS);       // Dorme um pouco
        enda = k_uptime_get();  // tempo após o delay
        printk("Thread B - Completo: %llu ms\n", enda - starta);

    }
}

K_THREAD_DEFINE(a_tid, 512, thread_A, NULL, NULL, NULL,
                PRIO_THREAD_A, 0, 0);
K_THREAD_DEFINE(b_tid, 512, thread_B, NULL, NULL, NULL,
                PRIO_THREAD_B, 0, 0);

int main(void)
{
    // Configurar LED e botão com pull-up
    gpio_pin_configure_dt(&ledc, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure_dt(&ledA, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledB, GPIO_OUTPUT_INACTIVE);

     if (!device_is_ready(ledA.port) || !device_is_ready(ledB.port)) {
        return 0;
    }
    
    // Configurar interrupção na borda de descida
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    
    // Main dorme
    while (1) {
        k_sleep(K_FOREVER);
    }
    return 0;
}