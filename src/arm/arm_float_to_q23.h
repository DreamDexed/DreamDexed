#pragma once

#include <stdint.h>

typedef int32_t q23_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Converts the elements of the floating-point vector to Q23 vector.
 * @param[in]  pSrc       points to the floating-point input vector
 * @param[out] pDst       points to the Q23 output vector
 * @param[in]  blockSize  length of the input vector
 */
void arm_float_to_q23(const float *pSrc, q23_t *pDst, int blockSize);

#ifdef __cplusplus
}
#endif
