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

void print_to_file(std::vector<double> measurementsVec, char * fileName, std::string description) {
    for (double measurement: measurementsVec) {
        long measurementMs = round(measurement)*1.0e-6;
        long measurementMcs = round(measurement)*1.0e-3;

        std::ofstream outfile;
        outfile.open(fileName, std::ios_base::app);
        outfile << description << "[ms] " << measurementMs << "\n";
        outfile << description << "[µs] " << measurementMcs << "\n\n";
    }
}

void print_avg_to_file(std::vector<double> measurementsVec, char * fileName, std::string description) {
    long long measurementMicros = 0.0;
    for (double measurement: measurementsVec) {
        measurementMicros += round(measurement)/1000;
    }
    long long measurementMs = measurementMicros/1000;
    
    int countOfMeasurements = measurementsVec.size();

    std::ofstream outfile;
    outfile.open(fileName, std::ios_base::app);

    outfile << description << "[ms] " << measurementMs/countOfMeasurements << "\n";
    outfile << description << "[µs] " << measurementMicros/countOfMeasurements << "\n";
}

void print_total_write_avg_to_file(std::vector<double> measurementsVec, char * fileName, std::string description) {
    
    long long measurementMicros = 0.0;
    for (int i = 0; i < measurementsVec.size(); i += 2) {
        double writeP = round(measurementsVec[i]);
        double writeRhs = round(measurementsVec[i+1]);
        measurementMicros += (writeP+writeRhs)/1000;
    }
    long long measurementMs = measurementMicros/1000;

    int countOfMeasurements = measurementsVec.size()/2;

    // repeated code, refactor later
    std::ofstream outfile;
    outfile.open(fileName, std::ios_base::app);

    outfile << description << "[ms] " << measurementMs/countOfMeasurements << "\n";
    outfile << description << "[µs] " << measurementMicros/countOfMeasurements << "\n";
}

