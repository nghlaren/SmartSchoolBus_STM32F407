#pragma once
#include <stdint.h>
extern uint32_t SystemCoreClock;
#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCPU_CLOCK_HZ SystemCoreClock
#define configTICK_RATE_HZ 1000
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE 128
#define configTOTAL_HEAP_SIZE (32*1024)
#define configMAX_TASK_NAME_LEN 12
#define configUSE_16_BIT_TICKS 0
#define configUSE_TIMERS 0
#define configUSE_MUTEXES 0
#define configKERNEL_INTERRUPT_PRIORITY 240
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 80
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
