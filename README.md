# 🚛 Concurrent Warehouse Simulation

![Language](https://img.shields.io/badge/language-C11-blue.svg)
![Build System](https://img.shields.io/badge/build-CMake-green.svg)
![Testing](https://img.shields.io/badge/tests-GoogleTest-red.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)

A high-performance, multi-process simulation of a logistics center using **System V IPC** (Shared Memory & Semaphores) for inter-process synchronization. The system models a real-time producer-consumer problem involving workers, a conveyor belt, and a fleet of delivery trucks with specific capacity constraints.

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
* **Build System:** CMake (3.10 or higher).
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
./warehouse_sim <N_Trucks> <K_BeltCapacity> <M_MaxBeltWeight> <W_TruckWeight> <V_TruckVolume>
```

**Example:**
```bash
./warehouse_sim 3 10 500.0 100.0 50.0
```
