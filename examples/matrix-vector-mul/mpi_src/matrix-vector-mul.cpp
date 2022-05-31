#include <iostream>
#include "df.h"
#include <mpi.h>

#define ROOT_RANK 0

#define N 30
#define M 30

void c_initMatrixByValue(DF &matrix, int n, int m, double value) {
    matrix.create<double>(n * m);
    double *ptrData = matrix.getData<double>();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            ptrData[i * m + j] = value;
        }
    }
}

void c_initVectorByValue(DF &vector, int n, double value) {
    vector.create<double>(n);
    double *ptrData = vector.getData<double>();
    for (int i = 0; i < n; ++i) {
        ptrData[i] = value;
    }
}

void c_mulMatrixOnVector(const DF &matrix, const DF &vector, int n, int m, DF &resultVector) {\
    const double *matrixData = matrix.getData<double>();
    const double *vectorData = vector.getData<double>();
    resultVector.create<double>(n);
    double *resultVectorData = resultVector.getData<double>();
    for (int i = 0; i < n; ++i) {
        resultVectorData[i] = 0;
        for (int j = 0; j < m; ++j) {
            resultVectorData[i] += matrixData[i * m + j] * vectorData[j];
        }
    }
}

void c_printVector(const DF &vector, int size) {
    const double *vectorData = vector.getData<double>();
    std::cout << "Vector is [";
    for (int i = 0; i < size - 1; ++i) {
        std::cout << vectorData[i] << " ";
    }
    std::cout << vectorData[size - 1] << "]" << std::endl;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DF partMatrix, vector, partResultVector;
    c_initMatrixByValue(partMatrix, N / size, M, 1.0);
    c_initVectorByValue(vector, N, 1.0);
    c_mulMatrixOnVector(partMatrix, vector, N / size, M, partResultVector);
    c_printVector(partResultVector, N / size);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
