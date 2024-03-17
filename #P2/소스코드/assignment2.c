#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <limits.h>
#include <unistd.h>

int main(void){
	while(1){
		char calculate = '\0';
		int isCheck=0;				//Wrong Input을 띄우기 위한 변수
		int num1 = INT_MIN;
		int num2 = INT_MIN;
		char input[128]={'\0'};
		char c = '\0';
		printf("Input:   ");
		//fgets로 input에 데이터 입력받기
		if(fgets(input, sizeof(input), stdin)==NULL){
			fprintf(stderr, "error");
		}
		//input의 총 길이
		int len = sizeof(input);
		//현재 커서가 위치한 곳
		int curlen = 0;
		for(curlen=0; curlen<len; curlen++){
			c = input[curlen];
			//문자 부호가 + -면 calculate에 부호 저장 후 for문 탈출
			if(c=='+' || c=='-'){
				calculate = c;
				break;
			}
			//숫자를 num1을 받음
			else if( c >= '0' && c <= '9'){
				num1*=10;
				num1 += c - '0';
			}
			//문자가 널이거나 \n이면 break
			else if(c=='\0' || c=='\n') break;
			//부호 +, -와 숫자가 아닌 다른 문자면 오류 출력
			else isCheck++;
		}
		//아무입력 없이 엔터를 입력받으면 해당 프로그램 종료
		if(num1==INT_MIN && calculate=='\0') return 0;
		//입력 오류 확인 후 입력오류면 출력
		else if(isCheck>0){ 
			printf("Wrong Input!\n");
			continue;
		}
		//calculate에 부호가 없고 num1에 숫자만 있으면 1)번 시스템 콜 호출
		else if(num1!=INT_MIN && calculate=='\0'){
			char res[32];
			long value = syscall(449, num1, res);
			printf("Output:  %s\n", res);
		}
		else{
			//위와 같이 num2를 걸러내는 함수
			for(curlen=curlen+1;curlen<len; curlen++){
				c = input[curlen];
				if( c >= '0' && c <= '9'){
					num2*=10;
					num2 += c - '0';
				}
				else if(c=='\0')
					break;
				else if(c=='\0' || c=='\n') break;
				else isCheck++;
			}
			if(num2==INT_MIN) 
			{
				printf("Wrong Input!\n");
				continue;
			}
			else if(isCheck>0){ 
				printf("Wrong Input!\n");
				continue;
			}
			else{
				long value = 0;
				long res = 0;
				//calculate가 +부호면 2)에 sys_print_plus시스템 콜 호출
				if(calculate=='+') value = syscall(450, num1, num2, &res);
				////calculate가 -부호면 2)에 sys_print_minus시스템 콜 호출
				else value = syscall(451, num1, num2, &res);
				printf("Output:  %ld\n", res);
			}
		}
	}
	return 0;
}
