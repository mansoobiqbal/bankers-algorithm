# Banker's Algorithm for Deadlock Avoidance

**Team:** [Mansoob Iqbal, 22k-4747], [Syed Bakhtiar Ahmed, 21k-4767]

## About
A C++ command-line simulator of the Banker's Algorithm. It models processes and
resource types, checks whether the system is in a safe state, and grants or
denies resource requests so that deadlock is avoided.

## Mandatory features
- Resource types, Allocation / Max / Need matrices
- Safety algorithm (shows the safe sequence)
- Request handling (request <= need, request <= available, trial allocation, safety check)
- Deadlock avoidance verification (unsafe requests are denied)

## Additional features (5)
1. Interactive input
2. Randomized resource requests
3. Rollback simulation
4. Logging of safe/unsafe states (log.txt)
5. Performance analysis (perf.csv)

## Build and run
g++ -std=c++17 -O2 banker.cpp -o banker
./banker

## Menu
1 Load textbook example | 2 Enter custom data | 3 Show state | 4 Safety check
5 Make a request | 6 Random simulation | 7 Performance analysis | 8 View log

## Sample results (textbook example)
- Safe sequence: P1 -> P3 -> P4 -> P0 -> P2
- P1 requests (1,0,2): granted
- P4 requests (3,3,0): denied (not available)
- P0 requests (0,2,0): denied (unsafe), rolled back
