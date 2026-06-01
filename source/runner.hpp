#ifndef runner_hpp__
#define runner_hpp__

#include "s5/scheduler2.hpp"
#include "s4/model.hpp"
#include "s4/optimizer.hpp"
#include "s5/distributions.hpp"
#include "s5/helpers.hpp"
//#include "s5/utils.hpp"
#include "s2/plm_device.hpp"
//#include "hook.hpp"
#include <iostream>
#include <fstream>
#include <ostream>
#include <vector>

class Runner {
public:
    void Run (std::string config_file);

    void InitConfigKeyMap ();
    void ParseConfigFile (const std::string &config_file);

private:
    enum DistributionType {
            NORMAL,
            CATEGORICAL
    };
    DistributionType model_distribution = DistributionType::NORMAL;

    struct ConfigKeyMap {
        std::string      name;
        std::function<void(std::ifstream&)> setter;
    };
    std::vector<ConfigKeyMap> config_key_map = {};

    class Model : public s4_Model {
    public:
        Model ();
        ~Model () override;

        void init (int64_t Height, int64_t Width, int64_t n, DistributionType dist_type, int num_levels=16);
        void init (torch::Tensor, DistributionType dist_type);
        torch::Tensor sample (int n);
        void squash ();
        torch::Tensor logp_action () override;
        torch::Tensor action () override;
        std::vector<torch::Tensor> parameters () override;
        torch::Tensor &get_parameters ();
        int64_t N_samples () const override;
        void set_definition (Distributions::Definition* def) override;
        Distributions::Definition* get_definition () override;


        DistributionType m_model_distribution;
        Distributions::Definition *m_dist = nullptr;
        torch::Tensor m_parameter;
        torch::Tensor m_parameter_std; /* Only used for Normal2 */
        torch::Tensor m_std;
        torch::Tensor m_action;
        std::vector<torch::Tensor> m_action_s;
        int64_t m_Height;
        int64_t m_Width;
        int64_t m_n;
        double std; /* May be unused */
    };
    Model model;
    int   model_height, model_width;


    Scheduler2 scheduler;

    std::string checkpoint_directory = "./checkpoints/";
    Helpers::Parameters params;

    std::string output_path = "output";
};


#endif
