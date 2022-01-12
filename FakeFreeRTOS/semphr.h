
#ifndef SEMAPHORE_H
#define SEMAPHORE_H
#include <stdint.h>


typedef void *SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateMutex(void);
void vSemaphoreDelete(SemaphoreHandle_t xSemaphore);
uint32_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, uint32_t xTicksToWait);
uint32_t xSemaphoreGive( SemaphoreHandle_t xSemaphore);
uint32_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore, int32_t *pxHigherPriorityTaskWoken);
uint32_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, int32_t *pxHigherPriorityTaskWoken);

#endif
