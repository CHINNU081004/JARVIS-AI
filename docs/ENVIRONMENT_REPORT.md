# Environment Inspection Report: JARVIS

**Date & Time:** 2026-11-22T02:26:30+05:30  
**Host Architecture:** x86_64  
**Operating System:** Ubuntu 26.04.1 LTS (Resolute Raccoon)  
**Kernel:** Linux 7.0.0-44-generic  

---

## 1. Hardware Specifications

| Component | Specification |
| :--- | :--- |
| **CPU** | Intel(R) Core(TM) i5-7200U @ 2.50GHz (2 Cores, 4 Threads) |
| **RAM** | 7.1 GiB Total (2.5 GiB used, ~4.6 GiB available) |
| **Swap** | 19 GiB |
| **Storage** | Root filesystem (`/dev/sdb2`): 116 GB Total, 33 GB used, 78 GB free (30% utilization) |
| **GPU** | Integrated Intel Graphics (HD Graphics 620) |

---

## 2. Verified Toolchain & Environment Audit

All toolchain components have been installed, verified, and confirmed operational:

| Tool / Module | Status | Verified Version / Path |
| :--- | :--- | :--- |
| **C++ Compiler (g++)** | Verified | GCC 15.2.0 (`/usr/bin/g++`) |
| **C Compiler (gcc)** | Verified | GCC 15.2.0 (`/usr/bin/gcc`) |
| **CMake** | Verified | CMake 4.2.3 (`/usr/bin/cmake`) |
| **Ninja** | Verified | Ninja 1.13.2 (`/usr/bin/ninja`) |
| **Qt 6 Framework** | Verified | Qt 6.10.2 (`/usr/lib/x86_64-linux-gnu`) |
| **Qt Modules Installed** | Verified | Qt Core, Network, Sql, Test, Quick, Qml, Gui |
| **QML Modules** | Verified | QtQuick, QtQuick.Controls, QtQuick.Layouts, QtQuick.Effects, QtQuick.Shapes, QtQuick.Window, QtQuick.Templates, QtQuick.Particles, QtCore, QtNetwork |
| **git** | Verified | `/usr/bin/git` |
| **dpkg-deb** | Verified | `/usr/bin/dpkg-deb` |
| **python3** | Verified | `/usr/bin/python3` (v3.14.4) |
| **Ollama** | Verified | `/usr/local/bin/ollama` (v0.40.16) |

---

## 3. Architecture & Build Configuration

- **Target Standard:** C++20 (`set(CMAKE_CXX_STANDARD 20)`)
- **UI Technology:** Qt 6 QML / QtQuick with hardware-accelerated Scene Graph (GL/Vulkan/software fallback support)
- **Persistence:** SQLite via `Qt6::Sql`
- **Networking:** Asynchronous streaming via `Qt6::Network`
- **Packaging:** Debian package (`.deb`) generated via CPack and `dpkg-deb`
