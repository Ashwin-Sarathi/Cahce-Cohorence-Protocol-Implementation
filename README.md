# Cahce-Cohorence-Protocol-Implementation
Implementation and Simulation of MESI and MOESI cache coherence protocols on the benchmark provided. 
This project is a part of the coursework ECE 506 (Architectures of Parallel Computing) at NC State University. 

There are 2 traces that can be run in either MESI or MOESI cache state configuration. 
To remove pre-existing binaries, use make clean
The debug trace has 10000 instructions and the long trace has 500000 instructions.
The commands to run the traces in MESI or MOESI modes are as follows:
1. MESI Debug - make debug_mesi
2. MOESI Debug - make debug_moesi
3. MESI Long Trace - make test_mesi
4. MOESI Long Trace - make test_moesi
