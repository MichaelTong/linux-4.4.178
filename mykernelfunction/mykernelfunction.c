#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/kernel_stat.h>

int fs_read_stats_switch = 1;
EXPORT_SYMBOL(fs_read_stats_switch);

u64 debuginfo = 0;
EXPORT_SYMBOL(debuginfo);

inline int init_fs_read_stats(struct fs_read_stats **stats) {
    *stats = alloc_percpu(struct fs_read_stats);
    if (! *stats)
        return 0;
    return 1;
}

inline void fs_read_stats_set_all(struct fs_read_stats *stats, 
                                int value) {
    int i;
    for_each_possible_cpu(i)
        memset(per_cpu_ptr(stats, i), value, sizeof(struct fs_read_stats));
}


/*
asmlinkage void sys_changeIgnoreK(int newval){
  ignoreK = newval;
  printk("Current value of ignoreK: %d\n",ignoreK);
}

asmlinkage void sys_changeCmdBit(int newval){
  cmdBit = newval == 0?0:1;
  printk("Current value of cmdBit: %d\n", cmdBit);
}

asmlinkage void sys_changeIgnoreR(int newval){
  ignoreR = newval;
  printk("Current value of ignoreR: %d\n", ignoreR);
}

asmlinkage void sys_changeReadPolicy(int newval){
  readPolicy = newval;
  switch(readPolicy)
    {
    case 0:
      printk("Normal read\n");
      break;
    case 1:
      printk("Reactive read\n");
      break;
    case 2:
      printk("Proactive read\n");
      break;
    case 3:
      printk("Adaptive read\n");
      break;
    case 4:
      printk("EBUSY RAID\n");
      break;
    case 5:
      printk("GCT RAID\n");
      break;
    case 6:
      printk("Fake GCT\n");
      break;
    case 7:
      printk("Default 10ms\n");
      break;
    case 10:
      printk("GCT+PREEMPTION\n");
      break;
    default:
      printk("Unknown Policy\n");
      break;
    }
}


asmlinkage void sys_changeEnableCount(int newval){
  if(newval==1)
    {
      enableCount = 1;
      printk("Enable wait counter\n");
    }
  else
    {
      enableCount = 0;
      printk("Disable wait counter, previous value of counter is %d\n", waitCounter);
      waitCounter = 0;
    }
}

asmlinkage void sys_changeDebuginfo(unsigned long newval)
{
  debuginfo = newval;
  printk("Debug info now 0x%lX\n", debuginfo);
}

asmlinkage unsigned long long sys_retryattempted(int option)
{
  unsigned long long val = 0;
  switch (option)
    {
    case 1://return the value and clear
      val = retry_attempted;
      PRINTK_MIKET(-1, "Return retry_attempted: %llu", val);
    default://clear
      retry_attempted = 0;
      PRINTK_MIKET(7, "Reset retry_attempted");
      break;
    }
  return val;
}

asmlinkage unsigned long long sys_readforwriteio(int option)
{
  unsigned long long val = 0;
  switch (option)
    {
    case 1://return the value and clear
      val = readforwrite_io;
      PRINTK_MIKET(7, "Return readforwrite_io: %llu", val);
    default://clear
      readforwrite_io = 0;
      PRINTK_MIKET(7, "Reset readforwrite_io");
      break;
    }
  return val;
}

asmlinkage unsigned long long sys_readforwriteblockedio(int option)
{
  unsigned long long val = 0;
  switch (option)
    {
    case 1://return the value and clear
      val = readforwrite_blocked_io;
      PRINTK_MIKET(7, "Return readforwrite_blocked_io: %llu", val);
    default://clear
      readforwrite_blocked_io = 0;
      PRINTK_MIKET(7, "Reset readforwrite_blocked_io");
      break;
    }
  return val;
}
*/