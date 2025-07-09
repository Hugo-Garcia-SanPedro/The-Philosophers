# Concurrent Philosophers Simulation 🧠🔁🍽️

A multi-threaded Windows simulation of the classic *Dining Philosophers* problem — reimagined with dynamic zones like a field, bridge, temple, lobby, and dining hall. The simulation showcases synchronization techniques such as semaphores, mutexes, and critical sections, with advanced deadlock prevention and barrier-style coordination.

---

## 📑 Table of Contents
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Architecture Overview](#architecture-overview)
- [Synchronization Strategy](#synchronization-strategy)
- [Deadlock Prevention](#Deadlock-Prevention)
- [Barrier Synchronization](#Barrier-Synchronization)

---

## ✨ Features
- 🧵 **Multi-threaded**: Each philosopher operates in its own thread.
- 🌉 **Extended Zones**: Field, bridge, lobby, dining hall, and temple.
- 🔒 **Synchronization**: Semaphores, mutexes, and critical sections used throughout.
- 🚫 **Deadlock Prevention**: Seat-based fork acquisition strategy.
- ⛓️ **Barrier Coordination**: All philosophers must walk before crossing the bridge.
- 📦 **DLL Integration**: Core simulation logic dynamically loaded at runtime.

---

## 📋 Prerequisites
- Windows 10 or later (x64).
- Microsoft Visual Studio 2019+ (C++ desktop workload).
- Windows SDK (for threading APIs and synchronization primitives).
- Precompiled DLL file: `filosofar2.dll` implementing `FI2_*` functions.
- Configure DLL_PATH in the main.

---

## 🧬 Architecture Overview
- Main: Entry point, DLL loading.
- Filosofar2.h: Declares simulation interface.
- Create_IPCS: Initializes semaphores, mutexes and shared memoory.

---

## 🛡️ Synchronization Strategy
Semaphores:
- sem_bridge, sem_lobby, sem_dining, sem_temple
Mutexes:
- Forks (mutex_forks[])
- Movement grid (mutex_move[x][y])
Critical Sections:
- Bridge direction counter
- Seat occupancy arrays
- Shared counters (shared_memory::total_walked)

---

## 🔀 Deadlock Prevention
Philosophers alternate fork pickup order (left-first vs. right-first) depending on seat index parity.

---

## ⏳ Barrier Synchronization
All threads must perform at least one walk (sem_all_walked) before crossing the bridge.

---
