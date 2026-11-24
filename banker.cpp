// Banker's Algorithm for Deadlock Avoidance - CS-2006 OS Project
// Mandatory: resources, Allocation/Max/Need matrices, safety algorithm,
//            request handling, deadlock avoidance verification
// Extras (5): interactive input, randomized requests, logging safe/unsafe
//             states, rollback simulation, performance analysis

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

// ---------- colors (set to false if your terminal shows odd symbols) ----------
const bool USE_COLOR = true;
const string GREEN = USE_COLOR ? "\033[32m" : "";
const string RED = USE_COLOR ? "\033[31m" : "";
const string CYAN = USE_COLOR ? "\033[36m" : "";
const string RESET = USE_COLOR ? "\033[0m" : "";

// ---------- system state ----------
struct State {
    int n = 0;                      // number of processes
    int m = 0;                      // number of resource types
    vector<vector<int>> alloc;      // Allocation matrix
    vector<vector<int>> maxm;       // Maximum matrix
    vector<vector<int>> need;       // Need = Max - Allocation
    vector<int> avail;              // Available vector

    void computeNeed() {
        need.assign(n, vector<int>(m, 0));
        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++) need[i][j] = maxm[i][j] - alloc[i][j];
    }
};

State sys;
mt19937 rng(random_device{}());
ofstream logFile;

// ---------- logging (extra: log safe/unsafe states) ----------
void logMsg(const string& s) {
    if (logFile.is_open()) {
        logFile << s << "\n";
        logFile.flush();
    }
}

string vecStr(const vector<int>& v) {
    string s = "(";
    for (size_t i = 0; i < v.size(); i++) s += to_string(v[i]) + (i + 1 < v.size() ? "," : "");
    return s + ")";
}

string seqStr(const vector<int>& seq) {
    string s;
    for (size_t i = 0; i < seq.size(); i++) s += "P" + to_string(seq[i]) + (i + 1 < seq.size() ? " -> " : "");
    return s;
}

// ---------- printing ----------
void printState(const State& s) {
    cout << "\n" << CYAN << "=== SYSTEM STATE ===" << RESET << "\n";
    cout << left << setw(6) << "Proc";
    for (const char* name : {"Allocation", "Max", "Need"}) {
        string h = name;
        cout << "| " << setw(max(4 * s.m, 10)) << h;
    }
    cout << "\n";
    auto row = [&](const vector<int>& v) {
        string t;
        for (int x : v) t += to_string(x) + " ";
        return t;
    };
    for (int i = 0; i < s.n; i++) {
        cout << setw(6) << ("P" + to_string(i));
        cout << "| " << setw(max(4 * s.m, 10)) << row(s.alloc[i]);
        cout << "| " << setw(max(4 * s.m, 10)) << row(s.maxm[i]);
        cout << "| " << setw(max(4 * s.m, 10)) << row(s.need[i]) << "\n";
    }
    cout << "Available: " << vecStr(s.avail) << "\n";
}

// ---------- MANDATORY: safety algorithm ----------
bool isSafe(const State& s, vector<int>& seq) {
    vector<int> work = s.avail;
    vector<bool> finish(s.n, false);
    seq.clear();
    while ((int)seq.size() < s.n) {
        bool found = false;
        for (int i = 0; i < s.n; i++) {
            if (finish[i]) continue;
            bool ok = true;
            for (int j = 0; j < s.m; j++)
                if (s.need[i][j] > work[j]) { ok = false; break; }
            if (ok) {
                for (int j = 0; j < s.m; j++) work[j] += s.alloc[i][j];
                finish[i] = true;
                seq.push_back(i);
                found = true;
            }
        }
        if (!found) return false;   // no process can finish -> unsafe
    }
    return true;
}

// ---------- MANDATORY: request handling (+ extra: rollback) ----------
enum Result { GRANTED, EXCEEDS_NEED, NOT_AVAILABLE, UNSAFE_ROLLBACK };

Result requestResources(State& s, int p, const vector<int>& req, vector<int>& seq) {
    // Step 1: request must not exceed the process's declared need
    for (int j = 0; j < s.m; j++)
        if (req[j] > s.need[p][j]) return EXCEEDS_NEED;
    // Step 2: resources must be available right now
    for (int j = 0; j < s.m; j++)
        if (req[j] > s.avail[j]) return NOT_AVAILABLE;
    // Step 3: pretend to allocate (save a backup for rollback)
    State backup = s;
    for (int j = 0; j < s.m; j++) {
        s.avail[j] -= req[j];
        s.alloc[p][j] += req[j];
        s.need[p][j] -= req[j];
    }
    // Step 4: safety check on the new state
    if (isSafe(s, seq)) return GRANTED;
    s = backup;                      // ROLLBACK: state was unsafe
    return UNSAFE_ROLLBACK;
}

// If a process has everything it needs, it finishes and returns its resources.
bool releaseIfFinished(State& s, int p) {
    for (int j = 0; j < s.m; j++)
        if (s.need[p][j] != 0) return false;
    bool holdsSomething = false;
    for (int j = 0; j < s.m; j++) {
        if (s.alloc[p][j] > 0) holdsSomething = true;
        s.avail[j] += s.alloc[p][j];
        s.alloc[p][j] = 0;
        s.maxm[p][j] = 0;
    }
    return holdsSomething;
}

// Runs one request, prints result, logs it. Returns the result.
Result processRequest(int p, const vector<int>& req) {
    vector<int> seq;
    Result r = requestResources(sys, p, req, seq);
    string line = "P" + to_string(p) + " requests " + vecStr(req) + " -> ";
    switch (r) {
        case GRANTED:
            line += "GRANTED (safe state), safe sequence: " + seqStr(seq);
            cout << GREEN << line << RESET << "\n";
            break;
        case EXCEEDS_NEED:
            line += "DENIED (request exceeds maximum need)";
            cout << RED << line << RESET << "\n";
            break;
        case NOT_AVAILABLE:
            line += "DENIED (resources not available, process must wait)";
            cout << RED << line << RESET << "\n";
            break;
        case UNSAFE_ROLLBACK:
            line += "DENIED (would lead to UNSAFE state) -> ROLLED BACK";
            cout << RED << line << RESET << "\n";
            break;
    }
    logMsg(line);
    if (r == GRANTED && releaseIfFinished(sys, p)) {
        string rel = "P" + to_string(p) + " has its full need, finishes and releases resources. Available now " + vecStr(sys.avail);
        cout << CYAN << rel << RESET << "\n";
        logMsg(rel);
    }
    return r;
}

// ---------- data loading ----------
void loadTextbookExample() {
    sys.n = 5;
    sys.m = 3;
    sys.alloc = {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};
    sys.maxm = {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};
    sys.avail = {3, 3, 2};
    sys.computeNeed();
    cout << "Loaded textbook example (5 processes, 3 resource types A,B,C).\n";
    logMsg("--- Loaded textbook example ---");
}

// ---------- EXTRA: interactive input ----------
bool readInt(int& x, int lo, int hi) {
    if (!(cin >> x) || x < lo || x > hi) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid input. Enter a number between " << lo << " and " << hi << ".\n";
        return false;
    }
    return true;
}

void inputCustomData() {
    int n, m;
    cout << "Number of processes (1-50): ";
    if (!readInt(n, 1, 50)) return;
    cout << "Number of resource types (1-10): ";
    if (!readInt(m, 1, 10)) return;
    State t;
    t.n = n;
    t.m = m;
    t.alloc.assign(n, vector<int>(m));
    t.maxm.assign(n, vector<int>(m));
    t.avail.assign(m, 0);
    cout << "Enter Allocation matrix (" << n << " rows x " << m << " cols):\n";
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            while (!readInt(t.alloc[i][j], 0, 100000)) {}
    cout << "Enter Max matrix (" << n << " rows x " << m << " cols):\n";
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            while (!readInt(t.maxm[i][j], 0, 100000)) {}
    cout << "Enter Available vector (" << m << " values):\n";
    for (int j = 0; j < m; j++)
        while (!readInt(t.avail[j], 0, 100000)) {}
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            if (t.alloc[i][j] > t.maxm[i][j]) {
                cout << RED << "Error: Allocation exceeds Max for P" << i << ". Data rejected.\n" << RESET;
                return;
            }
    t.computeNeed();
    sys = t;
    logMsg("--- Custom data entered by user ---");
    cout << "Data loaded.\n";
}

void interactiveRequest() {
    if (sys.n == 0) { cout << "Load data first.\n"; return; }
    int p;
    cout << "Process number (0-" << sys.n - 1 << "): ";
    if (!readInt(p, 0, sys.n - 1)) return;
    vector<int> req(sys.m);
    cout << "Request vector (" << sys.m << " values): ";
    for (int j = 0; j < sys.m; j++)
        while (!readInt(req[j], 0, 100000)) {}
    processRequest(p, req);
}

// ---------- EXTRA: randomized resource requests ----------
void randomSimulation() {
    if (sys.n == 0) { cout << "Load data first.\n"; return; }
    int count;
    cout << "How many random requests? (1-100): ";
    if (!readInt(count, 1, 100)) return;
    logMsg("--- Random simulation of " + to_string(count) + " requests ---");
    int granted = 0, denied = 0;
    for (int k = 0; k < count; k++) {
        int p = uniform_int_distribution<int>(0, sys.n - 1)(rng);
        vector<int> req(sys.m);
        for (int j = 0; j < sys.m; j++) {
            // sometimes ask for more than the need, to test denial
            int hi = sys.need[p][j] + (uniform_int_distribution<int>(0, 9)(rng) == 0 ? 1 : 0);
            req[j] = uniform_int_distribution<int>(0, hi)(rng);
        }
        if (processRequest(p, req) == GRANTED) granted++; else denied++;
    }
    cout << "\nSummary: " << granted << " granted, " << denied << " denied.\n";
    logMsg("Summary: " + to_string(granted) + " granted, " + to_string(denied) + " denied");
    vector<int> seq;
    bool safe = isSafe(sys, seq);
    cout << "Final state is " << (safe ? GREEN + "SAFE" : RED + "UNSAFE") << RESET << "\n";
}

// ---------- EXTRA: performance analysis ----------
// Builds a "worst case" state where only one process can finish per pass,
// which shows the O(m * n^2) cost of the safety algorithm.
State makeWorstCase(int n, int m) {
    State s;
    s.n = n; s.m = m;
    s.alloc.assign(n, vector<int>(m, 0));
    s.maxm.assign(n, vector<int>(m, 0));
    s.avail.assign(m, 0);
    for (int i = 0; i < n; i++) {
        s.alloc[i][0] = 1;
        s.maxm[i][0] = 1 + (n - i);   // need[i][0] = n - i
    }
    s.avail[0] = 1;
    s.computeNeed();
    return s;
}

State makeRandomState(int n, int m) {
    State s;
    s.n = n; s.m = m;
    s.alloc.assign(n, vector<int>(m));
    s.maxm.assign(n, vector<int>(m));
    s.avail.assign(m, 0);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            s.maxm[i][j] = uniform_int_distribution<int>(1, 10)(rng);
            s.alloc[i][j] = uniform_int_distribution<int>(0, s.maxm[i][j])(rng);
        }
    for (int j = 0; j < m; j++) s.avail[j] = uniform_int_distribution<int>(5, 15)(rng);
    s.computeNeed();
    return s;
}

double timeSafety(const State& s, int repeats) {
    vector<int> seq;
    auto t0 = chrono::steady_clock::now();
    for (int r = 0; r < repeats; r++) isSafe(s, seq);
    auto t1 = chrono::steady_clock::now();
    return chrono::duration<double, micro>(t1 - t0).count() / repeats;
}

void performanceAnalysis() {
    const int m = 5;
    vector<int> sizes = {10, 50, 100, 200, 400, 800, 1600};
    ofstream csv("perf.csv");
    csv << "processes,resources,random_us,worst_case_us\n";
    cout << "\n" << left << setw(12) << "Processes" << setw(12) << "Resources"
         << setw(18) << "Random (us)" << "Worst case (us)\n";
    for (int n : sizes) {
        double tr = timeSafety(makeRandomState(n, m), 50);
        double tw = timeSafety(makeWorstCase(n, m), 5);
        csv << n << "," << m << "," << tr << "," << tw << "\n";
        cout << setw(12) << n << setw(12) << m << setw(18) << fixed << setprecision(2) << tr << tw << "\n";
    }
    csv.close();
    cout << "Saved to perf.csv - open it in Excel and make a line chart for your report.\n";
    logMsg("--- Performance analysis saved to perf.csv ---");
}

void viewLog() {
    ifstream in("log.txt");
    if (!in) { cout << "No log yet.\n"; return; }
    string line;
    cout << "\n=== log.txt ===\n";
    while (getline(in, line)) cout << line << "\n";
}

// ---------- menu ----------
int main() {
    logFile.open("log.txt", ios::app);
    logMsg("===== New session =====");
    loadTextbookExample();   // start with a ready example

    while (true) {
        cout << "\n" << CYAN << "===== BANKER'S ALGORITHM SIMULATOR =====" << RESET << "\n"
             << "1. Load textbook example\n"
             << "2. Enter custom data\n"
             << "3. Show system state\n"
             << "4. Run safety check\n"
             << "5. Make a resource request\n"
             << "6. Random request simulation\n"
             << "7. Performance analysis (creates perf.csv)\n"
             << "8. View log\n"
             << "0. Exit\n"
             << "Choice: ";
        int c;
        if (!readInt(c, 0, 8)) continue;
        if (c == 0) break;
        switch (c) {
            case 1: loadTextbookExample(); break;
            case 2: inputCustomData(); break;
            case 3: if (sys.n) printState(sys); else cout << "No data loaded.\n"; break;
            case 4: {
                if (!sys.n) { cout << "No data loaded.\n"; break; }
                vector<int> seq;
                if (isSafe(sys, seq)) {
                    string l = "Safety check: SAFE, sequence " + seqStr(seq);
                    cout << GREEN << l << RESET << "\n";
                    logMsg(l);
                } else {
                    string l = "Safety check: UNSAFE (no safe sequence exists)";
                    cout << RED << l << RESET << "\n";
                    logMsg(l);
                }
                break;
            }
            case 5: interactiveRequest(); break;
            case 6: randomSimulation(); break;
            case 7: performanceAnalysis(); break;
            case 8: viewLog(); break;
        }
    }
    logMsg("===== Session ended =====");
    cout << "Goodbye.\n";
    return 0;
}
