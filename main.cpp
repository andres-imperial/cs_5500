#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <fstream>
#include <mpi.h>
#include <vector>

using namespace std;
#define MCW MPI_COMM_WORLD

int WIDTH = 800;
int HEIGHT = 800;
float re0;
float re1;
float c0;
float c1;

int value(int x, int y) {
  complex<float> point((static_cast<float>(x) * (re0 - re1) / WIDTH) + re1,
                       (static_cast<float>(y) * (c0 - c1) / HEIGHT) + c1);

  complex<float> z(point);
  unsigned int numIters = 0;
  for (; abs(z) < 2 && numIters <= 512; ++numIters) {
    z = z * z + point;
  }

  if (numIters < 34) {
    return 255 * numIters / 33;
  }

  return 0;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    re0 = 0.0014;
    re1 = -0.748;
    c0 = 0.1;
    c1 = c0 + (re1 - re0);
  } else {
    re0 = atoi(argv[1]);
    c0 = atoi(argv[2]);
    re1 = atoi(argv[3]);
    c1 = c0 + (re1 - re0);
  }

  int rank, size;
  int data;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MCW, &rank);
  MPI_Comm_size(MCW, &size);

  auto start = std::chrono::high_resolution_clock::now();

  int numRows = HEIGHT / size;
  if (rank < static_cast<int>(HEIGHT) % size) {
    ++numRows;
  }

  std::vector<int> subRows(numRows * WIDTH);

  for (int row = 0; row < numRows; ++row) {
    for (int col = 0; col < WIDTH; ++col) {
      int val = value(col, row * size + rank);
      subRows[row * WIDTH + col] = val;
    }
  }

  if (rank == 0) {
    ofstream myBrot("./myBrot.ppm");

    // Image header
    myBrot << "P3\n" << WIDTH << " " << HEIGHT << " 255\n";

    if (myBrot.good()) {
      // Recieve data
      vector<vector<int>> finalRows(size, vector<int>{});
      finalRows[0] = subRows;

      for (int i = 1; i < size; ++i) {
        int tempRows = HEIGHT / size;
        if (i < static_cast<int>(HEIGHT) % size) {
          ++tempRows;
        }

        std::vector<int> temp(tempRows * WIDTH);
        MPI_Recv(&temp[0], tempRows * WIDTH, MPI_INT, i, 0, MCW,
                 MPI_STATUS_IGNORE);
        finalRows[i] = temp;
      }

      for (int row = 0; row < HEIGHT; ++row) {
        for (int col = 0; col < WIDTH; ++col) {
          auto val = finalRows[row % size][row / size * WIDTH + col];
          switch (row % 6) {
          case 0:
            myBrot << val << ' ' << 0 << ' ' << 0 << "\n";
            break;
          case 1:
            myBrot << val << ' ' << val << ' ' << 0 << "\n";
            break;
          case 2:
            myBrot << 0 << ' ' << val << ' ' << 0 << "\n";
            break;
          case 3:
            myBrot << 0 << ' ' << val << ' ' << val << "\n";
            break;
          case 4:
            myBrot << 0 << ' ' << 0 << ' ' << val << "\n";
            break;
          case 5:
            myBrot << val << ' ' << 0 << ' ' << val << "\n";
            break;
          };
        }
      }

      myBrot.close();
    } else {
      throw std::runtime_error("was not able to open file");
    }
  } else {
    // Send data to thread 0
    MPI_Send(&subRows[0], numRows * WIDTH, MPI_INT, 0, 0, MCW);
  }

  MPI_Finalize();

  if (rank == 0) {
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    printf("%li\n", duration.count());
  }

  return 0;
}
