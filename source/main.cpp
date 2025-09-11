#include <iostream>
#include "examples/e1.hpp"
#include "examples/e2.hpp"
#include "examples/e15.hpp"
#include "examples/e13.hpp"
#include "examples/e16.hpp"
#include "examples/e17.hpp"
#include "examples/e18.hpp"
#include "examples/e19.hpp"
#include "examples/e20.hpp"
#include "examples/e21.hpp"
#include "examples/e22.hpp"
#include "examples/e23.hpp"
#include "examples/e24.hpp"
#include "examples/e25.hpp"
#include "examples/e26.hpp"
#include "examples/e27.hpp"
#include "examples/e28.hpp"
#include "s3/IP.hpp"
#include "utils/utils.hpp"
#include "s3/window.hpp"
#include "raylib.h"
#include "s5/viewer.hpp"

std::ostream& operator<<(std::ostream &os, const Utils::data_structure &ds) {
    return os << "Step: " << ds.iteration << ", Total Rewards: " << ds.total_rewards << '\n';
}

void run_code () {
#if defined(MACHINE)
    //e22(); //e17();    // Synchronization Test
    e23(); //e18();
    //e25(); // PLM working test
    //e26(); // New Sync Test
    //e27(); 
    //e28();
#elif defined(VIEWER)   // everything else is used as the machine to display results
    Viewer viewer(240*2, 320*2, 9001);
    viewer.run();
#endif
}

int main () {
   run_code ();
}
