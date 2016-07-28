
#include "eems2.hpp"

// The distance metric is a global variable, so that
// the pairwise_distance function can see it
// Choose 'euclidean' (the default) or 'greatcirc' (great circle distance)
string dist_metric;


void proposeMove(EEMS2 &eems2, Proposal &proposal, int chain){
    switch ( eems2.choose_move_type(chain) ) {
        case Q_VORONOI_BIRTH_DEATH:
            eems2.propose_birthdeath_qVoronoi(proposal);
            break;
        case M_VORONOI_BIRTH_DEATH:
            eems2.propose_birthdeath_mVoronoi(proposal);
            break;
        case Q_VORONOI_POINT_MOVE:
            eems2.propose_move_one_qtile(proposal);
            break;
        case M_VORONOI_POINT_MOVE:
            eems2.propose_move_one_mtile(proposal);
            break;
        case Q_VORONOI_RATE_UPDATE:
            eems2.propose_rate_one_qtile(proposal);
            break;
        case M_VORONOI_RATE_UPDATE:
            eems2.propose_rate_one_mtile(proposal);
            break;
        case M_MEAN_RATE_UPDATE:
            eems2.propose_overall_mrate(proposal);
            break;
        case Q_MEAN_RATE_UPDATE:
            eems2.propose_overall_qrate(proposal);
            break;
        case DF_UPDATE:
            eems2.propose_df(proposal);
            break;
        case CHAIN_SWAP:
            eems2.propose_chain_swap(proposal);
            break;
        default:
            cerr << "[RunEEMS2] Unknown move type" << endl;
            exit(EXIT_FAILURE);
    }
    
}

int main(int argc, char** argv)
{
    try {
        
        long seed_from_command_line = 1L;
        string params_file; bool error;
        
        po::options_description options("EEMS2 options from command line");
        po::variables_map vm;
        options.add_options()
        ("help", "Produce this help message")
        ("seed", po::value<long>(&seed_from_command_line)->default_value(time(NULL)), "Set the random seed")
        ("params", po::value<string>(&params_file)->required(), "Specify input parameter file") ;
        
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);
        
        if(vm.count("help")) {
            cerr << options << endl; return(EXIT_FAILURE);
        }
        
        Params params(params_file,seed_from_command_line);
        error = params.check_input_params( );
        if (error) {
            cerr << "[RunEEMS2] Error parametrizing EEMS2." << endl;
            return(EXIT_FAILURE);
        }
        
        // Specify the distance metric in the params.ini file
        dist_metric = params.distance;
        
        EEMS2 eems2(params);
        MCMC mcmc(params);
        
        
        boost::filesystem::path dir(eems2.prevpath().c_str());
        /*
        if (exists(dir)) {
            cerr << "Load final EEMS2 state from " << eems2.prevpath() << endl << endl;
            eems2.load_final_state();
        } else {
            cerr << "Initialize EEMS2 random state" << endl << endl;
            eems2.initialize_state();
        }
         */
        
        cerr << "Initialize EEMS2 random state" << endl << endl;
        eems2.initialize_state();
        error = eems2.start_eems(mcmc);
        if (error) {
            cerr << "[RunEEMS2] Error starting EEMS2." << endl;
            return(EXIT_FAILURE);
        }
        
        Proposal proposal;
        
        double cold_temperature;
        double hot_temperature;
        double temperatures [4] = { 20, 10, 5, 1};
        
        for (int chain = 0; chain < 4; chain ++ ){
            
            double temperature = temperatures[chain];
            mcmc.restart(params, temperature);
            eems2.prev_stored_accepted_proposals.swap(eems2.now_stored_accepted_proposals);
            eems2.now_stored_accepted_proposals.clear();
            
            while (!mcmc.finished) {
                
                proposeMove(eems2, proposal, chain);
                mcmc.add_to_total_moves(proposal.move);
                
                hot_temperature = temperatures[chain];
                if (chain == 0){
                    cold_temperature = -1;
                } else{
                    cold_temperature = temperatures[chain+1];
                }
                
                
                if (eems2.accept_proposal(proposal, hot_temperature, cold_temperature)) { mcmc.add_to_okay_moves(proposal.move); }
                
                eems2.update_hyperparams( );
                mcmc.end_iteration( );
                
                // Check whether to save the current parameter state,
                // as the thinned out iterations are not saved
                
                if (mcmc.to_store_iteration() >= 0){
                    Proposal state;
                    eems2.getState(state);
                    eems2.now_stored_accepted_proposals.push_back(state);
                }
                
                int iter = mcmc.to_write_iteration( );
                if (iter>=0) {
                    eems2.print_iteration(mcmc);
                    eems2.save_iteration(mcmc);
                    eems2.writePopSizes();
                    //eems2.printMigrationAndCoalescenceRates();
                }
                
            }
            
            cout << "Ending MCMC chain with temperature " << temperature << " with acceptance proportions: " << endl;
            cout << mcmc << endl;
            eems2.prev_stored_accepted_proposals.clear();
            
            
            
        }
        
        error = eems2.output_results(mcmc);
        if (error) { cerr << "[RunEEMS2] Error saving eems results to " << eems2.mcmcpath() << endl; }
        
        
    } catch(exception& e) {
        cerr << e.what() << endl;
        return(EXIT_FAILURE);
    }
    
    return(0);
}
