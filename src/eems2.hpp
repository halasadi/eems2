#pragma once

#include "util.hpp"
#include "mcmc.hpp"
#include "draw.hpp"
//#include "<omp.h>"

#include "graph.hpp"
#include "habitat.hpp"

#ifndef EEMS2_H
#define EEMS2_H

/*
 An updated set of parameter values
 The type of move is necessary in order to know which parameters have a new proposed value;
 the rest of the parameters won't be set to their current values (to avoid unnecessary copying)
 For example, if move = M_VORONOI_BIRTH_DEATH,
 then newmtiles, newmSeeds, nowmColors, newmEffcts (and of course, newpi, newll, newratioln) would have changed
 if move = M_VORONOI_POINT_MOVE,
 then newmSeeds, nowmColors (and of course, newpi, newll) would have changed
 The ratioln is the proposal ratio for birth/death proposal.
 For the usual Metropolis-Hastings updates, the acceptance probability is
 alpha = (prior ratio) * (likelihood ratio)
 For the birth/deatch RJ-MCMC updates, the acceptance probability is
 alpha = (proposal ratio) * (prior ratio) * (likelihood ratio)
 See Green, "Reversible jump Markov chain Monte Carlo computation and Bayesian model determination"
 */
struct Proposal {
    MoveType move; // the type of proposal/update
    int newqtiles; // number of m and q tiles, respectively
    int newmtiles;
    double newdf; // degrees of freedom
    double newpi; // log prior
    double newll; // log likelihood
    double newratioln; // RJ-MCMC proposal ratio, on the log scale
    double newmrateMu; // overall (mean) migration rate,
    double newqrateMu;
    
    VectorXd newqEffcts; // the diversity rate of each q tile
    VectorXd newmEffcts; // the migration rate of each m tile, relative to the ovarall mrateMu
    MatrixXd newqSeeds;  // the location of each q tile within the habitat
    MatrixXd newmSeeds;  // the location of each m tile within the habitat
    
};


struct Chain {
    
    double Temperature; // temperature of the chain
    int qtiles; // number of m and q tiles, respectively
    int mtiles;
    double df; // degrees of freedom
    double pi; // log prior
    double ll; // log likelihood
    double mrateMu; // overall (mean) migration rate,
    double qrateMu;
    
    VectorXd qEffcts; // the diversity rate of each q tile
    VectorXd mEffcts; // the migration rate of each m tile, relative to the ovarall mrateMu
    MatrixXd qSeeds;  // the location of each q tile within the habitat
    MatrixXd mSeeds;  // the location of each m tile within the habitat
    
    // update using Gibbs Sampling
    double mrateS2;
    double qrateS2;
    
};

class EEMS2 {
public:
    
    EEMS2(const Params &params);
    ~EEMS2( );
    
    void initialize_state( );
    void load_final_state( );
    bool start_eems(const MCMC &mcmc);
    double eval_prior(const MatrixXd &mSeeds, const VectorXd &mEffcts, const double mrateMu, const double mrateS2,
                      const MatrixXd &qSeeds, const VectorXd &qEffcts, const double qrateMu, const double qrateS2,
                      const double df) const;
    double eems2_likelihood(MatrixXd newmSeeds, MatrixXd newqSeeds, VectorXd newmEffcts,
                            VectorXd newqEffcts, double newmrateMu, double newdf, bool ismUpdate) const;
    
    void calculateIntegral(MatrixXd &eigenvals, MatrixXd &eigenvecs, const VectorXd &q, MatrixXd &integral, double bnd) const;
    
    MoveType choose_move_type( );
    
    // Gibbs updates:
    void update_hyperparams(int chainIndex );
    // Random-walk Metropolis-Hastings proposals:
    void propose_df(Proposal &proposal, int chainIndex);
    void propose_rate_one_qtile(Proposal &proposal, int chainIndex);
    void propose_rate_one_mtile(Proposal &proposal, int chainIndex);
    void propose_overall_mrate(Proposal &proposal, int chainIndex);
    void propose_overall_qrate(Proposal &proposal, int chainIndex);
    void propose_move_one_qtile(Proposal &proposal, int chainIndex);
    void propose_move_one_mtile(Proposal &proposal, int chainIndex);
    void propose_birthdeath_qVoronoi(Proposal &proposal, int chainIndex);
    void propose_birthdeath_mVoronoi(Proposal &proposal, int chainIndex);
    bool accept_proposal(Proposal &proposal, int chainIndex);
    
    void print_iteration(const MCMC &mcmc, int chainIndex) const;
    void save_iteration(const MCMC &mcmc, int chainIndex);
    bool output_results(const MCMC &mcmc, int chainIndex) const;
    bool output_current_state(int chainIndex) const;
    void check_ll_computation(int chainIndex) const;
    string datapath() const;
    string mcmcpath() const;
    string prevpath() const;
    string gridpath() const;
    
    double getMigrationRate(const int edge, int chainIndex) const;
    double getCoalescenceRate(const int deme, int chainIndex) const;
    int getnChains() const;
    int getll(int chainIndex) const ;
    double getTemperature(int chainIndex) const ;
    void printMigrationAndCoalescenceRates(int chainIndex) const;
    void writePopSizes(int chainIndex) const;
    void EEMS2:transferChain(int donorIndex, int receiptIndex);
    
    
private:
    
    Draw draw; // Random number generator
    Graph graph;
    Params params;
    Habitat habitat;
    
    const int nChains = 5;
    Chain chains[nChains];
    
    int o; // number of observed demes
    int d; // total number of demes
    int n; // number of samples
    MatrixXd observedIBD; // observed means (for number of IBD blocks)
    MatrixXd cMatrix; // number of pairwise observations between observed populations
    VectorXd cvec; // c is the vector of counts
    VectorXd cClasses; // cClasses is a vector of count of the number of 0's, number of 1's, etc. For likelihood;
    double maxCnt; // the maximum number of IBD segments shared (over all pairs)
    
    MatrixXd JtDhatJ;
    mutable MatrixXd expectedIBD;
    mutable MatrixXd eigenvals;
    mutable MatrixXd eigenvecs;
    
    
    // Variables to store the results in:
    // Fixed size:
    
    MatrixXd mcmcmhyper;
    MatrixXd mcmcqhyper;
    MatrixXd mcmcthetas;
    MatrixXd mcmcpilogl;
    VectorXd mcmcmtiles;
    VectorXd mcmcqtiles;
    // Variable length:
    vector<double> mcmcmRates;
    vector<double> mcmcqRates;
    vector<double> mcmcxCoord;
    vector<double> mcmcyCoord;
    vector<double> mcmcwCoord;
    vector<double> mcmczCoord;
    
    void initialize_sims();
    void randpoint_in_habitat(MatrixXd &Seeds);
    void rnorm_effects(const double lowerBnd, const double upperBnd, const double rateS2, VectorXd &Effcts);
    
    double eems2_likelihood(const MatrixXd &mSeeds, const VectorXd &mEffcts, const double mrateMu,
                            const MatrixXd &qSeeds, const VectorXd &qEffcts,
                            const double df, const double qrateMu, const bool ismUpdate) const;
    
};

#endif
