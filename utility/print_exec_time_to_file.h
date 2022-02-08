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

void print_average_to_file(std::vector<double> measurementsVec, char * fileName, std::string description) {
    long long measurementMs = 0.0;
    long long measurementMcs = 0.0;
    for (double measurement: measurementsVec) {
        measurementMs += round(measurement)*1.0e-6;
        measurementMcs += round(measurement)*1.0e-3;
    }
    int countOfMeasurements = measurementsVec.size();

    std::ofstream outfile;
    outfile.open(fileName, std::ios_base::app);

    outfile << "\n" << description << "[ms] " << measurementMs/countOfMeasurements << "\n";
    outfile << description << "[µs] " << measurementMcs/countOfMeasurements << "\n\n";
}