#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

asmlinkage long sys_print_reverse(int num, char __user *user_res)
{
	char *res;
	res = kmalloc(32, GFP_KERNEL);
	
	int length = 0;
	if(res == NULL){
		return -ENOMEM;
	}

	while(num > 0){
		res[length++] = num%10 + '0';
		num/=10;
	}

	if(copy_to_user(user_res, res, length+1)){
		kfree(res);
		return -EFAULT;
	}
	
	kfree(res);
	return 0;
}

SYSCALL_DEFINE2(print_reverse, int, num, char __user *, user_res)
{
	return sys_print_reverse(num, user_res);
}
