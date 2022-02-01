#include <fstream>
#include <chrono>

void print_to_file(std::chrono::steady_clock::time_point begin,
                   std::chrono::steady_clock::time_point end,
                   char * fileName) {
    std::ofstream outfile;
    outfile.open(fileName, std::ios_base::app);
    outfile << "Elapsed[ms] " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "\n";
    outfile << "Elapsed[µs] " << std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count() << "\n";
}