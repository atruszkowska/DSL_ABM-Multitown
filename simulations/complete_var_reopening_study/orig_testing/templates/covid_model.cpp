#include "../../../../include/abm.h"
#include <chrono>

/***************************************************** 
 *
 * ABM run of COVID-19 SEIR in New Rochelle, NY 
 *
 ******************************************************/

int main()
{
	// Time in days, space in km
	double dt = 0.25;
	// Max number of steps to simulate
	int tmax = 360;	
	// Print agent info this many steps
	int dt_out_agents = 90; 
	// Number of initially infected
	int inf0 = 16;
	// Number of agents in different stages of COVID-19
	int N_covid = 187; 

	// Input files
	std::string fin("input_data/NR_agents.txt");
	std::string hfile("input_data/NR_households.txt");
	std::string sfile("input_data/NR_schools.txt");
	std::string wfile("input_data/NR_workplaces.txt");
	std::string hsp_file("input_data/NR_hospitals.txt");
	std::string rh_file("input_data/NR_retirement_homes.txt");
	std::string cp_file("input_data/NR_carpool.txt");
	std::string pt_file("input_data/NR_public.txt");
	std::string ls_file("input_data/NR_leisure.txt");

	// File with infection parameters
	std::string pfname("input_data/infection_parameters.txt");
	// Files with age-dependent distributions
	std::string dexp_name("input_data/age_dist_exposed_never_sy.txt");
	std::string dh_name("input_data/age_dist_hospitalization.txt");
	std::string dhicu_name("input_data/age_dist_hosp_ICU.txt");
	std::string dmort_name("input_data/age_dist_mortality.txt");
	// Map for abm loading of distributions
	std::map<std::string, std::string> dfiles = 
		{ {"exposed never symptomatic", dexp_name}, {"hospitalization", dh_name}, 
		  {"ICU", dhicu_name}, {"mortality", dmort_name} };
	// File with 	
	std::string tfname("input_data/tests_with_time.txt");

	// Output file names
	// Active at the current step - all, detected and not 
	std::ofstream ftot_inf_cur("output/infected_with_time.txt");
	// Cumulative
	std::ofstream ftot_inf("output/total_infected.txt");
	std::ofstream ftot_dead("output/dead_with_time.txt");

	ABM abm(dt, pfname, dfiles, tfname);

	// First the places
	abm.create_households(hfile);
	abm.create_schools(sfile);
	abm.create_workplaces(wfile);
	abm.create_hospitals(hsp_file);
	abm.create_retirement_homes(rh_file);
	abm.create_carpools(cp_file);
	abm.create_public_transit(pt_file);
	abm.create_leisure_locations(ls_file);
	abm.initialize_mobility();

	// Then the agents
	abm.create_agents(fin, inf0);

	// Initialization for vaccination reopening studies
	abm.initialize_vac_and_reopening();
	abm.initialize_active_cases(N_covid);

	// Simulation
	std::vector<int> active_count(tmax+1);
	std::vector<int> infected_count(tmax+1);
	std::vector<int> total_dead(tmax+1);

	// For time measurement
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	for (int ti = 0; ti<=tmax; ++ti){
		// Save agent information
		/*if (ti%dt_out_agents == 0){
			std::string fname = "output/agents_t_" + std::to_string(ti) + ".txt";
			abm.print_agents(fname);
		}*/
		// Collect data
		active_count.at(ti) = abm.get_num_infected();
		infected_count.at(ti) = abm.get_total_infected();
		total_dead.at(ti) = abm.get_total_dead();
		
		// Propagate 
		abm.transmit_ideal_testing_vac_reopening();
	}

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
	std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds> (end - begin).count() << "[s]" << std::endl;

	// Totals
	// Infection
	std::copy(active_count.begin(), active_count.end(), std::ostream_iterator<int>(ftot_inf_cur, " "));
	std::copy(infected_count.begin(), infected_count.end(), std::ostream_iterator<int>(ftot_inf, " "));
	std::copy(total_dead.begin(), total_dead.end(), std::ostream_iterator<int>(ftot_dead, " "));
	
	// Print total values
	std::cout << "Total number of infected agents: " << abm.get_total_infected() << "\n"
			  << "Total number of casualities: " << abm.get_total_dead() << "\n"
			  << "Total number of recovered agents: " << abm.get_total_recovered() << "\n";
}
