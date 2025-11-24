#ifndef s4_model_hpp__
#define s4_model_hpp__

#include <cstdint>
#include <torch/torch.h>
#include "../s5/distributions.hpp"

class s4_Model {
public:
    virtual ~s4_Model () = default;
    virtual torch::Tensor logp_action () = 0;
    virtual int64_t N_samples () const = 0;
    virtual void set_definition (Distributions::Definition* def) = 0;
    virtual Distributions::Definition* get_definition () = 0;
};

#endif
