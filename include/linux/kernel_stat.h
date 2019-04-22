#ifndef _LINUX_KERNEL_STAT_H
#define _LINUX_KERNEL_STAT_H

#include <linux/smp.h>
#include <linux/threads.h>
#include <linux/percpu.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/vtime.h>
#include <asm/irq.h>
#include <linux/cputime.h>

/*
 * 'kernel_stat.h' contains the definitions needed for doing
 * some kernel statistics (CPU usage, context switches ...),
 * used by rstatd/perfmeter
 */

enum cpu_usage_stat {
	CPUTIME_USER,
	CPUTIME_NICE,
	CPUTIME_SYSTEM,
	CPUTIME_SOFTIRQ,
	CPUTIME_IRQ,
	CPUTIME_IDLE,
	CPUTIME_IOWAIT,
	CPUTIME_STEAL,
	CPUTIME_GUEST,
	CPUTIME_GUEST_NICE,
	NR_STATS,
};

struct kernel_cpustat {
	u64 cpustat[NR_STATS];
};

struct kernel_stat {
	unsigned long irqs_sum;
	unsigned int softirqs[NR_SOFTIRQS];
};

DECLARE_PER_CPU(struct kernel_stat, kstat);
DECLARE_PER_CPU(struct kernel_cpustat, kernel_cpustat);

/* Must have preemption disabled for this to be meaningful. */
#define kstat_this_cpu this_cpu_ptr(&kstat)
#define kcpustat_this_cpu this_cpu_ptr(&kernel_cpustat)
#define kstat_cpu(cpu) per_cpu(kstat, cpu)
#define kcpustat_cpu(cpu) per_cpu(kernel_cpustat, cpu)

extern unsigned long long nr_context_switches(void);

extern unsigned int kstat_irqs_cpu(unsigned int irq, int cpu);
extern void kstat_incr_irq_this_cpu(unsigned int irq);

static inline void kstat_incr_softirqs_this_cpu(unsigned int irq)
{
	__this_cpu_inc(kstat.softirqs[irq]);
}

static inline unsigned int kstat_softirqs_cpu(unsigned int irq, int cpu)
{
       return kstat_cpu(cpu).softirqs[irq];
}

/*
 * Number of interrupts per specific IRQ source, since bootup
 */
extern unsigned int kstat_irqs(unsigned int irq);
extern unsigned int kstat_irqs_usr(unsigned int irq);

/*
 * Number of interrupts per cpu, since bootup
 */
static inline unsigned int kstat_cpu_irqs_sum(unsigned int cpu)
{
	return kstat_cpu(cpu).irqs_sum;
}

extern void account_user_time(struct task_struct *, cputime_t, cputime_t);
extern void account_system_time(struct task_struct *, int, cputime_t, cputime_t);
extern void account_steal_time(cputime_t);
extern void account_idle_time(cputime_t);

#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
static inline void account_process_tick(struct task_struct *tsk, int user)
{
	vtime_account_user(tsk);
}
#else
extern void account_process_tick(struct task_struct *, int user);
#endif

extern void account_steal_ticks(unsigned long ticks);
extern void account_idle_ticks(unsigned long ticks);

struct fs_read_stats {
    s64 time_vfs_read_verify;
    s64 time_vfs_read__vfs_read;
    s64 time_vfs_read_post;
    s64 cnt_vfs_read;
    s64 time_sync_read_pre;
    s64 junk;
    s64 time_sync_read_read_iter;
    
    s64 time_file_read_resched;
    s64 time_file_read_find_page;
    s64 time_file_read_sync_readahead;
    s64 time_file_read_async_readahead;
    s64 time_file_read_wait_on_page;
    s64 time_file_read_inode;
    s64 time_file_read_copy_page;
    s64 time_file_read_read_page;
    s64 time_file_read_alloc_cold;
    s64 time_file_read_add_page;
    s64 time_file_read_loop;

    s64 time_copy_fault_in;
    s64 time_copy_kmap_a;
    s64 time_copy_copy_a;
    s64 time_copy_kunmap_a;
    s64 time_copy_kmap;
    s64 time_copy_copy;
    s64 time_copy_kunmap;
};

#ifdef CONFIG_SMP
#define fs_read_stats_lock()    ({ rcu_read_lock(); get_cpu(); })

#define fs_read_stats_unlock()  do {put_cpu(); rcu_read_unlock(); } while (0)

extern inline int init_fs_read_stats(struct fs_read_stats **stats);

#define __fs_read_stats_add(cpu, cpu_stat, field, addnd) \
    (per_cpu_ptr(cpu_stat, (cpu))->field += (addnd))

extern inline void fs_read_stats_set_all(struct fs_read_stats *my_stats, 
                                        int value);

#define fs_read_stats_read(stats, field)            \
({                                                  \
    typeof(stats->field) res = 0;                   \
    int _cpu;                                       \
    for_each_possible_cpu(_cpu)                     \
        res += per_cpu_ptr(stats, _cpu)->field;     \
    res;                                            \
})


#define my_stats_add_value(cpu_stat, field, value)          \
    do {                                                    \
        if (fs_read_stats_switch) {                         \
            int cpu = fs_read_stats_lock();                 \
            __fs_read_stats_add(cpu_stat, field, value);    \
            fs_read_stats_unlock();                         \
        }                                                   \
    } while(0)


extern struct fs_read_stats __percpu *mystats;
extern int fs_read_stats_switch;
extern u64 debuginfo;
#endif

#endif /* _LINUX_KERNEL_STAT_H */
