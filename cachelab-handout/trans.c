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
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int v0,v1,v2,v3;
    int v4,v5,v6,v7;
    if(M==32) {
        for(int i=0;i<32;i+=8) {
            for(int j=0;j<32;j+=8) {
                for(int k=0;k<8;k++) {
                    //使用临时变量防止对角线上的互斥
                    v0 = A[i+0][j+k],v1 = A[i+1][j+k],v2 = A[i+2][j+k],v3= A[i+3][j+k];
                    v4 = A[i+4][j+k],v5 = A[i+5][j+k],v6 = A[i+6][j+k],v7 = A[i+7][j+k];

                    B[j+k][i+0] = v0,B[j+k][i+1] = v1,B[j+k][i+2] = v2,B[j+k][i+3] = v3;
                    B[j+k][i+4] = v4,B[j+k][i+5] = v5,B[j+k][i+6] = v6,B[j+k][i+7] = v7;
                }             
            }
        }
    } else if(M==64){
        for(int i=0;i<64;i+=8) {
            for(int j=0;j<64;j+=8) {

                for(int k=0;k<4;k++) {
                    v0 = A[i+0][j+k],v1 = A[i+1][j+k], v2 = A[i+2][j+k],v3 = A[i+3][j+k];
                    v4 = A[i+0][j+k+4],v5 = A[i+1][j+k+4],v6 = A[i+2][j+k+4],v7 = A[i+3][j+k+4];

                    B[j+k][i+0] = v0,B[j+k][i+1] = v1,B[j+k][i+2] = v2,B[j+k][i+3] = v3;
                    B[j+k][i+4] = v4,B[j+k][i+5] = v5,B[j+k][i+6] = v6,B[j+k][i+7] = v7;
                }


                for(int k=0;k<4;k++) {
                    v0 = B[j+k][i+4];
                    v1 = B[j+k][i+5];
                    v2 = B[j+k][i+6];
                    v3 = B[j+k][i+7];

                    v4 = A[i+4][j+k];
                    v5 = A[i+5][j+k];
                    v6 = A[i+6][j+k];
                    v7 = A[i+7][j+k];

                    B[j+k][i+4] = v4;
                    B[j+k][i+5] = v5;
                    B[j+k][i+6] = v6;
                    B[j+k][i+7] = v7;

                    B[j+k+4][i+0] = v0;
                    B[j+k+4][i+1] = v1;
                    B[j+k+4][i+2] = v2;
                    B[j+k+4][i+3] = v3;
                }

                for(int k=4;k<8;k++) {
                    v4 = A[i+4][j+k],v5 = A[i+5][j+k],v6 = A[i+6][j+k],v7 = A[i+7][j+k];
                    B[j+k][i+4] = v4,B[j+k][i+5] = v5,B[j+k][i+6] = v6,B[j+k][i+7] = v7;
                }
            } 
        }
    } else {
        for(int i=0;i<N;i+=16){
            for(int j=0;j<M;j+=16){
                for(int k=i;k<i+16&&k<N;k++){
                    for(int h=j;h<j+16&&h<M;h++){
                        B[h][k]=A[k][h];
                    }
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

