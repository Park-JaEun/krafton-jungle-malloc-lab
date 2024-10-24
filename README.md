#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

*******************************
구현 로직
*******************************
세그리게이티드 리스트 기반 메모리 할당자 구현
*******************************
## 세그리게이티드 리스트(Segregated List) 방식을 사용한 메모리 할당자를 구현. 
### 각 크기 클래스별로 독립적인 가용 블록 리스트를 관리해 메모리 할당 및 해제 성능을 최적화.

## 1. 메모리 구조
### 블록 구조
각 블록은 헤더(header)와 풋터(footer)를 포함
헤더/풋터: 블록 크기와 할당 여부 정보 저장 (4바이트)
가용 블록: 이전/다음 가용 블록 포인터를 포함 (각 8바이트)

## 2. 세그리게이티드 리스트 구현
### 크기 클래스 분류
2의 거듭제곱 기준으로 크기 클래스 구분
각 클래스는 독립적인 가용 블록 리스트 관리
크기 순서로 정렬된 리스트 유지

## 3. 주요 기능
### 초기화 (mm_init)
초기 힙 공간 할당
프롤로그, 에필로그 블록 설정
세그리게이티드 리스트 배열 초기화

### 메모리 할당 (mm_malloc)
요청 크기를 정렬 요구사항에 맞게 조정
적절한 크기 클래스에서 가용 블록 검색
적합한 블록을 찾으면 분할하여 할당
적합한 블록이 없으면 힙 확장

### 메모리 해제 (mm_free)
블록을 가용 상태로 표시
적절한 크기 클래스의 리스트에 삽입
인접 가용 블록과 병합

### 크기 재조정 (mm_realloc)
새로운 크기로 블록 할당
기존 데이터 복사
기존 블록 해제

## 4. 성능 최적화 전략
### 즉시 병합(Immediate Coalescing)
해제 시 인접 가용 블록과 즉시 병합하여 외부 단편화 감소

### 최적 맞춤(Best Fit) 검색
세그리게이티드 리스트로 검색 범위 최소화
크기별로 정렬된 리스트로 최적 블록 빠르게 찾기

### 블록 분할(Splitting)
할당 시 필요한 크기만큼만 사용
남은 공간을 새로운 가용 블록으로 분할

## 구현 세부사항
### 데이터 구조
cCopytypedef struct block {
    size_t header;        // 블록 크기 + 할당 비트
    struct block *next;   // 다음 가용 블록 포인터
    struct block *prev;   // 이전 가용 블록 포인터
    size_t footer;        // 블록 크기 + 할당 비트 (헤더와 동일)
} block_t;

### 주요 매크로
cCopy#define WSIZE       4    // 워드 크기 (4바이트)
#define DSIZE       8    // 더블 워드 크기 (8바이트)
#define CHUNKSIZE  (1<<12)  // 힙 확장 크기 (4096바이트)
#define CLASSES    20    // 크기 클래스 개수

## 성능 분석
### 시간 복잡도
할당(malloc): O(1) ~ O(n), 평균 O(1)
해제(free): O(1)
재할당(realloc): O(1) ~ O(n)

### 시간 복잡도
블록 오버헤드: 16바이트 (헤더 4 + 풋터 4 + 포인터 8)
정렬 요구사항: 8바이트 정렬

### 제한사항 및 고려사항
최소 블록 크기: 16바이트 (오버헤드 포함)
더블 워드(8바이트) 정렬 필수
외부 단편화 가능성 존재
세그리게이티드 리스트 관리 오버헤드