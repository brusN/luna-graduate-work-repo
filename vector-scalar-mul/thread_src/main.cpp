#pragma warning(disable : 4996)

#include <stdio.h>
#include "mpi.h"
#include "df.h"

#define SIZE 100

void c_initVector(DF& vector, int size, int part_index, int value)
{
    vector.create<int>(size);
    int* vector_data = vector.getData<int>();
    for (int i = 0; i < size; i++)
    {
        vector_data[i] = value;
    }
}

void c_mulVectors(const DF& vector1, const DF& vector2, DF& result)
{
    size_t vector_size1 = vector1.getSize();
    size_t vector_size2 = vector2.getSize();
    if (vector_size1 != vector_size2)
    {
        throw std::invalid_argument("Vectors have different length!");
    }

    size_t result_vector_size = vector_size1 / sizeof(int);
    const int* vector1_data = vector1.getData<int>();
    const int* vector2_data = vector2.getData<int>();
    int scalar = 0;
    for (int i = 0; i < result_vector_size; i++)
    {
        scalar += vector1_data[i] * vector2_data[i];
    }
    //int all_scalar;
    //MPI_Allreduce(&scalar, &all_scalar, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    result.setValue(scalar);
}

void all_reduce(DF& result, int rank)
{
    int scalar = result.getValue<int>();
    //printf("#%d: scalar = %d\n", rank, scalar);
    int all_scalar;
    MPI_Allreduce(&scalar, &all_scalar, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    //printf("#%d: all_scalar = %d\n", rank, all_scalar);
    result.setValue(all_scalar);
}

void c_printResult(DF& result, int part_index)
{
    //int size = vector.getSize() / sizeof(int);
    //const int* vector_data = vector.getData<int>();
    //printf("Vector with index part %d is [", part_index);
    //for (int i = 0; i < size - 1; i++)
    //{
    //    printf("%d ", vector_data[i]);
    //}
    //printf("%d]\n", vector_data[size - 1]);
    printf("#%d: %d\n", part_index, result.get_int());
}

int main(int argc, char* argv[])
{
    int size;
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    //printf("Process %d of %d\n", rank, size);

    DF vector1, vector2, result;
    c_initVector(vector1, SIZE / size, rank, 1);
    c_initVector(vector2, SIZE / size, rank, 2);
    c_mulVectors(vector1, vector2, result);

    all_reduce(result, rank);

    c_printResult(result, rank);
    
    MPI_Finalize();
    return 0;
}
