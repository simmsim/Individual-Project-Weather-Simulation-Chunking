#include <fstream>
#include <chrono>
#include <string>

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