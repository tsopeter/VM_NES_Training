#include <torch/torch.h>
#include <iostream>

int main () {
    int x = (torch::cuda::is_available()) ? 0 : -1;
    std::cout << "CUDA is " << ((x != 0) ? "Not" : "") << " available.\n";
    return x;
}