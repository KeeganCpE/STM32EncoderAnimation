#include "encoder.h"
#include "screen.h"

extern TIM_HandleTypeDef htim2;

void measureEncoder(Frame* currFrame) {
    uint32_t pos = __HAL_TIM_GET_COUNTER(&htim2);

    if (pos >= 2 && pos < 10000) {
        // Counting forward
        *currFrame = (*currFrame + 1) % 7;
        __HAL_TIM_SET_COUNTER(&htim2, 0);

    } else if (pos > 10000) {
        // Counting backward — counter wrapped near ARR (19999)
        *currFrame = (*currFrame + 6) % 7;  // +6 ≡ -1 (mod 7), avoids underflow
        __HAL_TIM_SET_COUNTER(&htim2, 0);
    }
}
