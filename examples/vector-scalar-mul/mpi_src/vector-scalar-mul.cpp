#include <iostream>
#include <mpi.h>
#include "df.h"

#define ROOT_RANK 0
#define N 4
#define VECTOR_SIZE 12

void c_initVectorPart(DF &vector, int size, int value) {
    vector.create<int>(size);
    int *vectorData = vector.getData<int>();
    for (int i = 0; i < size; ++i) {
        vectorData[i] = value;
    }
}

void c_vectorPartScalarMul(const DF &vector1Part, const DF &vector2Part, DF &resultVectorPart, int partSize) {
    size_t vectorSize1 = vector1Part.getSize();
    size_t vectorSize2 = vector2Part.getSize();
    if (vectorSize1 != vectorSize2) {
        throw std::invalid_argument("Vectors have different length!");
    }
    const int *vector1Data = vector1Part.getData<int>();
    const int *vector2Data = vector2Part.getData<int>();
    resultVectorPart.create<int>(1);
    int *resultVectorData = resultVectorPart.getData<int>();
    resultVectorData[0] = 0;
    for (size_t i = 0; i < partSize; ++i) {
        resultVectorData[0] += vector1Data[i] * vector2Data[i];
    }
}


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DF vector1, vector2, resultVector;
    const int vectorPartSize = VECTOR_SIZE / N;
    c_initVectorPart(vector1, vectorPartSize, 1);
    c_initVectorPart(vector2, vectorPartSize, 2);
    c_vectorPartScalarMul(vector1, vector2, resultVector, VECTOR_SIZE / N);

    // Printing result
    int resultScalarMul = 0;
    MPI_Reduce(resultVector.getData<int>(), &resultScalarMul, 1, MPI_INT, MPI_SUM, ROOT_RANK, MPI_COMM_WORLD);
    if (rank == ROOT_RANK) {
        printf("Result is %d\n", resultScalarMul);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}