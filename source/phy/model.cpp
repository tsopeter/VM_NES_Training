#include "model.hpp"
#include <cmath>

Phy_Model::Phy_Model (Phy_Model_Parameters params) 
: m_params(params)
{
    torch::manual_seed(42);

    /* Initialize m_parameter */
    m_parameter = torch::rand({m_params.parameter_H, m_params.parameter_W}) * 2 * M_PI - M_PI;
    m_parameter.set_requires_grad(true);

    m_number_of_actions = 0;
    m_action_counter    = 0;

    /* Set VonMises */
    m_von_mises.set_mu(m_parameter, m_params.kappa);
}

Phy_Model::~Phy_Model () {

}

torch::Tensor Phy_Model::logp_action() {
    /*Return logp from m_von_mises with action */

    /* Make sure that m_von_mises uses the correct m_parameter */
    m_von_mises.set_mu(m_parameter, m_params.kappa);

    return m_von_mises.log_prob(m_action);
}

bool Phy_Model::generate_actions (int64_t number_of_actions) {
    /* Make sure that actions are generated without
       autograd */
    torch::NoGradGuard no_grad;

    if (!is_actions_depleted()) return false;   /* Actions must be depleted before generating new actions */

    /* Input must be a multiple of the number of bits per frame used */
    assert (number_of_actions % m_params.bits_per_frame == 0);

    /* Generate N number of actions */
    m_action = m_von_mises.sample(number_of_actions);

    m_number_of_actions = number_of_actions;
    m_action_counter    = number_of_actions;
    return true;
}

void Phy_Model::refill_actions_without_sampling () {
    m_action_counter = m_number_of_actions;
}

bool Phy_Model::is_actions_depleted () const {
    return m_action_counter <= 0;
}

torch::Tensor Phy_Model::get_actions_sequentially () {
    torch::Tensor action = m_action.index({
        torch::indexing::Slice(m_number_of_actions - m_action_counter,
                               m_number_of_actions - m_action_counter + m_params.bits_per_frame)
    });

    m_action_counter -= m_params.bits_per_frame;
    return action;
}

int64_t Phy_Model::N_samples () const {
    return m_number_of_actions;
}

std::vector<torch::Tensor> Phy_Model::parameters () {
    return {m_parameter};
}