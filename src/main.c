#include <kernel.h>
#include <sys/printk.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(race_condition_banco, LOG_LEVEL_INF);

#define STACK_SIZE 1024
#define THREAD_PRIORITY 7
#define NUM_OPERACOES 10
#define VALOR_TRANSACAO 50

/* RECURSO COMPARTILHADO: Saldo bancário global */
static volatile int saldo_conta = 1000;

/* Thread 1: Realiza depósitos sucessivos na conta */
void thread_depositar(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_OPERACOES; i++) {
        /* 1. LEITURA: Lê o saldo global desatualizado no momento */
        int saldo_local = saldo_conta;
        LOG_INF("[Depósito] Leu saldo atual: R$ %d", saldo_local);

        /* 2. JANELA DE VULNERABILIDADE:
         * O k_msleep entrega a CPU para a outra thread antes de atualizar a memória */
        k_msleep(2);

        /* 3. MODIFICAÇÃO E ESCRITA: Processa o cálculo e sobrescreve o saldo */
        saldo_local += VALOR_TRANSACAO;
        saldo_conta = saldo_local;
        LOG_INF("--> [Depósito] Atualizou saldo para: R$ %d", saldo_conta);

        k_msleep(5);
    }
}

/* Thread 2: Realiza saques sucessivos na conta */
void thread_sacar(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_OPERACOES; i++) {
        /* 1. LEITURA */
        int saldo_local = saldo_conta;
        LOG_INF("[Saque   ] Leu saldo atual: R$ %d", saldo_local);

        /* 2. JANELA DE VULNERABILIDADE */
        k_msleep(2);

        /* 3. MODIFICAÇÃO E ESCRITA */
        saldo_local -= VALOR_TRANSACAO;
        saldo_conta = saldo_local;
        LOG_INF("--> [Saque   ] Atualizou saldo para: R$ %d", saldo_conta);

        k_msleep(5);
    }
}

/* Instanciação das duas threads no Zephyr */
K_THREAD_DEFINE(thread_dep, STACK_SIZE, thread_depositar, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread_saq, STACK_SIZE, thread_sacar,     NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

int main(void)
{
    int saldo_inicial = saldo_conta;
    int saldo_esperado = saldo_inicial; // + (10 * 50) - (10 * 50) = 1000

    LOG_INF("==================================================");
    LOG_INF(" DEMONSTRACAO DE RACE CONDITION: SISTEMA BANCARIO ");
    LOG_INF("==================================================");
    LOG_INF("Saldo Inicial: R$ %d", saldo_inicial);
    LOG_INF("Executando %d depósitos e %d saques de R$ %d...", NUM_OPERACOES, NUM_OPERACOES, VALOR_TRANSACAO);
    LOG_INF("Saldo Final Esperado: R$ %d", saldo_esperado);
    LOG_INF("--------------------------------------------------");

    /* Aguarda a conclusão dos ciclos de ambas as threads */
    k_msleep(3000);

    /* Verificação de corrupção dos dados */
    LOG_INF("==================================================");
    LOG_INF("RESULTADO FINAL DA CONTA:");
    LOG_INF("Saldo Esperado : R$ %d", saldo_esperado);
    LOG_INF("Saldo REAL     : R$ %d", saldo_conta);

    if (saldo_conta != saldo_esperado) {
        LOG_INF("STATUS         : RACE CONDITION DETECTADA!");
        LOG_INF("Diferença de R$ %d devido a operações concorrentes desprotegidas.", saldo_conta - saldo_esperado);
    } else {
        LOG_INF("STATUS         : Nenhuma inconsistência detectada (Execução atípica).");
    }
    LOG_INF("==================================================");

    return 0;
}