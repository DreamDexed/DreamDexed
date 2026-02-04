#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief zip two floating-point vector.
 * @param[in]  pSrc1      points to the input vector 1
 * @param[in]  pSrc2      points to the input vector 2
 * @param[out] pDst       points to the output vector
 * @param[in]  blockSize  number of samples in the vector
 */
void arm_zip_f32(const float *pSrc1, const float *pSrc2, float *pDst, int blockSize);

#ifdef __cplusplus
}
#endif
