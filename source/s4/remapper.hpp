#ifndef remapper_hpp__
#define remapper_hpp__

#include <torch/torch.h>

class Remapper {
public:
    Remapper  ();
    Remapper  (torch::Tensor map);
    ~Remapper ();

    torch::Tensor remap (torch::Tensor input);
    void set_map (torch::Tensor map);

private:

    torch::Tensor m_map;


};

#endif
