#ifndef csv_hpp__
#define csv_hpp__

#include "../s5/helpers.hpp"
#include <iostream>
#include <fstream>

class CSV {
public:
    CSV (std::string filename);
    ~CSV ();

    void add_headers (const std::vector<std::string> &header);
    void write (const Helpers::Performance &perf);
    void write (const std::vector<std::string> &row);

private:
    std::string filename;
};



#endif /* csv_hpp__ */