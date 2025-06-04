#include "np2lt.hpp"
#include <cnpy.h>

#define LOAD_DEFINITION(fname, type, ktype)                                             \
    torch::Tensor np2lt::fname (const std::filesystem::path &path) {                   \
            cnpy::NpyArray npy_data = cnpy::npy_load(path.string());                   \
            std::vector<int64_t> shape(npy_data.shape.begin(), npy_data.shape.end());  \
            at::IntArrayRef sizes(shape);                                              \
            type* loaded_data = npy_data.data<type>();                                 \
            auto tensor = torch::from_blob(loaded_data, sizes, torch::ktype).clone();  \
            return tensor;                                                             \
    }    

LOAD_DEFINITION(f32, float, kFloat32);
LOAD_DEFINITION(f64, double, kFloat64);
LOAD_DEFINITION(i32, int32_t, kInt32);
LOAD_DEFINITION(i64, int64_t, kInt64);
LOAD_DEFINITION(u32, uint32_t, kUInt32);
LOAD_DEFINITION(u64, uint64_t, kUInt64);