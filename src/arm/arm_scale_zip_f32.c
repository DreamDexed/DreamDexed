#include "arm_scale_zip_f32.h"

/**
  Scale two vectors and zip after.  For floating-point data, the algorithm used is:

  <pre>
      pDst[n] = pSrc1[n] * scale, pDst[n+1] = pSrc2[n] * scale   0 <= n < blockSize.
  </pre>

 */

/**
 * @brief Scale two floating-point vector with a scalar and zip after.
 * @param[in]  pSrc1      points to the input vector 1
 * @param[in]  pSrc2      points to the input vector 2
 * @param[in]  scale      scale scalar
 * @param[out] pDst       points to the output vector
 * @param[in]  blockSize  number of samples in the vector
 */

#if defined(ARM_MATH_NEON_EXPERIMENTAL)
#include <arm_math.h>

void arm_scale_zip_f32(const float *pSrc1, const float *pSrc2, float scale, float *pDst, int blockSize)
{
	int blkCnt; /* Loop counter */

	f32x4x2_t res;

	/* Compute 4 outputs at a time */
	blkCnt = blockSize >> 2;

	while (blkCnt > 0)
	{
		res.val[0] = vmulq_n_f32(vld1q_f32(pSrc1), scale);
		res.val[1] = vmulq_n_f32(vld1q_f32(pSrc2), scale);
		vst2q_f32(pDst, res);

		/* Increment pointers */
		pSrc1 += 4;
		pSrc2 += 4;
		pDst += 8;

		/* Decrement the loop counter */
		blkCnt--;
	}

	/* If the blockSize is not a multiple of 4, compute any remaining output samples here.
	** No loop unrolling is used. */
	blkCnt = blockSize & 3;

	while (blkCnt > 0)
	{
		*pDst++ = *pSrc1++ * scale;
		*pDst++ = *pSrc2++ * scale;

		/* Decrement the loop counter */
		blkCnt--;
	}
}
#else
void arm_scale_zip_f32(const float *pSrc1, const float *pSrc2, float scale, float *pDst, int blockSize)
{
	int blkCnt; /* Loop counter */

	blkCnt = blockSize;

	while (blkCnt > 0)
	{
		*pDst++ = *pSrc1++ * scale;
		*pDst++ = *pSrc2++ * scale;

		/* Decrement the loop counter */
		blkCnt--;
	}
}
#endif
