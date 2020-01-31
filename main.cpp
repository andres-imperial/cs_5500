#include <chrono>
#include <ctime>
#include <iostream>
#include <mpi.h>
#include <stdlib.h>
#include <thread>
#include <unistd.h>

#define MCW MPI_COMM_WORLD

using namespace std;

int main(int argc, char **argv){
  // Init random seed
  srand(time(NULL));

  // Variables
  int rank, size;
  int bomb;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MCW, &rank);
  MPI_Comm_size(MCW, &size);

  // process zero will start the bomb
  if (rank == 0) {
    bomb = rand() % 100;
    cout << "process 0 generated a bomb with timer at" << bomb << endl;

    while (true) {
      // find next sucker
      int addr = rand() % size;
      // don't send it to yourself
      addr = addr == rank ? (addr + 1) % size : addr;
      MPI_Send(&bomb, 1, MPI_INT, addr, 0, MCW);
      MPI_Recv(&bomb, 1, MPI_INT, MPI_ANY_SOURCE, 0, MCW, MPI_STATUS_IGNORE);
      this_thread::sleep_for(std::chrono::milliseconds(10));
      --bomb;

      if (bomb == 0) {
        cout << "BOOM! -- process " << rank << " loses\n";
        MPI_Abort(MCW, 0);
      }
      cout << "process 0 holds the bomb with timer at " << bomb
           << " and tossed it\n";
    }
  } else {
    while (true) {
      MPI_Recv(&bomb, 1, MPI_INT, MPI_ANY_SOURCE, 0, MCW, MPI_STATUS_IGNORE);
      this_thread::sleep_for(std::chrono::milliseconds(10));
      --bomb;

      if (bomb == 0) {
        cout << "BOOM! -- process " << rank << " loses\n";
        MPI_Abort(MCW, 0);
      }
      cout << "process " << rank << " holds the bomb with timer at " << bomb
           << " and tossed it\n";

      // find next sucker
      int addr = rand() % size;
      // don't send it to yourself
      addr = addr == rank ? (addr + 1) % size : addr;
      MPI_Send(&bomb, 1, MPI_INT, rand() % size, 0, MCW);
    }
  }

  MPI_Finalize();

  return 0;
}
