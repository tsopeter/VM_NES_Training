#include "csv.hpp"


CSV::CSV (std::string filename) {
    this->filename = filename;
}

CSV::~CSV () {

}

void CSV::add_headers (const std::vector<std::string> &header) {
    std::ofstream ofs(filename, std::ios::out);
    if (!ofs) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    // If nothing is given, we assume that the
    // user will use perf
    if (header.empty()) {
        ofs << "Loss,Compute_Time_s\n";
        ofs.close();
        return;
    }

    for (size_t i = 0; i < header.size(); ++i) {
        ofs << header[i];
        if (i < header.size() - 1) {
            ofs << ",";
        }
    }
    ofs << "\n";
    ofs.close();
}

void CSV::write (const Helpers::Performance &perf) {
    std::ofstream ofs(filename, std::ios::out | std::ios::app);
    if (!ofs) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    ofs << perf.loss << "," << perf.compute_time_s << "\n";
    ofs.close();
}