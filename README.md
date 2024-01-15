# OS_Scheduling_Simulation

### Introduction
This project involves simulating the execution of processes in an operating system and evaluating performance using various job scheduling algorithms. The program will maintain ready and I/O queues, simulate process execution, and gather statistics for different scheduling policies. (With C structures and pointers)

### Problem Description
The canonical model of a process involves computation and I/O in a loop until completion. Each job has attributes like name, run time, and probability of blocking. The input file provides job information, and the program simulates the execution using First Come First Serve and Round Robin scheduling policies. Measure and analyze performance of the system using many of the important measures used to evaluate operating systems performance. The probability of blokcing is set by random int or input parameter. Result collects and prints the busy time, idle time,utilization, number of dispatches and overall throughput for both CPU and I/O devices. 

Include: prism.c

### Input Data
Each job has: name (up to 10 characters), run time (integer), and probability of blocking (float between 0 and 1).
Input file format: <name> <run time> <probability>

### Error Checking
Name: No more than 10 characters.
Run time: Integer greater than or equal to 1.
Probability: Decimal number between 0 and 1 with 2 decimal places.


###  Program Operation Summary
1. Read Job Information \
    Read job details including name, run time, and probability of blocking from the input file.
   
2. Simulate Job Execution \
  Based on the chosen scheduling policy (FCFS or RR), simulate the execution of processes.
  Determine if a process blocks for I/O based on remaining time and probability.
  For FCFS CPU scheduling, a non-blocking process runs until completion; if blocking, determine the run time before blocking.
  For RR CPU scheduling, consider a quantum of 5 units when deciding when a process will block.
  Simulate I/O queue, servicing requests with randomly generated service times.

4. Gather Statistics

