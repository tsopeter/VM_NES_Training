#include "e29.hpp"

#include <iostream>
#include <torch/torch.h>
#include "../s5/distributions.hpp"
#include "../s4/pencoder.hpp"

int e29 () {
    int H = 28;
    int W = 28;
    int n_classes = 2;

    PEncoder pen (0, 0, 2 * H, 2 * W);

    torch::Tensor logits = torch::randn({H, W, n_classes}).squeeze();
    //Distributions::Categorical dist = Distributions::Categorical (logits);
    Distributions::Binary dist = Distributions::Binary (logits);

    auto z = dist.sample(10);
    std::cout << z.sizes() << '\n';

    auto t = pen.MEncode_u8Tensor_Binary(z);
    std::cout << t.sizes() << '\n';

    auto log_prob = dist.log_prob(z);
    std::cout << log_prob.sizes() << '\n';

    std::cout << z << '\n';


    return 0;
}