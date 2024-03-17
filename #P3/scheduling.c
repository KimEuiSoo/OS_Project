#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <linux/sched.h>
#include <sys/syscall.h>

//총 크기
#define MAX_SIZE 100
//프로세스 갯수
#define PROCESS_COUNT 21

int cfs_nice = 0;
int rt_rr = 0;

struct sched_attr{
	uint32_t size;
	uint32_t sched_policy;
	uint64_t sched_flags;
	int32_t sched_nice;

	uint32_t sched_priority;

	uint64_t sched_runtime;
	uint64_t sched_deadline;
	uint64_t sched_period;
};

//자식 프로세스들이 수행하는 함수
void CFS_default(){
	//배열 곱셈 작업을 수행하기 위한 배열
	int A[MAX_SIZE][MAX_SIZE];
	int B[MAX_SIZE][MAX_SIZE];
	int result[MAX_SIZE][MAX_SIZE];

	//배열 곱셈 작업을 수행하기 위한 변수
	int k, j, i, count=0;

	//배열 곱셈
	while(count < MAX_SIZE){
		for(k=0; k<MAX_SIZE; k++){
			for(j=0; j<MAX_SIZE; j++){
				for(i=0; i<MAX_SIZE; i++){
					result[k][j] += A[k][j] * B[i][j];
				}
			}
		}
		count++;
	}
}

static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags){
	return syscall(SYS_sched_setattr, pid, attr, flags);
}

//우선순위 적용 시켜주는 함수
void setNice(int nice){
	struct sched_attr attr;
	//attr 초기화
	memset(&attr, 0, sizeof(attr));

	attr.size = sizeof(attr);
	//SCHED_NORMAL로 설정
	attr.sched_policy = SCHED_NORMAL;
	attr.sched_priority = 0;
	//nice값 부여
	attr.sched_nice = nice;

	//해당 시스템 콜을 호출하는 함수 호출
	sched_setattr(0, &attr, 0);
}

//프로세스 스케줄링 하는 함수
void CFS_process_scheduling(int num){	
	int pid[PROCESS_COUNT];
	int fds[PROCESS_COUNT][2];
	//시작 시간, 종료 시간 계산하기 위한 변수 배열
	struct timeval start_time[PROCESS_COUNT], end_time[PROCESS_COUNT];

	double total_elapsed_time = 0.0;
	
	//파이프 생성
	for(int i=0; i<PROCESS_COUNT; i++){
		if(pipe(fds[i]) == -1){
			perror("error");
			exit(1);
		}
	}	

	//프로세스 21개를 생성해서 각 조건에 맞춰서 스케줄링하기
	for(int i=0; i<PROCESS_COUNT; i++){
		pid[i] = fork();	//자식 프로세스 생성
		int nice_val=0;
		if(pid[i] < 0){
			perror("fork failed");
			exit(1);
		}
		else if(pid[i]==0){
			if(cfs_nice){	//CFS_NICE를 선택했을때
				if(i<7){
					//nice 값을 설정
					nice_val=19;
					setNice(nice_val);
				}
				else if(i<14){
					//nice 값을 설정
					nice_val=0;
					setNice(nice_val);
				}
				else if(i<21){
					//nice 값을 설정
					nice_val=-20;
					setNice(nice_val);
				}
			}
			else setNice(0);	//CFS_DEFAULT일때 nice값이 0으로 설정

			//시작시간을 start_time에 넣어준다.
			gettimeofday(&start_time[i], NULL);
			CFS_default();
			//종료시간을 end_time에 넣어준다
			gettimeofday(&end_time[i], NULL);

			//실행 시간을 받는 변수
			double elapsed_time = (end_time[i].tv_sec - start_time[i].tv_sec) + (end_time[i].tv_usec - start_time[i].tv_usec)/1000000.0;
			
			//시작시간 HH:MM:SS를 format해주기 위한 변수
			char start_time_s[20];
			//start_time_s에 포멧된 시간을 저장
			strftime(start_time_s, 20, "%T", localtime(&start_time[i].tv_sec));
			//시작시간 ms를 저장하는 변수
			char sms[5];
			//시작시간을 .xxx초로 받아서 저장해줌
			snprintf(sms, sizeof(sms), ".%03ld", start_time[i].tv_usec/1000);
			//두 문자열을 합친다.
			strcat(start_time_s, sms);
			
			//시작시간과 마찬가지로 종료시간도 포멧 절차를 진행한다.
			char end_time_s[20];
			strftime(end_time_s, 20, "%T", localtime(&end_time[i].tv_sec));
			char ems[5];
			snprintf(ems, sizeof(ems), ".%03ld", end_time[i].tv_usec/1000);
			strcat(end_time_s, ems);
			
			//해당 elasped_time을 배열에다가 pipe해줘서 부모가 해당 프로세스에 elapsed_time값을 얻을 수 있게 써주는 작업
			write(fds[i][1], &elapsed_time, sizeof(elapsed_time));
			//해당 fds[i][1]은 종료한다.
			close(fds[i][1]);

			printf("PID: %d | NICE: %d | Start Time: %s | End Time: %s | Elapsed Time: %lf\n", getpid(), nice_val, start_time_s, end_time_s, elapsed_time);
			exit(0);
		}
	}
	
	//total_elapsed_time 구하기
	for(int i=0; i<PROCESS_COUNT; i++){
		double elapsed_time;
		//fds에 있는 elapsed_time을 읽어준다.
		read(fds[i][0], &elapsed_time, sizeof(elapsed_time));
		total_elapsed_time += elapsed_time;
		close(fds[i][0]);
	}
	//자식 프로세스 실행이 종료 될때 까지 wait해주는 동작
	for(int i=0; i<PROCESS_COUNT; i++){
		int status;
		wait(&status);
	}

	//평균 시간 구하기
	double average_elapsed_time = total_elapsed_time / PROCESS_COUNT;
	printf("Scheduling Policy: %s | Average Elapsed Time: %lf\n", cfs_nice ? "CFS_NICE":"CFS_DEFAULT",average_elapsed_time);
}

void Policy(){
	struct sched_attr attr;
	//attr 초기화 설정
	memset(&attr, 0, sizeof(attr));

	attr.size = sizeof(attr);
	if(rt_rr){
		attr.sched_policy = SCHED_RR;	//policy SCHED_RR로 설정
	}
	else attr.sched_policy = SCHED_FIFO;	//policy SCHED_FIFO로 설정
	attr.sched_priority = 1;

	//시스템콜 SYS_sched_setattr 호출
	if(syscall(__NR_sched_setattr, 0, &attr, 0) == -1){
		perror("error");
		exit(1);
	}
}

void RT_process_scheduling(int num){
	int pid[PROCESS_COUNT];
	int fds[PROCESS_COUNT][2];
	//시작 시간, 종료 시간 계산하기 위한 변수 배열
	struct timeval start_time[PROCESS_COUNT], end_time[PROCESS_COUNT];	

	double total_elapsed_time = 0.0;
	
	for(int i=0; i<PROCESS_COUNT; i++){
		if(pipe(fds[i]) == -1){
			perror("error");
			exit(1);
		}
	}	
	
	Policy();
	
	//프로세스 21개를 생성해서 각 조건에 맞춰서 스케줄링하기
	for(int i=0; i<PROCESS_COUNT; i++){
		pid[i] = fork();
		int nice_val=0;
		if(pid[i] < 0){
			perror("fork failed");
			exit(1);
		}
		else if(pid[i]==0){			//CFS와 동일하게 진행
			gettimeofday(&start_time[i], NULL);
			CFS_default();
			gettimeofday(&end_time[i], NULL);

			double elapsed_time = (end_time[i].tv_sec - start_time[i].tv_sec) + (end_time[i].tv_usec - start_time[i].tv_usec)/1000000.0;
			
			char start_time_s[20];
			strftime(start_time_s, 20, "%T", localtime(&start_time[i].tv_sec));
			char sms[5];
			snprintf(sms, sizeof(sms), ".%03ld", start_time[i].tv_usec/1000);
			strcat(start_time_s, sms);
			
			char end_time_s[20];
			strftime(end_time_s, 20, "%T", localtime(&end_time[i].tv_sec));
			char ems[5];
			snprintf(ems, sizeof(ems), ".%03ld", end_time[i].tv_usec/1000);
			strcat(end_time_s, ems);
			
			write(fds[i][1], &elapsed_time, sizeof(elapsed_time));
			close(fds[i][1]);

			printf("PID: %d | Start Time: %s | End Time: %s | Elapsed Time: %lf\n", getpid(), start_time_s, end_time_s, elapsed_time);
			exit(0);
		}
	}
	
	//total_elapsed_time 구하기
	for(int i=0; i<PROCESS_COUNT; i++){
		double elapsed_time;
		read(fds[i][0], &elapsed_time, sizeof(elapsed_time));
		total_elapsed_time += elapsed_time;
		close(fds[i][0]);
	}

	//자식 프로세스 실행이 종료 될때 까지 wait해주는 동작
	for(int i=0; i<PROCESS_COUNT; i++){
		int status;
		wait(&status);
	}
	
	char buf[100];
	char combined_buf[120];
	if(rt_rr){
		FILE *file;
		///proc/sys/kernel/sched_rr_timeslice_ms 읽기 전용으로 연다
		file = fopen("/proc/sys/kernel/sched_rr_timeslice_ms","r");
		if(file == NULL){
			perror("error");
			exit(1);
		}
		if(fgets(buf, sizeof(buf), file)==NULL){	//buf에 file에 있는 time slice 값을 넣는다.
			perror("error");
			fclose(file);
			exit(1);
		}
		
		fclose(file);
		
		size_t len = strcspn(buf, "\n");	//줄바꿈 위치의 길이를 찾는다.
		if(len < sizeof(buf))
			buf[len] = '\0';//줄바꿈을 NULL값으로 변경
		//Time Quantum: {time slice}값을 출력하게 끔 하기 위해 합쳐서 combined_buf에 저장
		snprintf(combined_buf, sizeof(combined_buf), " Time Quantum: %s |", buf);
	}
	
	//평균 시간 구하기
	double average_elapsed_time = total_elapsed_time / PROCESS_COUNT;
	printf("Scheduling Policy: %s |%s Average Elapsed Time: %lf\n", rt_rr? "RT_RR" : "RT_FIFO", rt_rr ? combined_buf : "", average_elapsed_time);
}


int main(void){	
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);
	while(1){
		int num=0;
		cfs_nice = 0;
		rt_rr = 0;

		printf("Input the Scheduling Polity to apply: \n");
		printf("1.  CFS_DEFAULT\n");
		printf("2.  CFS_NICE\n");
		printf("3.  RT_FIFO\n");
		printf("4.  RT_RR\n");
		printf("0.  Exit\n");

		scanf("%d", &num);

		switch(num){
			case 1: 
				CFS_process_scheduling(num);
				break;
			case 2:
				cfs_nice = 1;
				CFS_process_scheduling(num);
				break;
			case 3:
				RT_process_scheduling(num);
				break;
			case 4:
				rt_rr=1;
				RT_process_scheduling(num);
				break;
			case 0:
				return 0; 
				break;
			default:
				break;
		}
	}
}
