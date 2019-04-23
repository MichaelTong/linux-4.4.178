#ifndef _ASM_X86_UACCESS_64_H
#define _ASM_X86_UACCESS_64_H

/*
 * User space memory access functions
 */
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/lockdep.h>
#include <asm/alternative.h>
#include <asm/cpufeatures.h>
#include <asm/page.h>
#include <linux/ktime.h>
#include <linux/smp.h>
#include <linux/rcupdate.h>

extern struct fs_read_stats __percpu *mystats;

#define fs_read_stats_lock()    ({ rcu_read_lock(); get_cpu(); })

#define fs_read_stats_unlock()  do {put_cpu(); rcu_read_unlock(); } while (0)

#define __fs_read_stats_add(cpu, cpu_stat, field, addnd) \
    (per_cpu_ptr(cpu_stat, (cpu))->field += (addnd))
/*
 * Copy To/From Userspace
 */

/* Handles exceptions in both to and from, but doesn't do access_ok */
__must_check unsigned long
copy_user_enhanced_fast_string(void *to, const void *from, unsigned len);
__must_check unsigned long
copy_user_generic_string(void *to, const void *from, unsigned len);
__must_check unsigned long
copy_user_generic_unrolled(void *to, const void *from, unsigned len);

static __always_inline __must_check unsigned long
copy_user_generic(void *to, const void *from, unsigned len)
{
	unsigned ret;

	/*
	 * If CPU has ERMS feature, use copy_user_enhanced_fast_string.
	 * Otherwise, if CPU has rep_good feature, use copy_user_generic_string.
	 * Otherwise, use copy_user_generic_unrolled.
	 */
	alternative_call_2(copy_user_generic_unrolled,
			 copy_user_generic_string,
			 X86_FEATURE_REP_GOOD,
			 copy_user_enhanced_fast_string,
			 X86_FEATURE_ERMS,
			 ASM_OUTPUT2("=a" (ret), "=D" (to), "=S" (from),
				     "=d" (len)),
			 "1" (to), "2" (from), "3" (len)
			 : "memory", "rcx", "r8", "r9", "r10", "r11");
	return ret;
}

__must_check unsigned long
copy_in_user(void __user *to, const void __user *from, unsigned len);

static __always_inline __must_check
int __copy_from_user_nocheck(void *dst, const void __user *src, unsigned size)
{
	int ret = 0;

	if (!__builtin_constant_p(size))
		return copy_user_generic(dst, (__force void *)src, size);
	switch (size) {
	case 1:
		__uaccess_begin_nospec();
		__get_user_asm(*(u8 *)dst, (u8 __user *)src,
			      ret, "b", "b", "=q", 1);
		__uaccess_end();
		return ret;
	case 2:
		__uaccess_begin_nospec();
		__get_user_asm(*(u16 *)dst, (u16 __user *)src,
			      ret, "w", "w", "=r", 2);
		__uaccess_end();
		return ret;
	case 4:
		__uaccess_begin_nospec();
		__get_user_asm(*(u32 *)dst, (u32 __user *)src,
			      ret, "l", "k", "=r", 4);
		__uaccess_end();
		return ret;
	case 8:
		__uaccess_begin_nospec();
		__get_user_asm(*(u64 *)dst, (u64 __user *)src,
			      ret, "q", "", "=r", 8);
		__uaccess_end();
		return ret;
	case 10:
		__uaccess_begin_nospec();
		__get_user_asm(*(u64 *)dst, (u64 __user *)src,
			       ret, "q", "", "=r", 10);
		if (likely(!ret))
			__get_user_asm(*(u16 *)(8 + (char *)dst),
				       (u16 __user *)(8 + (char __user *)src),
				       ret, "w", "w", "=r", 2);
		__uaccess_end();
		return ret;
	case 16:
		__uaccess_begin_nospec();
		__get_user_asm(*(u64 *)dst, (u64 __user *)src,
			       ret, "q", "", "=r", 16);
		if (likely(!ret))
			__get_user_asm(*(u64 *)(8 + (char *)dst),
				       (u64 __user *)(8 + (char __user *)src),
				       ret, "q", "", "=r", 8);
		__uaccess_end();
		return ret;
	default:
		return copy_user_generic(dst, (__force void *)src, size);
	}
}

static __always_inline __must_check
int __copy_from_user(void *dst, const void __user *src, unsigned size)
{
	might_fault();
	return __copy_from_user_nocheck(dst, src, size);
}

static __always_inline __must_check
int __copy_to_user_nocheck(void __user *dst, const void *src, unsigned size)
{
	int ret = 0;

	if (!__builtin_constant_p(size))
		return copy_user_generic((__force void *)dst, src, size);
	switch (size) {
	case 1:
		__uaccess_begin();
		__put_user_asm(*(u8 *)src, (u8 __user *)dst,
			      ret, "b", "b", "iq", 1);
		__uaccess_end();
		return ret;
	case 2:
		__uaccess_begin();
		__put_user_asm(*(u16 *)src, (u16 __user *)dst,
			      ret, "w", "w", "ir", 2);
		__uaccess_end();
		return ret;
	case 4:
		__uaccess_begin();
		__put_user_asm(*(u32 *)src, (u32 __user *)dst,
			      ret, "l", "k", "ir", 4);
		__uaccess_end();
		return ret;
	case 8:
		__uaccess_begin();
		__put_user_asm(*(u64 *)src, (u64 __user *)dst,
			      ret, "q", "", "er", 8);
		__uaccess_end();
		return ret;
	case 10:
		__uaccess_begin();
		__put_user_asm(*(u64 *)src, (u64 __user *)dst,
			       ret, "q", "", "er", 10);
		if (likely(!ret)) {
			asm("":::"memory");
			__put_user_asm(4[(u16 *)src], 4 + (u16 __user *)dst,
				       ret, "w", "w", "ir", 2);
		}
		__uaccess_end();
		return ret;
	case 16:
		__uaccess_begin();
		__put_user_asm(*(u64 *)src, (u64 __user *)dst,
			       ret, "q", "", "er", 16);
		if (likely(!ret)) {
			asm("":::"memory");
			__put_user_asm(1[(u64 *)src], 1 + (u64 __user *)dst,
				       ret, "q", "", "er", 8);
		}
		__uaccess_end();
		return ret;
	default:
		return copy_user_generic((__force void *)dst, src, size);
	}
}

static __always_inline __must_check
int __copy_to_user_nocheck_miket(void __user *dst, const void *src, unsigned size, int t)
{
	int ret = 0;
    ktime_t t1, t2;
    s64 lat;
    int cpu;
    t1 = ktime_get_boottime();
	if (!__builtin_constant_p(size)) {
        ret = copy_user_generic((__force void *)dst, src, size);
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_touser_generic, lat);
            fs_read_stats_unlock();
        }
        return ret;
    }
	switch (size) {
	case 1:
		__uaccess_begin();
		__put_user_asm(*(u8 *)src, (u8 __user *)dst,
			      ret, "b", "b", "iq", 1);
		__uaccess_end();
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_tosuer_asm, lat);
            fs_read_stats_unlock();
        }
		return ret;
	case 2:
		__uaccess_begin();
		__put_user_asm(*(u16 *)src, (u16 __user *)dst,
			      ret, "w", "w", "ir", 2);
		__uaccess_end();
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_tosuer_asm, lat);
            fs_read_stats_unlock();
        }
		return ret;
	case 4:
		__uaccess_begin();
		__put_user_asm(*(u32 *)src, (u32 __user *)dst,
			      ret, "l", "k", "ir", 4);
		__uaccess_end();
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_tosuer_asm, lat);
            fs_read_stats_unlock();
        }
		return ret;
	case 8:
		__uaccess_begin();
		__put_user_asm(*(u64 *)src, (u64 __user *)dst,
			      ret, "q", "", "er", 8);
		__uaccess_end();
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_tosuer_asm, lat);
            fs_read_stats_unlock();
        }
		return ret;
	case 10:
		__uaccess_begin();
		__put_user_asm(*(u64 *)src, (u64 __user *)dst,
			       ret, "q", "", "er", 10);
		if (likely(!ret)) {
			asm("":::"memory");
			__put_user_asm(4[(u16 *)src], 4 + (u16 __user *)dst,
				       ret, "w", "w", "ir", 2);
		}
		__uaccess_end();
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_tosuer_asm, lat);
            fs_read_stats_unlock();
        }
		return ret;
	case 16:
		__uaccess_begin();
		__put_user_asm(*(u64 *)src, (u64 __user *)dst,
			       ret, "q", "", "er", 16);
		if (likely(!ret)) {
			asm("":::"memory");
			__put_user_asm(1[(u64 *)src], 1 + (u64 __user *)dst,
				       ret, "q", "", "er", 8);
		}
		__uaccess_end();
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_tosuer_asm, lat);
            fs_read_stats_unlock();
        }
		return ret;
	default:
        ret = copy_user_generic((__force void *)dst, src, size);
        t2 = ktime_get_boottime();
        if (t == 1) {
            lat = ktime_to_ns(ktime_sub(t2, t1));
            cpu = fs_read_stats_lock();
            __fs_read_stats_add(cpu, mystats, time_touser_generic, lat);
            fs_read_stats_unlock();
        }
		return ret;
	}
}

static __always_inline __must_check
int __copy_to_user(void __user *dst, const void *src, unsigned size)
{
	might_fault();
	return __copy_to_user_nocheck(dst, src, size);
}

static __always_inline __must_check
int __copy_in_user(void __user *dst, const void __user *src, unsigned size)
{
	int ret = 0;

	might_fault();
	if (!__builtin_constant_p(size))
		return copy_user_generic((__force void *)dst,
					 (__force void *)src, size);
	switch (size) {
	case 1: {
		u8 tmp;
		__uaccess_begin_nospec();
		__get_user_asm(tmp, (u8 __user *)src,
			       ret, "b", "b", "=q", 1);
		if (likely(!ret))
			__put_user_asm(tmp, (u8 __user *)dst,
				       ret, "b", "b", "iq", 1);
		__uaccess_end();
		return ret;
	}
	case 2: {
		u16 tmp;
		__uaccess_begin_nospec();
		__get_user_asm(tmp, (u16 __user *)src,
			       ret, "w", "w", "=r", 2);
		if (likely(!ret))
			__put_user_asm(tmp, (u16 __user *)dst,
				       ret, "w", "w", "ir", 2);
		__uaccess_end();
		return ret;
	}

	case 4: {
		u32 tmp;
		__uaccess_begin_nospec();
		__get_user_asm(tmp, (u32 __user *)src,
			       ret, "l", "k", "=r", 4);
		if (likely(!ret))
			__put_user_asm(tmp, (u32 __user *)dst,
				       ret, "l", "k", "ir", 4);
		__uaccess_end();
		return ret;
	}
	case 8: {
		u64 tmp;
		__uaccess_begin_nospec();
		__get_user_asm(tmp, (u64 __user *)src,
			       ret, "q", "", "=r", 8);
		if (likely(!ret))
			__put_user_asm(tmp, (u64 __user *)dst,
				       ret, "q", "", "er", 8);
		__uaccess_end();
		return ret;
	}
	default:
		return copy_user_generic((__force void *)dst,
					 (__force void *)src, size);
	}
}

static __must_check __always_inline int
__copy_from_user_inatomic(void *dst, const void __user *src, unsigned size)
{
	return __copy_from_user_nocheck(dst, src, size);
}

static __must_check __always_inline int
__copy_to_user_inatomic(void __user *dst, const void *src, unsigned size)
{
	return __copy_to_user_nocheck(dst, src, size);
}

static __must_check __always_inline int
__copy_to_user_inatomic_miket(void __user *dst, const void *src, unsigned size, int t)
{
	return __copy_to_user_nocheck_miket(dst, src, size, t);
}

extern long __copy_user_nocache(void *dst, const void __user *src,
				unsigned size, int zerorest);

static inline int
__copy_from_user_nocache(void *dst, const void __user *src, unsigned size)
{
	might_fault();
	return __copy_user_nocache(dst, src, size, 1);
}

static inline int
__copy_from_user_inatomic_nocache(void *dst, const void __user *src,
				  unsigned size)
{
	return __copy_user_nocache(dst, src, size, 0);
}

unsigned long
copy_user_handle_tail(char *to, char *from, unsigned len);

#endif /* _ASM_X86_UACCESS_64_H */
