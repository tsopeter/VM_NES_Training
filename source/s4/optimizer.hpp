#ifndef s4_optimizer_hpp__
#define s4_optimizer_hpp__

#include <torch/torch.h>

class s4_Optimizer {
public:
    Optimizer ();
    ~Optimizer ();
private:

    void step (torch::Tensor &loss);

};

#endif

