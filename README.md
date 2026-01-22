# 🚛 Concurrent Warehouse Simulation

[![Doxygen](https://img.shields.io/badge/Docs-Doxygen-blue?style=flat&logo=doxygen)](https://mkosiorek.pl/docs/warehouse_sim/files.html)
![Language](https://img.shields.io/badge/language-C11-blue.svg)
![Build System](https://img.shields.io/badge/build-CMake-green.svg)
![Testing](https://img.shields.io/badge/tests-GoogleTest-red.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)

A high-performance, multi-process simulation of a logistics center using **System V IPC** (Shared Memory & Semaphores) for inter-process synchronization. The system models a real-time producer-consumer problem involving workers, a conveyor belt, and a fleet of delivery trucks with specific capacity constraints.

## 📚 Documentation & Architecture

The project features comprehensive technical documentation generated using **Doxygen**. It covers:
* Detailed descriptions of all modules (Dispatcher, Truck, Workers).
* **Dependency Graphs** (Include Graphs) visualizing code modularity.
* Overview of data structures and IPC mechanisms.

**[View Full Documentation Online](https://mkosiorek.pl/docs/warehouse_sim/files.html)**

You can also generate the documentation locally:
```bash
cd build
doxygen Doxyfile
# Open doc_output/html/index.html in your web browser
```

## 🚀 Key Features

* **Multi-Process Architecture:** Utilizes `fork()` to spawn autonomous processes for the Dispatcher, Workers, and Trucks.
* **Robust Synchronization:** Implements binary and counting semaphores to manage a circular buffer (conveyor belt) without race conditions or deadlocks.
* **Smart Loading Logic:** Trucks utilize a custom "Peek & Check" algorithm to inspect items on the belt and only load packages that fit their remaining Weight/Volume capacity.
* **Signal Handling:** Asynchronous control flow using `SIGUSR1` (Force Departure/Express Load) and `SIGTERM` (Graceful Shutdown with resource cleanup).
* **Real-time Logging:** Inter-process logs are redirected to a file (`simulation.log`) via `dup2`, keeping the main CLI clean for user interaction.
* **Integration Testing:** Comprehensive test suite written in **GoogleTest** to verify concurrency logic and edge cases.

## 🛠 System Architecture

The simulation consists of three main entities sharing a memory block:

1.  **Dispatcher (Parent):** Orchestrates the simulation, handles user commands, and manages process lifecycles.
2.  **Workers (Producers):**
    * *Standard Workers:* Generate packages at a regular interval.
    * *Express Worker:* Triggered manually by the Dispatcher via signal to prioritize high-value loads.
3.  **Trucks (Consumers):** Dock at the loading bay, retrieve compatible items from the conveyor belt, and depart upon reaching capacity or receiving a force signal.

## 📋 Prerequisites

* **OS:** Linux (requires System V IPC support).
* **Compiler:** GCC or Clang (supporting C11).
* **Build System:** CMake (3.25 or higher).
* **Testing:** GoogleTest (automatically fetched or installed globally).

## 🔨 Build Instructions

Clone the repository and build the project using CMake:

**Default generator:**
```bash
mkdir build
cd build
cmake ..
make
```

**Using Ninja for fast building:**
```bash
mkdir build
cd build
cmake .. -G "Ninja"
ninja
```

**Optional: Throttling the Simulation**\
By default, the simulation runs almost at full CPU speed (few delays were implemented to simulate work time). To make logs readable in real-time, you can set a delay (in milliseconds) for the child processes loop:

```bash
# Example: 1 second delay per loop iteration
cmake .. -DSIM_DELAY_MS=1000
make
```

## 🖥 Usage
Run the simulation from the build directory. You must provide the configuration parameters:
```bash
cd build/src
./warehouse_sim <N_Trucks> <K_BeltCapacity> <M_MaxBeltWeight> <W_TruckWeight> <V_TruckVolume>
```

**Example:**
```bash
./warehouse_sim 3 10 500.0 100.0 50.0
```

**Interactive CLI Commands**
Once running, the Dispatcher listens for commands on stdin:
- 1: Force Departure - Signals the currently docked truck to leave immediately, regardless of load.
- 2: Express Load - Signals the Express Worker (P4) to place a priority package.
- 3: Shutdown - Sends SIGTERM to all processes, cleans up IPC resources, and exits safely.

## 🔍 Observing Logs
Since stdout of child processes is redirected to a file to keep the interface clean, open a second terminal window to watch the simulation in real-time:

```bahs
cd build/src
tail -f simulation.log
```

## 🧪 Testing
The project includes unit and integration tests covering the Truck, Workers and Shared Memory helpers logic.

to run the tests:
```bash
cd build
ctest --output-on-failure
# OR run the test executable directly (more elegant and complete data display):
cd build/tests
./truck_tests
```

## 📂 Project Structure
```bash
.
├── CMakeLists.txt              # Main build configuration
├── build.sh
├── doc
│   └── Doxyfile.in
├── src
│   ├── CMakeLists.txt
│   ├── common                  # Shared headers, IPC wrappers, Utils
│   │   ├── CMakeLists.txt
│   │   ├── common.h            # Shared structutres and definitions
│   │   ├── sem_wrapper.c
│   │   ├── sem_wrapper.h       # Helper library wrapping System V semaphore functions   
│   │   ├── shm_wrapper.c
│   │   ├── shm_wrapper.h       # Helper library wrapping Shared Memory      
│   │   ├── utils.c
│   │   └── utils.h
│   ├── main.c                  # Warehouse dispatcher logic
│   ├── truck.c                 # Truck process logic
│   ├── worker_express.c        # Express Worker (P4) logic
│   └── worker_std.c            # Stdandard Worker logic
└── tests                       # GoogleTest scenarios
    ├── CMakeLists.txt
    ├── test_truck.cpp
    ├── test_utils.cpp
    ├── test_worker_express.cpp
    └── test_worker_std.cpp
```

## 📄 License
This project is open-source and available for educational purposes.

