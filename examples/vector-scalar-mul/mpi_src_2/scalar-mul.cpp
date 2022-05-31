#include <stdio.h>
#include "mpi.h"
#include "df.h"

#define SIZE 100

void c_initVector(DF& vector, int size, int value)
{
    vector.create<int>(size);
    int* vectorData = vector.getData<int>();
    for (int i = 0; i < size; ++i)
    {
        vectorData[i] = value;
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
    result.setValue(scalar);
}

void c_sum_result(const DF& a, const DF& b, DF& c)
{
    int a_ = a.getValue<int>();
    int b_ = b.getValue<int>();
    c.setValue(a_ + b_);
}

void c_copy_result(const DF& a, DF& b)
{
    b.copy(a);
}

void c_printResult(const DF& res)
{
    printf("Scalar: %d\n", res.getValue<int>());
    fflush(stdout);
}

int main(int argc, char* argv[])
{
    int size;
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
 
    DF vector1, vector2, result, scalar;
    DF buffer;
    c_initVector(vector1, SIZE, rank, 1);
    c_initVector(vector2, SIZE, rank, 2);
    c_mulVectors(vector1, vector2, result);
    
    int send_result;
    int recv_result;
    if (size > 1)
    {
        if (rank == 0)
        {
            send_result = result.getValue<int>();
            MPI_Send(&send_result, 1, MPI_INT, 1, rank, MPI_COMM_WORLD);
            printf("%d: send to %d; result = %d\n", rank, 1, send_result);
            fflush(stdout);
        }
        if (rank == 1)
        {
            MPI_Recv(&recv_result, 1, MPI_INT, 0, rank - 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%d: recv from %d; result = %d\n", rank, 0, recv_result);
            fflush(stdout);
            buffer.setValue(recv_result);
            c_sum_result(buffer, result, scalar);
        }
    }

    if (size == 1)
    {
        c_copy_result(result, scalar);
        printf("Size = %d\n", size);
        fflush(stdout);
    }

    int send_scalar;
    int recv_scalar;
    for (int i = 2; i <= size - 1; i++)
    {
        if (rank == i - 1)
        {
            send_scalar = scalar.getValue<int>();
            MPI_Send(&send_scalar, 1, MPI_INT, i, rank, MPI_COMM_WORLD);
            printf("%d: send to %d; scalar = %d\n", rank, i, send_scalar);
            fflush(stdout);
        }
        if (rank == i)
        {
            MPI_Recv(&recv_scalar, 1, MPI_INT, i - 1, rank - 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%d: recv from %d; scalar = %d\n", rank, i - 1, recv_scalar);
            fflush(stdout);
            buffer.setValue(recv_scalar);
            c_sum_result(result, buffer, scalar);
        }
    }

    if (rank == size - 1)
    {
        c_printResult(scalar);
    }

    MPI_Finalize();
    return 0;
}
