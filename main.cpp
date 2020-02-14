#include <cmath>
#include <complex>
#include <cstdio>
#include <fstream>
#include <mpi.h>
#include <vector>

using namespace std;
#define MCW MPI_COMM_WORLD

int value(int x, int y, int width, int height) {
  complex<float> point(static_cast<float>(x) / width - 1.5,
                       static_cast<float>(y) / height - 0.5);

  complex<float> z(0, 0);
  unsigned int numIters = 0;
  for (; abs(z) < 2 && numIters <= 34; ++numIters) {
    z = z * z + point;
  }

  if (numIters < 34) {
    return 255 * numIters / 33;
  }

  return 0;
}

int main(int argc, char **argv) {
  int width = 0;
  int height = 0;
  if (argc != 3) {
    throw std::runtime_error("command args are missing: width and height are "
                             "required as such: ./a.out <width> <height>");
  } else {
    width = atoi(argv[1]);
    height = atoi(argv[2]);
  }

  int rank, size;
  int data;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MCW, &rank);
  MPI_Comm_size(MCW, &size);

  int numRows = height / size;
  if (rank < static_cast<int>(height) % size) {
    ++numRows;
  }

  std::vector<int> subRows(numRows * width);

  for (int row = 0; row < numRows; ++row) {
    for (int col = 0; col < width; ++col) {
      int val = value(col, row * size + rank, width, height);
      subRows[row * width + col] = val;
    }
  }

  if (rank == 0) {
    ofstream myBrot("./myBrot.ppm");

    // Image header
    myBrot << "P3\n" << width << " " << height << " 255\n";

    if (myBrot.good()) {
      // Recieve data
      vector<vector<int>> finalRows(size, vector<int>{});
      finalRows[0] = subRows;

      for (int i = 1; i < size; ++i) {
        int tempRows = height / size;
        if (i < static_cast<int>(height) % size) {
          ++tempRows;
        }

        std::vector<int> temp(tempRows * width);
        MPI_Recv(&temp[0], tempRows * width, MPI_INT, i, 0, MCW,
                 MPI_STATUS_IGNORE);
        finalRows[i] = temp;
      }

      for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
          auto val = finalRows[row % size][row / size * width + col];
          switch ((row / 2) % 3) {
          case 0:
            myBrot << val << ' ' << 0 << ' ' << 0 << "\n";
            break;
          case 1:
            myBrot << 0 << ' ' << val << ' ' << 0 << "\n";
            break;
          case 2:
            myBrot << 0 << ' ' << 0 << ' ' << val << "\n";
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
    MPI_Send(&subRows[0], numRows * width, MPI_INT, 0, 0, MCW);
  }

  MPI_Finalize();

  return 0;
}
