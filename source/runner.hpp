#ifndef runner_hpp__
#define runner_hpp__

#include "s5/scheduler2.hpp"
#include "s4/model.hpp"
#include "s4/optimizer.hpp"
#include "s5/distributions.hpp"
#include "s5/helpers.hpp"
#include "s5/utils.hpp"
#include <iostream>
#include <fstream>
#include <ostream>
#include <vector>

class Runner {
public:
enum DistributionType {
        NORMAL,
        CATEGORICAL,
        BINARY
};

int ModelHeight = 400;
int ModelWidth  = 640;
int n_epochs    = 50;
DistributionType ModelDistribution = DistributionType::NORMAL;
std::string checkpoint_directory = "./checkpoints/";
Helpers::Parameters params;

struct ConfigKeyMap {
    std::string      name;
    std::function<void(std::ifstream&)> setter;
};

std::vector<ConfigKeyMap> config_key_map = {};
void InitConfigKeyMap ();


void ParseConfigFile (const std::string &filename);
void ExportConfig    (const std::string &filename);

void SaveCheckpoint  (int epoch, torch::Tensor mask , const std::string &directory);

class Model : public s4_Model {
public:
    Model ();
    ~Model () override;

    void init (int64_t Height, int64_t Width, int64_t n, DistributionType dist_type);
    void init (torch::Tensor, DistributionType dist_type);
    torch::Tensor sample (int n);
    void squash ();
    torch::Tensor logp_action () override;
    std::vector<torch::Tensor> parameters ();
    torch::Tensor &get_parameters ();
    int64_t N_samples () const override;



    Distributions::Definition *m_dist = nullptr;
    torch::Tensor m_parameter;
    torch::Tensor m_action;
    std::vector<torch::Tensor> m_action_s;
    int64_t m_Height;
    int64_t m_Width;
    int64_t m_n;
    double std; /* May be unused */

};

bool m_inference = false;
bool m_load_checkpoint = false;
int  m_checkpoint_epoch = 0;
void LoadCheckpointFile (const std::string &directory);
torch::Tensor m_checkpoint_mask; 
double m_std;
s5_Utils::Affine m_affine_params;
std::string m_checkpoint_mask_location;

int m_sub_texture_offset_h = 0;
int m_sub_texture_offset_w = 0;
bool m_auto = false;

double m_sub_texture_scale_h = 1.0;
double m_sub_texture_scale_w = 1.0;

void Run (std::string config_file);
void Inference (std::string config_file, s2_DataTypes data_type, int n_data_points);
void StaticInference (std::string config_file, s2_DataTypes data_type, int n_data_points);

void PopulateNewDirectory (const std::string &directory);
bool TestIfScreenIsOkay (Helpers::Parameters &, Scheduler2&);

struct Time {
    double hours;
    double minutes;
    double seconds;

    std::string to_string () const;
};

void Alignment ();


Time GetCurrentTime ();

};


#endif
