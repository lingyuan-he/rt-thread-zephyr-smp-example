#include <stdint.h>
#include <stdio.h>
#include <rtthread.h>
#include <rthw.h>

#define THREAD_PRIORITY 25
#define THREAD_STACK_SIZE 4096
#define THREAD_TIMESLICE 5
// Number of threads.
#define NUM_THREADS 4

// Thread structs.
static struct rt_thread thread_structs[NUM_THREADS];
// Thread stacks.
rt_align(RT_ALIGN_SIZE) static rt_uint8_t thread_stacks[NUM_THREADS][THREAD_STACK_SIZE];
// Thread number for arguments.
static rt_uint32_t thread_args[NUM_THREADS];
// Thread names.
static char thread_names[NUM_THREADS][10];

// Spinlock for logging.
rt_spinlock_t lock;

static void thread_entry(void *parameter)
{
    rt_uint32_t count = 0, thread_num = *((rt_uint32_t *) parameter);

    while (1)
    {
        rt_spin_lock(&lock);
        rt_kprintf("Thread %u is running on core %d, count: %u\n",
                   thread_num, rt_hw_cpu_id(), count++);
        rt_spin_unlock(&lock);
        rt_thread_mdelay(1000);
    }
}

int main(int argc, char** argv)
{
    int i;
    rt_err_t result;

    // Make sure initialization of all cores are finished.
    rt_thread_mdelay(2000);

    rt_kprintf("Demo for SMP CPU binding\n");

    rt_spin_lock_init(&lock);

    for (i = 0; i < NUM_THREADS; i++) {
        snprintf(thread_names[i], sizeof(thread_names[i]), "Thread %u", i);
        thread_args[i] = i + 1;
        result = rt_thread_init(thread_structs + i,
                                thread_names[i],
                                thread_entry,
                                (void *)(thread_args + i),
                                thread_stacks + i,
                                sizeof(thread_stacks[i]),
                                THREAD_PRIORITY,
                                THREAD_TIMESLICE);

        rt_kprintf("Created thread %d\n", i);
        if (result != RT_EOK) {
            rt_kprintf("Failed to create thread %d: %d\n", i, result);
            return result;
        }
        if (i <= 2) {
            // First 3 threads bind to cpu core 0, 1, and 2 respectively.
            rt_size_t bind_cpu = i;
            result = rt_thread_control(thread_structs + i,
                                       RT_THREAD_CTRL_BIND_CPU,
                                       (void *)bind_cpu);
            if (result != RT_EOK) {
                rt_kprintf("Failed to bind thread %d: %d\n", i, result);
                return result;
            }
            rt_kprintf("Binded thread %d to CPU %d\n", i, bind_cpu);
        }
        // Otherwise the thread is unbinded.
    }

    /**
     * Memory Barrier: ensure all thread variables are pushed down to the
     * hardware cache lines before thread startup begins.
     */
    rt_hw_dsb();
    rt_hw_isb();

    for (i = 0; i < NUM_THREADS; i++) {
        rt_thread_startup(thread_structs + i);
    }

    return 0;
}
