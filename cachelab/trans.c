/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */

// 8*8 Blocking
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    for (int i = 0; i < M; i += 8) {
        for (int j = 0; j < N; j += 8) {
            for (int k = i; k < i + 8; k ++) {
                for (int m = j; m < j + 8; m++) {
                    B[m][k] = A[k][m];
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

char trans_desc_3232[] = "Matrix 32*32";
void trans_3232(int M, int N, int A[N][M], int B[M][N])
{
   int v0, v1, v2, v3, v4, v5, v6, v7;
    for (int i = 0; i < N; i += 8) {
        for (int j = 0; j < M; j += 8) {
            for (int k = i; k < i + 8; k ++) {
                v0 = A[k][j]; v1 = A[k][j + 1];
                v2 = A[k][j + 2]; v3 = A[k][j + 3];
                v4 = A[k][j + 4]; v5 = A[k][j + 5];
                v6 = A[k][j + 6]; v7 = A[k][j + 7];
                B[j][k] = v0; B[j + 1][k] = v1;
                B[j + 2][k] = v2; B[j + 3][k] = v3;
                B[j + 4][k] = v4; B[j + 5][k] = v5;
                B[j + 6][k] = v6; B[j + 7][k] = v7;
            }
        }
    }
}

char trans_desc_6464[] = "Matrix 64*64";
void trans_6464(int M, int N, int A[N][M], int B[M][N])
{
   int v0, v1, v2, v3, v4, v5, v6, v7;
    for (int i = 0; i < N; i += 8) {
        for (int j = 0; j < M; j += 8) {
            for (int k = i; k < i + 4; k++) {
                v0 = A[k][j]; v1 = A[k][j + 1]; v2 = A[k][j + 2]; v3 = A[k][j + 3];
                v4 = A[k][j + 4]; v5 = A[k][j + 5]; v6 = A[k][j + 6]; v7 = A[k][j + 7];
                B[j][k] = v0; B[j + 1][k] = v1; B[j + 2][k] = v2; B[j + 3][k] = v3;
                B[j][k + 4] = v4; B[j + 1][k + 4] = v5; B[j + 2][k + 4] = v6; B[j + 3][k + 4] = v7;
            }

            for (int m = j; m < j + 4; m++) {
                v0 = A[i + 4][m]; v1 = A[i + 5][m]; v2 = A[i + 6][m]; v3 = A[i + 7][m]; 
                v4 = B[m][i + 4]; v5 = B[m][i + 5]; v6 = B[m][i + 6]; v7 = B[m][i + 7];

                B[m][i + 4] = v0; B[m][i + 5] = v1; B[m][i + 6] = v2; B[m][i + 7] = v3;
                B[m + 4][i] = v4; B[m + 4][i + 1] = v5; B[m + 4][i + 2] = v6; B[m + 4][i + 3] = v7;
            }

            for (int x = i + 4; x < i + 8; x++) {
                v0 = A[x][j + 4]; v1 = A[x][j + 5]; v2 = A[x][j + 6]; v3 = A[x][j + 7];
                B[j + 4][x] = v0; B[j + 5][x] = v1; B[j + 6][x] = v2; B[j + 7][x] = v3;
            }
        }
    }
}

char trans_desc_6167[] = "Matrix 61*67";
void trans_6167(int M, int N, int A[N][M], int B[M][N])
{
   int i, j, k, n, v0, v1, v2, v3, v4, v5, v6, v7;
    for (i = 0; i < N - 16; i += 16) {
        for (j = 0; j < M - 16; j += 16) {
            for (k = i; k < i + 16; k++) {
                v0 = A[k][j]; v1 = A[k][j + 1];
                v2 = A[k][j + 2]; v3 = A[k][j + 3];
                v4 = A[k][j + 4]; v5 = A[k][j + 5];
                v6 = A[k][j + 6]; v7 = A[k][j + 7];

                B[j][k] = v0; B[j + 1][k] = v1;
                B[j + 2][k] = v2; B[j + 3][k] = v3;
                B[j + 4][k] = v4; B[j + 5][k] = v5;
                B[j + 6][k] = v6; B[j + 7][k] = v7;

                v0 = A[k][j + 8]; v1 = A[k][j + 9];
                v2 = A[k][j + 10]; v3 = A[k][j + 11];
                v4 = A[k][j + 12]; v5 = A[k][j + 13];
                v6 = A[k][j + 14]; v7 = A[k][j + 15];

                B[j + 8][k] = v0; B[j + 9][k] = v1;
                B[j + 10][k] = v2; B[j + 11][k] = v3;
                B[j + 12][k] = v4; B[j + 13][k] = v5;
                B[j + 14][k] = v6; B[j + 15][k] = v7;
            }
        }
    }

    for (k = i; k < N; k++) {
        for (n = 0; n < M; n++) {
            B[n][k] = A[k][n];
        }
    }

    for (k = 0; k < i; k++) {
        for (n = j; n < M; n++) {
            B[n][k] = A[k][n];
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

    registerTransFunction(trans_3232, trans_desc_3232);

    registerTransFunction(trans_6464, trans_desc_6464); 

    registerTransFunction(trans_6167, trans_desc_6167);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}