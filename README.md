# `BaseJMP` Code Repository

This repository contains the source code developed for my Master Thesis "Applying Emulation and Protocol Fuzzing for Identifying Vulnerabilities in Mobile Baseband Processors".
The thesis explores fuzzing strategies that are based on active learning and grammar-based fuzzing.

In order to maintain a modular codebase, this repository is organized with submodules. 
To clone the repository with all its necessary components, use the flag `--recurse-submodules`.

## Structure

The structure is divided into six sections, each containing different submodules relevant to this project.

### 1. Base-Station, Core Network & UE Simulation

This section contains setups for simulating different gNB, UE, and core network implementations.
The motivation lies in the fact that Software UEs are implemented (somewhat) similar to real-world implementations.
In addition, they provide a good insight into fuzzing real-world UEs and protocol fuzzing.

1. [`UERANSIM`](https://github.com/Jojeker/a) Contains a setup for the simulated UERANSIM implementation. 
It was initially explored due to the simplicity of abstracting the physical layer.

2. [`OAI`](https://github.com/Jojeker/a) This is a C-implementation, that is more similar to an actual UE.

3. [`srsUE`](https://github.com/Jojeker/a) This is a very complete implementation, that is written in C++.


### 2. Learning Prototypes

This section provides some tooling and tests to explore the active learning.

1. [Mealy Machine](https://github.com/Jojeker/MealyMachine) This is a very simple mealy-machine that is used to perform automata learning (a narrowed-down version of the real setup).

2. [LLVM Global Instrumentation Pass](https://github.com/Jojeker/LLVMGlobalInstrumentation) This is an LLVM pass, that is used to track writes to (global section) symbols and provide output, when those symbols are accessed. It is used for tracking state change (`enum`/`bool`).

3. [`LearnLib` Mealy-Learner](https://github.com/Jojeker/MealyLearner) This project is over-arching for the learning setup and performs learning for the [Mealy Machine](https://github.com/Jojeker/MealyMachine) by instrumenting the machine with the compiler pass to additionally track internal state.

### 3. `Scapy` & `Pycrate` "Naïve Fuzzing" Setup

This section provides a bare-bones fuzzing setup, that fuzzes the ASN.1 message structure by sending arbitrarily crafted ASN.1 packets to a UE.
It uses a pre-defined message sequence, that is recorded beforehand and then replayed with modified IEs and Fields.

1. [ASN.1 Fuzzing Scripts](https://github.com/Jojeker/ASN1-Fuzzing-Scripts) This repository contains (currently one) `pcap` recording that is used to parse out the ASN.1 fields, modify them and re-encode a valid packet. It is used as a preparatory step to integrate with a modified `pycrate` repository that is extended by a fuzzing component.

2. [`pycrate` Fuzzer](https://github.com/Jojeker/pycrate-nr-fuzz) This (TBC) fuzzer modifies the ASN.1 engine `pycrate` that can de-/encode packets. It does this by validating the ranges and options of packets to create valid payloads, that do not reach parser-errors in the UE.

### 4. Active Fuzzing Setup

This section provides a more complex fuzzer, that uses "real" components to perform Fuzzing and Learning.
For this setup, a modified gNB and Core Network and a LearnLib implementation and Fuzzer.

1. [`open5gs` Fuzzer](https://github.com/Jojeker/open5gs) This repository is a modified version of the Core Network Implementation Open5gs. 
The modifications include the AMF and UPF, since these components are the only two that are interacting with the UE. 
The protocol of interest is mainly NAS and GTP (User Plane Traffic).
Also, scripts for creating fuzzing payloads from python exist.

2. [`srsRAN/OAI` Fuzzer](https://github.com/Jojeker/a) This repository contains multiple fuzzer implementations for the gNBs. 
This includes RRC changes (replaying, dropping, duplicating, etc.).

### 5. Re-Hosting Setup

This section is currently up-to-date with remote and includes [FirmWire](https://github.com/Jojeker/FirmWire).

### 6. Reproducers

This section contains patches to reproduce some results:

1. [Shannon: Samsung Galaxy S5 Mini](https://github.com/Jojeker/Shannon-S5-Mini-Reproducer) This contains some artifacts and scripts to reproduce the "A walk with Shannon talk" by Amat Cama.
2. [5GBaseChecker](https://github.com/Jojeker/5GBaseChecker) This is a patched, version of the 5GBaseChecker Project that is more reproducible.


### Artifacts

The artifacts of the evaluation will be added as a release.