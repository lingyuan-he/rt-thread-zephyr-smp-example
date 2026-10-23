#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>

/* Number of threads */
#define THREADS_NUM	4

#define STACK_SIZE	4096

static K_THREAD_STACK_ARRAY_DEFINE(THREAD_STACKS, THREADS_NUM, STACK_SIZE);
static struct k_thread THREADS[THREADS_NUM];
static unsigned int THREAD_NUM_ARGS[THREADS_NUM] = {1, 2, 3, 4};

void thread_entry(void *arg1, void *arg2, void *arg3)
{
    unsigned int thread_num = *((unsigned int *) arg1);
    unsigned int count = 0;
    int core_id;


    ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

    while (1) {
        core_id = arch_curr_cpu()->id;
        printk("Thread %u is running on core %u, count: %u\n",
               thread_num, core_id, count++);

        k_msleep(1000);
    }
}

int main(void)
{
    k_tid_t pid;
	int i;

	printk("Demo for SMP CPU pinning and masking.\n");

	for (i = 0; i < THREADS_NUM; i++) {
		pid = k_thread_create(
            &THREADS[i], /* thread struct */
            THREAD_STACKS[i], /* thread stack */
            STACK_SIZE, /* stack size */
		    thread_entry, /* entry function pointer */
		    (void *)(THREAD_NUM_ARGS + i), /* argument 1 - thread number */
            NULL, NULL, /* argument 2/3 - unused */
		    K_PRIO_COOP(10), /* lowest priotiry - cooperative */
            0, /* option */
            K_FOREVER /* wait for explicit start */);
        if (i == 0) {
            // Pinning, thread runs on core 0.
            k_thread_cpu_pin(pid, 0);
        } else if (i == 1) {
            // Mask enable, thread runs on core 1.
            k_thread_cpu_mask_clear(pid);
            k_thread_cpu_mask_enable(pid, 1);
        } else if (i == 2) {
            // Mask disable, thread runs on core 2.
            k_thread_cpu_mask_enable_all(pid);
            k_thread_cpu_mask_disable(pid, 0);
            k_thread_cpu_mask_disable(pid, 1);
            k_thread_cpu_mask_disable(pid, 3);
        }
        // No binding for the last thread.

        k_thread_start(pid);
	}
	return 0;
}
