#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define COLUMN_CNT 12

int main() {
	DIR *proc_dir;
	struct dirent *entry;
	char line[1000];

	printf("%-8s %-8s %-8s %s\n", "PID", "TTY", "TIME", "CMD");

	// /proc 디렉토리 열기
	proc_dir = opendir("/proc");
	// /proc 디렉토리를 열지 못하면 오류
	if(proc_dir == NULL){
		perror("Error opening /proc directory");
		exit(EXIT_FAILURE);
	}

	// /proc 디렉토리 내의 각 프로세스 디렉토리를 순회합니다.
	while((entry = readdir(proc_dir)) != NULL){
		int is_number = 1;
		// 해당 디렉토리 이름이 숫자로만 구성되어 있는지 판별합니다.
		for(int i = 0; entry->d_name[i] != '\0'; i++){
			if(!isdigit(entry->d_name[i])){
				is_number = 0;
				break;
			}
		}

		//숫자로만 이루어 졌으면 해당 데이터들을 조사
		if(is_number){

			// 현재 프로세스의 TTY확인(pts/1형식)으로
			char my_tty[16];
			char tty_link[64];
			// /proc/PID/fd/0 심블릭 링크를 읽어옵니다.
			snprintf(tty_link, sizeof(tty_link), "/proc/%d/fd/0", getpid());
			//읽은 링크를 my_tty에 저장합니다.
			ssize_t len = readlink(tty_link, my_tty, sizeof(my_tty) - 1);
			if(len > 0){
				my_tty[len] = '\0';
			}else{
				strcpy(my_tty, "pts/1");
			}


			char proc_path[512];
			char data_path[512];
			char cmdline[256];
			char stat[256];
			char stat_fields[64][256];

			//해당 PID 데이터을 이용해서 /proc에 self데이터 주소를 저장
			snprintf(proc_path, sizeof(proc_path), "/proc/%s", entry->d_name);
			strcpy(data_path, proc_path);

			// /proc/PID/cmdline를 읽기 전용으로 연다
			FILE *cmdline_file = fopen(strcat(data_path, "/cmdline"), "r");
			if(cmdline_file != NULL){
				//fget으로 명령어에 cmd정보를 가져옵니다.
				fgets(cmdline, sizeof(cmdline), cmdline_file);
				fclose(cmdline_file);

				// /proc/PID/stat에 파일을 읽기 전용으로 연다.
				FILE *stat_file = fopen(strcat(proc_path, "/stat"),"r");
				if(stat_file != NULL){
					//fgets 함수를 이용해서 stat의 정보를 읽어온다.
					fgets(stat, sizeof(stat), stat_file);
					fclose(stat_file);

					char *saveptr;
					char *token = strtok_r(stat, " ", &saveptr);
					int i=0;

					//각 stat값을 " "로 구별해줘서 자르고 stat_fields에 각각 넣어준다.
					while(token!=NULL){
						strcpy(stat_fields[i], token);
						token = strtok_r(NULL, " ", &saveptr);
						i++;
					}
					//총 시간 계산
					unsigned long total_time = strtoul(stat_fields[13], NULL, 10)+strtoul(stat_fields[14], NULL, 10);
					

					if(strcmp(stat_fields[6], "0") != 0&&(strstr(cmdline, "ps") != NULL || strcmp(cmdline, "bash")==0)){
						printf("%-8s %-8s %02lu:%02lu:%02lu %s\n", entry->d_name, &my_tty[5], total_time/3600, (total_time%3600)/60, total_time%60, cmdline);
					}
				}
			}
		}
	}

	closedir(proc_dir);
	return 0;
}
