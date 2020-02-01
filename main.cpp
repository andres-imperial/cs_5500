#include <algorithm>
#include <cstdio>
#include <mpi.h>
#include <time.h>
#include <vector>

using namespace std;
#define MCW MPI_COMM_WORLD

int main(int argc, char **argv) {
  int rank, size;
  int data;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MCW, &rank);
  MPI_Comm_size(MCW, &size);

  if (rank == 0) {
    srand(time(0));
    unsigned int numElements = rand() % 100;
    vector<int> unsortedArray(numElements);

    unsigned int rowsPerThread = numElements / size;


    for (int i = 0; i < numElements; i++) {
      unsortedArray[i] = rand() % 100;
    }

    for (int process = 1; process < size; ++process) {
      unsigned int start = process * rowsPerThread + 1;
      unsigned int end = (process + 1) * rowsPerThread;

      if (numElements - end < rowsPerThread)
        end = numElements - 1;

      int elements = end - start + 1;

      printf("start: %u -- end: %u\n", start, end);

      MPI_Send(&elements, 1, MPI_INT, process, 0, MCW);

      MPI_Send(&unsortedArray[start], elements, MPI_INT, process, 0, MCW);
    }

    vector<vector<int>> sortedArrays;

    vector<int> myArray(unsortedArray.begin(),
                        unsortedArray.begin() + rowsPerThread);
    sort(myArray.begin(), myArray.end());
    sortedArrays.push_back(myArray);
    for (int process = 1; process < size; ++process) {
      unsigned int start = process * rowsPerThread + 1;
      unsigned int end = (process + 1) * rowsPerThread;

      if (numElements - end < rowsPerThread)
        end = numElements - 1;

      int elements = end - start + 1;

      vector<int> temp(elements);
      MPI_Recv(&temp[0], elements, MPI_INT, MPI_ANY_SOURCE, 1, MCW,
               MPI_STATUS_IGNORE);
      sortedArrays.push_back(temp);
    }

    vector<int> finalArray(numElements);
    for (unsigned int i = 0; i < numElements; ++i) {
      int smallest = 0;
      for (unsigned int j = 1; j < sortedArrays.size(); ++j) {
        if (sortedArrays[smallest] > sortedArrays[j]) {
          smallest = j;
        }
      }
      finalArray[i] = sortedArrays[smallest][0];
      printf("smallest: %i -- sortedArrays.size() : %lu\n", smallest,
             sortedArrays.size());
      sortedArrays[smallest].erase(sortedArrays[smallest].begin());
      if (sortedArrays[smallest].empty()) {
        sortedArrays.erase(sortedArrays.begin() + smallest);
      }
    }

    for (auto element : finalArray) {
      printf("%i\n", element);
    }
  }

  else {
    int elements;
    printf("here\n");
    MPI_Recv(&elements, 1, MPI_INT, 0, 0, MCW, MPI_STATUS_IGNORE);

    printf("elements: %i\n", elements);
    vector<int> myArray(elements, 0);
    MPI_Recv(&myArray[0], elements, MPI_INT, 0, 0, MCW, MPI_STATUS_IGNORE);
    printf("here\n");

    sort(myArray.begin(), myArray.end());
    printf("here\n");

    MPI_Send(&myArray[0], elements, MPI_INT, 0, 1, MCW);
  }

  MPI_Finalize();

  return 0;
}
