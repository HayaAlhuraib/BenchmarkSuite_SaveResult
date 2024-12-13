/* Standard C includes */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sched.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Include all implementations declarations */
#include "impl/naive.h"
#include "impl/opt.h"  // Include optimized implementation

/* Include common headers */
#include "common/types.h"
#include "common/macros.h"

/* Include application-specific headers */
#include "include/types.h"

/* Helper function to print a matrix */
void print_matrix(const char* name, float* matrix, size_t rows, size_t cols) {
    printf("%s:\n", name);
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            printf("%.2f ", matrix[i * cols + j]);
        }
        printf("\n");
    }
    printf("\n");
}

/* Helper function to create the Result directory */
void create_result_directory() {
    struct stat st = {0};
    if (stat("Result", &st) == -1) {
        if (mkdir("Result", 0700) == 0) {
            printf("Result directory created successfully.\n");
        } else {
            perror("Error creating Result directory");
        }
    } else {
        printf("\n ");
    }
}

/* Helper function to export a matrix to a CSV file in the Result directory */
void export_matrix_to_csv(const char* filename, float* matrix, size_t rows, size_t cols) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "Result/%s", filename);

    FILE* file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing.\n", filepath);
        return;
    }

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            fprintf(file, "%.6f", matrix[i * cols + j]);
            if (j < cols - 1) {
                fprintf(file, ",");
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

int main(int argc, char** argv) {
    /* Set the buffer for printf to NULL */
    setbuf(stdout, NULL);

    /* Arguments */
    int nthreads = 1;
    int cpu      = 0;

    /* Create the Result directory */
    create_result_directory();

    /* Prompt the user for matrix dimensions */
    size_t rows_A, cols_A, rows_B, cols_B;

    printf("Enter the number of rows for Matrix A: ");
    scanf("%zu", &rows_A);

    printf("Enter the number of columns for Matrix A: ");
    scanf("%zu", &cols_A);

    printf("Enter the number of rows for Matrix B: ");
    scanf("%zu", &rows_B);

    while (rows_B != cols_A) {
        printf("Number of rows for Matrix B must be equal to the number of columns for Matrix A (%zu).\n", cols_A);
        printf("Enter the number of rows for Matrix B: ");
        scanf("%zu", &rows_B);
    }

    printf("Enter the number of columns for Matrix B: ");
    scanf("%zu", &cols_B);

    /* Initialize matrices */
    srand((unsigned int)time(NULL)); // Seed the random number generator

    float* A = malloc(rows_A * cols_A * sizeof(float));
    float* B = malloc(rows_B * cols_B * sizeof(float));
    float* R = malloc(rows_A * cols_B * sizeof(float));

    if (!A || !B || !R) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    /* Initialize input matrices */
    for (size_t i = 0; i < rows_A * cols_A; i++) {
        A[i] = (float)(rand() % 10); // Random numbers between 0 and 9
    }

    for (size_t i = 0; i < rows_B * cols_B; i++) {
        B[i] = (float)(rand() % 10); // Random numbers between 0 and 9
    }

    /* Print input matrices */
    print_matrix("Matrix A", A, rows_A, cols_A);
    print_matrix("Matrix B", B, rows_B, cols_B);

    /* Export input matrices */
    export_matrix_to_csv("matrix_A.csv", A, rows_A, cols_A);
    export_matrix_to_csv("matrix_B.csv", B, rows_B, cols_B);

    /* Prepare arguments */
    args_t args = { .input = malloc((rows_A * cols_A + rows_B * cols_B) * sizeof(float)), .output = R, .size = rows_A };

    memcpy(args.input, A, rows_A * cols_A * sizeof(float));
    memcpy((float*)args.input + rows_A * cols_A, B, rows_B * cols_B * sizeof(float));

    /* Compare Naive and Optimized Implementations */
    float* R_naive = malloc(rows_A * cols_B * sizeof(float));
    args_t args_naive = { .input = args.input, .output = R_naive, .size = rows_A };

    clock_t start = clock();
    impl_scalar_naive(&args_naive);
    clock_t end = clock();
    export_matrix_to_csv("result_naive.csv", R_naive, rows_A, cols_B);
    double naive_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Naive Runtime: %.6f seconds\n", naive_time);
    print_matrix("Result Matrix R (Naive)", R_naive, rows_A, cols_B);

    float* R_opt = malloc(rows_A * cols_B * sizeof(float));
    args_t args_opt = { .input = args.input, .output = R_opt, .size = rows_A };

    start = clock();
    impl_scalar_opt(&args_opt);
    end = clock();
    export_matrix_to_csv("result_optimized.csv", R_opt, rows_A, cols_B);
    double opt_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Optimized Runtime: %.6f seconds\n", opt_time);
    print_matrix("Result Matrix R (Optimized)", R_opt, rows_A, cols_B);

    printf("Speedup: %.2fx\n", naive_time / opt_time);

    /* Free memory */
    free(A);
    free(B);
    free(R);
    free(R_naive);
    free(R_opt);
    free(args.input);

    return 0;
}
