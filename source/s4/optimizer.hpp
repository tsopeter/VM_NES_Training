#ifndef s4_optimizer_hpp__
#define s4_optimizer_hpp__

#include <torch/torch.h>

class s4_Optimizer {
public:
    s4_Optimizer ();
    ~s4_Optimizer ();
private:

    void step (torch::Tensor &loss);

};

#endif

