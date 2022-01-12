#include "FreeRTOS.h"
#include "semphr.h"

static uint32_t dummy_semphr;

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return (SemaphoreHandle_t)&dummy_semphr;
}
void vSemaphoreDelete(SemaphoreHandle_t xSemaphore)
{
}
uint32_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, uint32_t xTicksToWait)
{
    return pdTRUE;
}
uint32_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)
{
    return pdTRUE;
}
uint32_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore, int32_t *pxHigherPriorityTaskWoken)
{
    return pdTRUE;
}
uint32_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, int32_t *pxHigherPriorityTaskWoken)
{
    return pdTRUE;
}
