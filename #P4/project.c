#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_VIRTUAL_ADDRESS 1048575  // 2^20 - 1
#define MAX_PHYSICAL_MEMORY_SIZE 65536  // 64KB
#define MAX_FRAME_SIZE 4096  // 4KB
#define MAX_FRAMES (MAX_PHYSICAL_MEMORY_SIZE / MAX_FRAME_SIZE)
#define MAX_STRING_LENGTH 512

//Page 테이블
typedef struct {
    int valid;
    int frame;
} PageTableEntry;

PageTableEntry pageTable[MAX_VIRTUAL_ADDRESS + 1];

// Page 프레임
int pageFrames[MAX_FRAMES];

int maxFrames=0;
int nextFrameToReplace = 0;

int frameSize=0;

typedef enum {
    OPTIMAL,
    FIFO,
    LRU,
    SECOND_CHANCE
} PageReplacementAlgorithm;

void initializePageTable();
void initializePageFrames();
void simulate(PageReplacementAlgorithm algorithm, char *inputFileName);
int getPageNumber(int virtualAddress);
int isPageFault(int pageNumber);
int findVictimPageOptimal(int *referenceString, int currentIndex);
int findVictimPageLRU(int *referenceString, int currentIndex);
int findVictimPageSecondChance(int currentIndex);
int findVictimPageFifo();

int main() {
    int virtualAddressLength, frameSizeOption, memorySizeOption, algorithmOption, inputMethod;
    
    //시뮬레이터 메모리 설정
    printf("A. simulation에 사용할 가상주소 길이를 선택하시오 (1. 18bits   2. 19.bits   3. 20bits): ");
    scanf("%d", &virtualAddressLength);
    
    printf("B. simulation에 사용할 페이지(프레임)의 크기를 선택하시오 (1. 1KB   2. 2KB   3. 4KB): ");
    scanf("%d", &frameSizeOption);
    
    printf("C. simulation에 사용할 물리메모리의 크기를 선택하시오 (1. 32KB   2. 64KB): ");
    scanf("%d", &memorySizeOption);
    
    printf("D. simulation에 적용할 page Replacement 알고리즘을 선택하시오 (1. Optimal   2. FIFO   3. LRU   4. Second-Chance): ");
    scanf("%d", &algorithmOption);
    
    printf("E. 가상주소 스트링 입력방식을 선택하시오 (1. input.in   2. 기존 파일 사용): ");
    scanf("%d", &inputMethod);

    //프레임 크기 설정
    switch (frameSizeOption) {
        case 1:
            frameSize = 1024;  // 1KB
            break;
        case 2:
            frameSize = 2048;  // 2KB
            break;
        case 3:
            frameSize = 4096;  // 4KB
            break;
        default:
            printf("Invalid frame size option.\n");
            return 1;
    }

    //물리 주소 크기 설정
    int physicalMemorySize;
    switch (memorySizeOption) {
        case 1:
            physicalMemorySize = 32768;  // 32KB
            break;
        case 2:
            physicalMemorySize = 65536;  // 64KB
            break;
        default:
            printf("Invalid memory size option.\n");
            return 1;
    }

    //페이지 테이블 페이지 프레임 초기화
    initializePageTable();
    initializePageFrames();

    // 최대 프레임 구하기
    maxFrames = physicalMemorySize / frameSize;

    // input파일 입력
    switch (algorithmOption) {
        case 1:
            simulate(OPTIMAL, (inputMethod == 2) ? "input.in" : NULL);
            break;
        case 2:
            simulate(FIFO, (inputMethod == 2) ? "input.in" : NULL);
            break;
        case 3:
            simulate(LRU, (inputMethod == 2) ? "input.in" : NULL);
            break;
        case 4:
            simulate(SECOND_CHANCE, (inputMethod == 2) ? "input.in" : NULL);
            break;
        default:
            printf("Invalid algorithm option.\n");
            return 1;
    }

    return 0;
}

void initializePageTable() {
    for (int i = 0; i <= MAX_VIRTUAL_ADDRESS; i++) {
        pageTable[i].valid = 0;
        pageTable[i].frame = -1;
    }
}

void initializePageFrames() {
    for (int i = 0; i < MAX_FRAMES; i++) {
        pageFrames[i] = -1;
    }
}

//페이지 교체 알고리즘 실행하는 함수
void simulate(PageReplacementAlgorithm algorithm, char *inputFileName) {
    srand(time(NULL));
    //가상 주소담는 변수
    int referenceString[5000];
    if (inputFileName != NULL) {
        // Load from file
        FILE *file = fopen(inputFileName, "r");
        if (file == NULL) {
            perror("Error opening input file");
            exit(1);
        }

        int i = 0;
        //input.in에 있는 가상주소 변수에 담기
        while (fscanf(file, "%d", &referenceString[i]) != EOF) {
            i++;
        }

        fclose(file);
    } else {	//1번을 골랐으면 랜덤 난수 생성하여 담기
        
        for (int i = 0; i < 5000; i++) {
            referenceString[i] = rand() % (MAX_VIRTUAL_ADDRESS + 1);
        }
    }
    
    char outputFileName[20];	//outputFile 생성
    switch (algorithm) {
        case OPTIMAL:
            sprintf(outputFileName, "output.opt");
            break;
        case FIFO:
            sprintf(outputFileName, "output.fifo");
            break;
        case LRU:
            sprintf(outputFileName, "output.lru");
            break;
        case SECOND_CHANCE:
            sprintf(outputFileName, "output.sc");
            break;
    }
    
    //outputFile 쓰기로 열기
    FILE *outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        exit(1);
    }
    
    int pageFaults = 0;	//page Fault 갯수 세는 변수
    for (int i = 0; i < 5000; i++) {
    	//가상 주소 변수
        int virtualAddress = referenceString[i];
        //가상주소에 따른 pageNumber
        int pageNumber = getPageNumber(virtualAddress);
        //page fault 발생 시 F/H 변수 담기
        char faultString;

        if (isPageFault(pageNumber)) {
            pageFaults++;
            faultString='F';

            int victimPage = -1;
            switch (algorithm) {
                case OPTIMAL:
                    victimPage = findVictimPageOptimal(referenceString, i);
                    break;
                case LRU:
                    victimPage = findVictimPageLRU(referenceString, i);
                    break;
                case SECOND_CHANCE:
                    victimPage = findVictimPageSecondChance(i);
                    break;
                case FIFO:
                    victimPage = findVictimPageFifo();
                    break;
                default:
                    printf("error....\n");
                    break;
            }
            
            
            // 페이지 테이블과 페이지 프레임 업데이트 하기
            pageTable[victimPage].valid = 0;
            pageTable[victimPage].frame = -1;
            pageTable[pageNumber].valid = 1;
            pageTable[pageNumber].frame = victimPage;
            pageFrames[victimPage] = pageNumber;
        }
        else faultString = 'H';
        
        // 물리주소 변수
    	int physicalAddress = pageTable[pageNumber].frame * frameSize + virtualAddress % frameSize;
    	
        fprintf(outputFile, "V.A: %d, Page No: %d, Frame No: %d, P.A: %d, Page Fault: %c\n",
                virtualAddress, pageNumber, pageTable[pageNumber].frame, physicalAddress, faultString);
    }

    // 총 Page Fault 출력
    fprintf(outputFile, "Total Number of Page Faults: %d\n", pageFaults);

    fclose(outputFile);
}

int getPageNumber(int virtualAddress) {
    return virtualAddress / MAX_FRAME_SIZE;
}

int isPageFault(int pageNumber) {
    return !pageTable[pageNumber].valid;
}

//가장 먼저 메모리에 올라온 페이지 교체해주는 알고리즘
int findVictimPageFifo(){
    int victimPage = -1;
    
    victimPage = pageFrames[0];
    
    return victimPage;
}

//가장 오랫동안 사용하지 않을 페이지를 교체하는 알고리즘 함수
int findVictimPageOptimal(int *referenceString, int currentIndex) {
    int victimPage = -1;
    int farthestIndex = -1;
    
    for (int i = 0; i < MAX_FRAMES; i++) {
        int nextPageIndex = currentIndex + 1;
        while (nextPageIndex < 5000) {
            // 다음 참조된 페이지의 인덱스가 현재 페이지 프레임과 같은지 확인한다.
            if (getPageNumber(referenceString[nextPageIndex]) == pageFrames[i]) {
            	//다음 참조된 페이지의 인덱스가 현재까지의 최대 인덱스보다 크면 해당 페이지를 가장 먼저 참조하는 페이지로 선택한다.
                if (nextPageIndex > farthestIndex) {
                    farthestIndex = nextPageIndex;
                    victimPage = i;	//가장 먼저 참조하는 페이지 인덱스를 기억한다.
                }
                break;
            }
            nextPageIndex++;
        }

        if (nextPageIndex == 5000) {
            victimPage = i;
            break;
        }
    }

    return victimPage;
}

//사용하지 않는 페이지를 교체하는 알고리즘 함수
int findVictimPageLRU(int *referenceString, int currentIndex) {
    int victimPage = -1;
    int leastRecentlyUsedIndex = currentIndex;

    for (int i = 0; i < MAX_FRAMES; i++) {
        int recentIndex = currentIndex - 1;
        while (recentIndex >= 0) {
            // 이전 참조된 페이지의 인덱스가 현제 페이지 프레임과 같은지 확인한다.
            if (getPageNumber(referenceString[recentIndex]) == pageFrames[i]) {
            	// 이전 참조된 페이지의 인덱스가 현재까지의 최소 인덱스보다 작으면 해당 페이지를 가장 최근에 참조한 페이지로 선택한다.
                if (recentIndex < leastRecentlyUsedIndex) {
                    leastRecentlyUsedIndex = recentIndex;
                    victimPage = i;		//가장 최근에 참조한 페이지 인덱스 기억한다.
                }
                break;
            }
            recentIndex--;
        }

        if (recentIndex == -1) {
            victimPage = i;
            break;
        }
    }

    return victimPage;
}

//미구현
int findVictimPageSecondChance(int currentIndex) {

    int victimPage = -1;

    return victimPage;
}
