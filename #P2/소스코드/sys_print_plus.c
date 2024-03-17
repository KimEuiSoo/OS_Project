#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

asmlinkage long sys_print_plus(int num1, int num2, long __user *user_res){
	long res = 0;
	res = (long)(num1-num2);

	if(copy_to_user(user_res, &res, sizeof(long))){
		return -EFAULT;
	}
	return 0;
}

SYSCALL_DEFINE3(print_plus, int, num1, int, num2, long __user *, user_res){
	return sys_print_plus(num1, num2, user_res);
}
