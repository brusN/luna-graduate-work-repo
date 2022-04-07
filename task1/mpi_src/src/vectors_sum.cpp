#include <iostream>
#include <mpi.h>
#include "df.h"

#define ROOT_RANK 0
#define N 10
#define VECTOR_SIZE 100

void c_initVector(DF &vector, int size, int partIndex, int value) {
    vector.create<int>(size);
    int *vectorData = vector.getData<int>();
    for (int i = 0; i < size; ++i) {
        vectorData[i] = value;
    }
}

void c_sumVectors(const DF &vector1, const DF &vector2, DF &resultVector) {
    size_t vectorSize1 = vector1.getSize();
    size_t vectorSize2 = vector2.getSize();
    if (vectorSize1 != vectorSize2) {
        throw std::invalid_argument("Vectors have different length!");
    }

    size_t resultVectorSize = vectorSize1 / sizeof(int);
    const int *vector1Data = vector1.getData<int>();
    const int *vector2Data = vector2.getData<int>();
    resultVector.create<int>(resultVectorSize);
    int *resultVectorData = resultVector.getData<int>();
    for (size_t i = 0; i < resultVectorSize; ++i) {
        resultVectorData[i] = vector1Data[i] + vector2Data[i];
    }
}

void c_printVector(const DF &vector, int partIndex) {
    int size = vector.getSize() / sizeof(int);
    const int *vectorData = vector.getData<int>();
    printf("Vector with index part %d is [", partIndex);
    for (int i = 0; i < size - 1; ++i) {
        printf("%d ", vectorData[i]);
    }
    printf("%d]\n", vectorData[size - 1]);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DF vector1, vector2, resultVector;
    c_initVector(vector1, VECTOR_SIZE, rank, 1);
    c_initVector(vector2, VECTOR_SIZE, rank, 2);
    c_sumVectors(vector1, vector2, resultVector);
    c_printVector(resultVector, rank);

    MPI_Finalize();
    return EXIT_SUCCESS;
}