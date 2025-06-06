#ifndef s4_slicer_hpp__
#define s4_slicer_hpp__

#include <iostream>
#include <array>
#include <memory>

#include <torch/torch.h>
#include "../s3/cam.hpp"   /* u8Image */
#include <raylib.h>

class s4_Slicer_Region {
public:
    virtual ~s4_Slicer_Region () = default;
    virtual torch::Tensor get_region (int,int) = 0;
};

class s4_Slicer_Circle : public s4_Slicer_Region {
public:
    s4_Slicer_Circle (int x, int y, int r);
    ~s4_Slicer_Circle () override;

    torch::Tensor get_region (int,int) override;
private:
    int m_x, m_y, m_r;
};

class s4_Slicer_Square : public s4_Slicer_Region {
public:
    s4_Slicer_Square (int x, int y, int r);
    ~s4_Slicer_Square () override;
    
    torch::Tensor get_region (int,int) override;
private:
    int m_x, m_y, m_r;
};

using s4_Slicer_Region_Vector = 
    std::vector<std::shared_ptr<s4_Slicer_Region>>;

class s4_Slicer {
public:

    /**
     * @brief s4_Slicer is used to generate the target mask for the neural network.
     * 
     */
    s4_Slicer ();

    /**
     * @brief s4_Slicer is used to generate the target mask for the neural network.
     *        
     * @param s4_Slicer_Region_Vector is of type std::vector<std::shared_ptr<s4_Slicer_Region>>, which is used to easily
     *        construct regions for detection at runtime.
     * 
     */
    s4_Slicer (s4_Slicer_Region_Vector, int, int);

    /**
     * @brief s4_Slicer is used to generate the target mask for the neural network.
     *        
     * @param [Tensor] Vector of tensors that denote each region. Tensors must be the same size
     * 
     */
    s4_Slicer (std::vector<torch::Tensor>);

    /**
     * @brief s4_Slicer is used to generate the target mask for the neural network.
     *        
     * @param [u8Image] Vector of 8-bit images that denote each region. Each image must be the same size
     * 
     */
    s4_Slicer (std::vector<u8Image>);

    /**
     * @brief s4_Slicer is used to generate the target mask for the neural network.
     *        
     * @param Tensor A Tensor must have shape [N,...] for N-classes
     * 
     */
    s4_Slicer (torch::Tensor);
    ~s4_Slicer ();

    /**
     * @brief detect is used to detect regions in a image to get the response
     * 
     */
    torch::Tensor detect (torch::Tensor);

    /**
     * @brief detect is used to detect regions in a image to get the response
     * 
     */
    torch::Tensor detect (u8Image);

    /**
     * @brief detect is used to detect regions in a image to get the response
     * 
     */
    torch::Tensor detect (std::vector<u8Image>);

    /**
     * @brief detect is used to detect regions in a image to get the response
     * 
     */
    torch::Tensor detect (std::vector<torch::Tensor>);

    /**
     * @brief Visualizes the region as an image
     * 
     */
    Image visualize ();

    /**
     * @brief sets device of regions
     * 
     */
    void device (torch::Device);

private:
    torch::Tensor m_regions;
};

#endif