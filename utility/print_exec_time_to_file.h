#include <fstream>
#include <chrono>
#include <string>
#include <vector>
#include <cmath>

void print_to_file(std::chrono::steady_clock::time_point begin,
                   std::chrono::steady_clock::time_point end,
                   char * fileName, std::string description) {
    std::ofstream outfile;
    outfile.open(fileName, std::ios_base::app);
    outfile << description << "[ms] " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "\n";
    outfile << description << "[µs] " << std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count() << "\n";
}

void print_to_file(std::chrono::steady_clock::time_point begin,
                   std::chrono::steady_clock::time_point end,
                   char * fileName) {
    std::string description = "Elapsed";
    print_to_file(begin, end, fileName, description);
}

void print_new_line_to_file(char * fileName) {
    std::ofstream outfile;
    outfile.open(fileName, std::ios_base::app);
    outfile << "\n";
}

void print_total_to_file(std::vector<double> measurementsVec, char * fileName, std::string description) {
    long long measurementMicros = 0.0;
    for (double measurement: measurementsVec) {
        measurementMicros += round(measurement)/1000;
    }
    long long measurementMs = measurementMicros/1000;

    std::ofstream outfile;
    outfile.open(fileName, std::ios_base::app);

    outfile << description << "[ms] " << measurementMs << "\n";
    outfile << description << "[µs] " << measurementMicros << "\n";
}