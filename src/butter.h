#pragma once

void butter_lp( int n, float fcf, float *coeffs );
void butter_hp( int n, float fcf, float *coeffs );
void butter_bp( int n, float f1f, float f2f, float *coeffs );
void butter_bs( int n, float f1f, float f2f, float *coeffs );
void butter_stage_arrange_arm( int n, int stage, float *coeffs );
