#include "../include/abm.h"

/***************************************************** 
 * class: ABM
 * 
 * Interface for agent-based modeling 
 *
 * Provides operations for creation, management, and
 * progression of an agent-based model
 *
 * Stores model-related data
 *
 * NOTE: IDs of objects correspond to their positions
 * in the vectors of objects and determine the way
 * they are accessed; IDs start with 1 but are corrected
 * by -1 when accessing;   
 * 
******************************************************/

//
// Initialization and object construction
//

// Set initial values on all the data collection variables and containers
void ABM::initialize_data_collection()
{
	n_infected_tot = 0;
	n_dead_tot = 0;
	n_dead_tested = 0;
	n_dead_not_tested = 0;
	n_recovered_tot = 0;
	n_recovering_exposed = 0;

	tot_tested = 0;
	tot_tested_pos = 0;
	tot_tested_neg = 0;
	tot_tested_false_pos = 0;
	tot_tested_false_neg = 0;

	n_infected_day = {};
	n_dead_day = {};
	n_recovered_day = {};
	tested_day = {};
	tested_pos_day = {};	
	tested_neg_day = {};
	tested_false_pos_day = {};
	tested_false_neg_day = {};
}

// Load infection parameters, store in a map
void ABM::load_infection_parameters(const std::string infile)
{
	// Load parameters
	LoadParameters ldparam;
	infection_parameters = ldparam.load_parameter_map(infile);

	// Set infection distributions
	infection.set_latency_distribution(infection_parameters.at("latency log-normal mean"),
					infection_parameters.at("latency log-normal standard deviation"));	
	infection.set_inf_variability_distribution(infection_parameters.at("agent variability gamma shape"),
					infection_parameters.at("agent variability gamma scale"));
	infection.set_onset_to_death_distribution(infection_parameters.at("otd logn mean"), 
					infection_parameters.at("otd logn std"));
	infection.set_onset_to_hospitalization_distribution(infection_parameters.at("oth gamma shape"), infection_parameters.at("oth gamma scale"));
	infection.set_hospitalization_to_death_distribution(infection_parameters.at("htd wbl shape"), infection_parameters.at("htd wbl scale"));

	// Set single-number probabilities
	infection.set_other_probabilities(infection_parameters.at("average fraction to get tested"),
									  infection_parameters.at("probability of death in ICU"), 
								  infection_parameters.at("probability dying if needing but not admitted to icu"));
}

// Load age-dependent distributions, store in a map of maps
void ABM::load_age_dependent_distributions(const std::map<std::string, std::string> dist_files)
{
	LoadParameters ldparam;
	std::map<std::string, double> one_file;
	for (const auto& dfile : dist_files){
		one_file = ldparam.load_age_dependent(dfile.second);
		for (const auto& entry : one_file){
			age_dependent_distributions[dfile.first][entry.first] = entry.second;
		}
		one_file.clear();
	}

	// Send to Infection class for further processing 
	infection.set_expN2sy_fractions(age_dependent_distributions.at("exposed never symptomatic"));
	infection.set_mortality_rates(age_dependent_distributions.at("mortality"));
	infection.set_hospitalized_fractions(age_dependent_distributions.at("hospitalization"));
	infection.set_hospitalized_ICU_fractions(age_dependent_distributions.at("ICU"));
}

// Initialize testing and its time dependence
void ABM::load_testing(const std::string fname) 
{
	// Regular properties
	testing.initialize_testing(infection_parameters.at("start testing"),
					infection_parameters.at("negative tests fraction"),
					infection_parameters.at("fraction false negative"),
					infection_parameters.at("fraction false positive"),
					infection_parameters.at("fraction to get tested"),
					infection_parameters.at("exposed fraction to get tested"));	
	// Time-dependent test fractions
	std::vector<std::vector<std::string>> file = read_object(fname);
	std::vector<std::vector<double>> fractions_times = {};
	std::vector<double> temp(3,0.0);
	for (auto& entry : file){
		for (int i=0; i<3; ++i){
			temp.at(i) = std::stod(entry.at(i));
		}
		fractions_times.push_back(temp);
	}
	testing.set_time_varying(fractions_times);
}

// Generate and store household objects
void ABM::create_households(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// One household per line
	for (auto& house : file){
		// Extract properties, add infection parameters
		Household temp_house(std::stoi(house.at(0)), 
			std::stod(house.at(1)), std::stod(house.at(2)),
			infection_parameters.at("household scaling parameter"),
			infection_parameters.at("severity correction"),
			infection_parameters.at("household transmission rate"),
			infection_parameters.at("transmission rate of home isolated"));
		// Store 
		households.push_back(temp_house);
	}
}

// Generate and store retirement homes objects
void ABM::create_retirement_homes(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// One household per line
	for (auto& rh : file){
		// Extract properties, add infection parameters
		RetirementHome temp_RH(std::stoi(rh.at(0)), 
			std::stod(rh.at(1)), std::stod(rh.at(2)),
			infection_parameters.at("severity correction"),
			infection_parameters.at("RH employee absenteeism factor"),
			infection_parameters.at("RH employee transmission rate"),
			infection_parameters.at("RH resident transmission rate"),
			infection_parameters.at("RH transmission rate of home isolated"));
		// Store 
		retirement_homes.push_back(temp_RH);
	}
}

// Generate and store school objects
void ABM::create_schools(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// One workplace per line
	for (auto& school : file){
		// Extract properties, add infection parameters
		// School-type dependent absenteeism
		double psi = 0.0;
		std::string school_type = school.at(3);
		if (school_type == "daycare")
 			psi = infection_parameters.at("daycare absenteeism correction");
		else if (school_type == "primary" || school_type == "middle")
 			psi = infection_parameters.at("primary and middle school absenteeism correction");
		else if (school_type == "high")
 			psi = infection_parameters.at("high school absenteeism correction");
		else if (school_type == "college")
 			psi = infection_parameters.at("college absenteeism correction");
		else
			throw std::invalid_argument("Wrong school type: " + school_type);
		School temp_school(std::stoi(school.at(0)), 
			std::stod(school.at(1)), std::stod(school.at(2)),
			infection_parameters.at("severity correction"),	
			infection_parameters.at("school employee absenteeism correction"), psi,
			infection_parameters.at("school employee transmission rate"), 
			infection_parameters.at("school transmission rate"));
		// Store 
		schools.push_back(temp_school);
	}
}

// Generate and store workplace objects
void ABM::create_workplaces(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// One workplace per line
	for (auto& work : file){
		// Get the occupation type of the workplace
		std::string work_type = work.at(3);
		std::string rate_by_type = "outside";
		double work_rate = 0.0;	 
		if (work_type == "A") { 
			rate_by_type = "management science art transmission rate"; 
		} else if (work_type == "B") { 
			rate_by_type = "service occupation transmission rate";
		} else if (work_type == "C") { 
			rate_by_type = "sales office transmission rate"; 
		} else if (work_type == "D") { 
			rate_by_type = "construction maintenance transmission rate"; 
		} else if (work_type == "E") { 
			rate_by_type = "production transportation transmission rate"; 
		} 
		// Get the transmission rate by occupation
		if (work_type != "outside") {
		 	work_rate = infection_parameters.at(rate_by_type);
		} else {
			work_rate = 1.0;
		}
		// Extract properties, add infection parameters
        Workplace temp_work(std::stoi(work.at(0)),
            std::stod(work.at(1)), std::stod(work.at(2)),
            infection_parameters.at("severity correction"),
            infection_parameters.at("work absenteeism correction"),
            work_rate, work.at(3));
		// Store 
		workplaces.push_back(temp_work);
	}
	set_outside_workplace_transmission();
}

// Create hospitals based on information in a file
void ABM::create_hospitals(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// One hospital per line
	for (auto& hospital : file){
		// Make a map of transmission rates for different 
		// hospital-related categories
		std::map<const std::string, const double> betas = 
			{{"hospital employee", infection_parameters.at("healthcare employees transmission rate")}, 
			 {"hospital non-COVID patient", infection_parameters.at("hospital patients transmission rate")},
			 {"hospital testee", infection_parameters.at("hospital tested transmission rate")},
			 {"hospitalized", infection_parameters.at("hospitalized transmission rate")}, 
			 {"hospitalized ICU", infection_parameters.at("hospitalized ICU transmission rate")}};
		Hospital temp_hospital(std::stoi(hospital.at(0)), 
			std::stod(hospital.at(1)), std::stod(hospital.at(2)),
			infection_parameters.at("severity correction"), betas);
		// Store 
		hospitals.push_back(temp_hospital);
	}
}

// Generate and store carpool objects
void ABM::create_carpools(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// One carpool per line
	for (auto& cpl : file) {
		// Extract properties, add infection parameters
		Transit temp_transit(std::stoi(cpl.at(0)), 
			infection_parameters.at("carpool transmission rate"),
			infection_parameters.at("severity correction"),  cpl.at(1));
		// Store 
		carpools.push_back(temp_transit);
	}
}

// Generate and store public transit objects
void ABM::create_public_transit(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// Transmission rate based on current capacity
	double beta_T = infection_parameters.at("public transit beta0") 
					+ infection_parameters.at("public transit beta full")
						*infection_parameters.at("public transit current capacity");
	// One public transit per line
	for (auto& pbt : file) {
		// Extract properties, add infection parameters
		Transit temp_transit(std::stoi(pbt.at(0)),
			beta_T, infection_parameters.at("severity correction"),  pbt.at(1));
		// Store 
		public_transit.push_back(temp_transit);
	}
}

// Generate and store leisure locations/weekend objects
void ABM::create_leisure_locations(const std::string fname)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);
	// One leisure location per line
	for (auto& lsr : file) {
		// Extract properties, add infection parameters
		Leisure temp_lsr(std::stoi(lsr.at(0)), 
			std::stod(lsr.at(1)), std::stod(lsr.at(2)),
			infection_parameters.at("severity correction"), 
			infection_parameters.at("leisure locations transmission rate"),
			lsr.at(3));
		// Store 
		leisure_locations.push_back(temp_lsr);
	}
	set_outside_leisure_transmission();
}

// Initialize Mobility and assignment of leisure locations
void ABM::initialize_mobility()
{
	mobility.set_probability_parameters(infection_parameters.at("leisure - dr0"), infection_parameters.at("leisure - beta"), infection_parameters.at("leisure - kappa"));
	mobility.construct_public_probabilities(households, leisure_locations);
}

// Create agents and assign them to appropriate places
void ABM::create_agents(const std::string fname, const int ninf0)
{
	load_agents(fname, ninf0);
	register_agents();
}

// Retrieve agent information from a file
void ABM::load_agents(const std::string fname, const int ninf0)
{
	// Read the whole file
	std::vector<std::vector<std::string>> file = read_object(fname);

	// Flu settings
	// Set fraction of flu (non-covid symptomatic)
	flu.set_fraction(infection_parameters.at("fraction with flu"));
	flu.set_fraction_tested_false_positive(infection_parameters.at("fraction false positive"));
	// Time interval for testing
	flu.set_testing_duration(infection_parameters.at("flu testing duration"));

	// For custom generation of initially infected
	std::vector<int> infected_IDs(ninf0);
	bool not_unique = true;
	int inf_ID = 0;
	if (ninf0 != 0){
		int nIDs = file.size();
		// Random choice of IDs
		for (int i=0; i<ninf0; ++i){
			not_unique = true;
			while (not_unique){
				inf_ID = infection.get_random_agent_ID(nIDs);
				auto iter = std::find(infected_IDs.begin(), infected_IDs.end(), inf_ID);
				not_unique = (iter != infected_IDs.end()); 
			}
			infected_IDs.at(i) = inf_ID;
		}
	}

	// Counter for agent IDs
	int agent_ID = 1;
	
	// One agent per line, with properties as defined in the line
	for (auto agent : file){
		// Agent status
		bool student = false, works = false, livesRH = false, worksRH = false,  
			 worksSch = false, patient = false, hospital_staff = false,
			 works_from_home = false;
		int house_ID = -1, workID = 0, cpID = 0, ptID = 0;
		double work_travel_time = 0.0;
		std::string work_travel_mode;
				
		// Household ID only if not hospitalized with condition
		// different than COVID-19
		if (std::stoi(agent.at(6)) == 1){
			patient = true;
			house_ID = 0;
		}else{
			house_ID = std::stoi(agent.at(5));
		}

		// No school or work if patient with condition other than COVID
		if (std::stoi(agent.at(12)) == 1 && !patient){
			hospital_staff = true;
		}
		if (std::stoi(agent.at(0)) == 1 && !patient){
			student = true;
		}
	   	// No work flag if a hospital employee	
		if (std::stoi(agent.at(1)) == 1 && !(patient || hospital_staff)){
			works = true; 
		}
			
		// Random or from the input file
		bool infected = false;
		if (ninf0 != 0){
			auto iter = std::find(infected_IDs.begin(), infected_IDs.end(), agent_ID); 
			if (iter != infected_IDs.end()){
				infected_IDs.erase(iter);
				infected = true;
				n_infected_tot++;
			}
		} else {
			if (std::stoi(agent.at(14)) == 1){
				infected = true;
				n_infected_tot++;
			}
		}

		// Retirement home resident
		if (std::stoi(agent.at(8)) == 1){
			 livesRH = true;
		}
		// Retirement home or school employee
		if (std::stoi(agent.at(9)) == 1){
			 worksRH = true;
		}
		if (std::stoi(agent.at(10)) == 1){
			 worksSch = true;
		}

		// Select correct work ID for special employment types
		// Hospital ID is set separately, but for consistency
		if (worksRH || worksSch || hospital_staff) {
			workID = std::stoi(agent.at(18));
		} else if (works) {
			workID = std::stoi(agent.at(11));
		}

		// Transit information
		if (std::stoi(agent.at(15)) == 1) {
			works_from_home = true;
			work_travel_mode = agent.at(17);
		} else {
			if (!(works || hospital_staff)) {
				work_travel_mode = "None";
			} else {
				work_travel_mode = agent.at(17);
				if (work_travel_mode == "carpool") {
					cpID = std::stoi(agent.at(19));
				}
				if (work_travel_mode == "public") {
					ptID = std::stoi(agent.at(20));
				}
				work_travel_time = std::stod(agent.at(16));
			}
		}
		Agent temp_agent(student, works, std::stoi(agent.at(2)), 
			std::stod(agent.at(3)), std::stod(agent.at(4)), house_ID,
			patient, std::stoi(agent.at(7)), livesRH, worksRH,
		    worksSch, workID, hospital_staff, std::stoi(agent.at(13)), 
			infected, work_travel_mode, work_travel_time, cpID, ptID, 
			works_from_home);
	
        // Set agent occupation
        std::string work_type = agent.at(21);
        std::string rate_by_type;
        temp_agent.set_occupation(work_type);
        // And the corresponding transmission rate
        double work_rate = 0.0;
        if (work_type != "none") {
            if (work_type == "A") {
                rate_by_type = "management science art transmission rate";
            } else if (work_type == "B") {
                rate_by_type = "service occupation transmission rate";
            } else if (work_type == "C") {
                rate_by_type = "sales office transmission rate";
            } else if (work_type == "D") {
                rate_by_type = "construction maintenance transmission rate";
            } else if (work_type == "E") {
                rate_by_type = "production transportation transmission rate";
            }
            temp_agent.set_occupation_transmission(infection_parameters.at(rate_by_type));
        }

		// Set Agent ID
		temp_agent.set_ID(agent_ID++);
		
		// Set properties for exposed if initially infected
		if (temp_agent.infected() == true){
			initial_exposed(temp_agent);
			// Saving infected information
			if (collect_data) {
				collect_infected_properties(temp_agent);
			}
		}	
		
		// Store
		agents.push_back(temp_agent);
	}
}

// Assign agents to households, schools, and worplaces
void ABM::register_agents()
{
	int house_ID = 0, school_ID = 0, work_ID = 0, hospital_ID = 0;
	int agent_ID = 0, tr_ID = 0;
	bool infected = false;

	for (const auto& agent : agents){
		
		// Agent ID and infection status
		agent_ID = agent.get_ID();
		infected = agent.infected();

		// If not a non-COVID hospital patient, 
		// register in the household or a retirement home
		if (agent.hospital_non_covid_patient() == false){
			if (agent.retirement_home_resident()){
				house_ID = agent.get_household_ID();
				RetirementHome& rh = retirement_homes.at(house_ID - 1); 
				rh.register_agent(agent_ID, infected);
			} else {
				house_ID = agent.get_household_ID();
				Household& house = households.at(house_ID - 1); 
				house.register_agent(agent_ID, infected);
			}
		}

		// Register in schools, workplaces, and hospitals 
		if (agent.student()){
			school_ID = agent.get_school_ID();
			School& school = schools.at(school_ID - 1); 
			school.register_agent(agent_ID, infected);		
		}

		if (agent.works() && !agent.works_from_home()
				&& !agent.hospital_employee()){
			work_ID = agent.get_work_ID();
			if (agent.retirement_home_employee()){
				RetirementHome& rh = retirement_homes.at(work_ID - 1);
				rh.register_agent(agent_ID, infected);
			} else if (agent.school_employee()){
				School& school = schools.at(work_ID - 1); 
				school.register_agent(agent_ID, infected);
			} else {
				Workplace& work = workplaces.at(work_ID - 1);
				work.register_agent(agent_ID, infected);
			}		
		}

		if (agent.hospital_employee() || 
				agent.hospital_non_covid_patient()){
			hospital_ID = agent.get_hospital_ID();
			Hospital& hospital = hospitals.at(hospital_ID - 1);
			hospital.register_agent(agent_ID, infected);	
		}

		// Register transit if carpool or public
		if (agent.get_work_travel_mode() == "carpool") {
			tr_ID = agent.get_carpool_ID();
			Transit& carpool = carpools.at(tr_ID-1);
			carpool.register_agent(agent_ID, infected);	
		}
		if (agent.get_work_travel_mode() == "public") {
			tr_ID = agent.get_public_transit_ID();
			Transit& public_tr = public_transit.at(tr_ID-1);
			public_tr.register_agent(agent_ID, infected);	
		}
	} 
}

// Initial set-up of exposed agents
void ABM::initial_exposed(Agent& agent)
{
	bool never_sy = infection.recovering_exposed(agent.get_age());
	// Total latency period
	double latency = infection.latency();
	// Portion of latency when the agent is not infectious
	double dt_ninf = std::min(infection_parameters.at("time from exposed to infectiousness"), latency);
	if (never_sy){
		// Set to total latency + infectiousness duration
		double rec_time = infection_parameters.at("recovery time");
		agent.set_latency_duration(latency + rec_time);
		agent.set_latency_end_time(time);
		agent.set_infectiousness_start_time(time, dt_ninf);
	}else{
		// If latency shorter, then  not infectious during the entire latency
		agent.set_latency_duration(latency);
		agent.set_latency_end_time(time);
		agent.set_infectiousness_start_time(time, dt_ninf);
	}
	agent.set_inf_variability_factor(infection.inf_variability());
	agent.set_exposed(true);
	agent.set_recovering_exposed(never_sy);
}

// Initialization for vaccination vs. reopening studies
void ABM::initialize_vac_and_reopening()
{
	// Flu and initial vaccination
	n_vaccinated = static_cast<int>(infection_parameters.at("initially vaccinated"));
	random_vaccines = true;
	// To invoke flu, testing, and vaccinations
	infection_parameters.at("start testing") = 0.0;
	start_testing_flu_and_vaccination();
	
	// Schools - constant reduction
	double sch_rate_students =  infection_parameters.at("school transmission rate")
									*infection_parameters.at("school transmission reduction");	
	double sch_rate_emp =  infection_parameters.at("school employee transmission rate")
									*infection_parameters.at("school transmission reduction");	
	for (auto& school : schools){
		school.change_transmission_rate(sch_rate_students);
		school.change_employee_transmission_rate(sch_rate_emp);
	}

	// Workplaces - phase 4, constant
	double work_tr_rate = infection_parameters.at("fraction of phase 4 businesses");
	for (auto& workplace : workplaces) {
		if (workplace.outside_town()) {
			workplace.adjust_outside_lambda(infection_parameters.at("fraction of phase 4 businesses"));
		} else {
			workplace.change_transmission_rate(workplace.get_transmission_rate()*work_tr_rate);
		}
	}
	
	// Carpools - reduction proportional to workplaces
	double new_tr_rate = infection_parameters.at("carpool transmission rate")
							*infection_parameters.at("fraction of phase 4 businesses");
	for (auto& car  : carpools) {
		car.change_transmission_rate(new_tr_rate);
	}

	// Public transit
 	new_tr_rate = infection_parameters.at("public transit beta0") 
				+ infection_parameters.at("public transit beta full")
				*infection_parameters.at("public transit current capacity")
				*infection_parameters.at("fraction of phase 4 businesses");
	for (auto& pt  : public_transit) {
		pt.change_transmission_rate(new_tr_rate);
	}
	
	// Public leisure locations
 	new_tr_rate = infection_parameters.at("leisure locations transmission rate") * 
			infection_parameters.at("fraction of phase 4 businesses");
	ini_beta_les = new_tr_rate;
	del_beta_les = infection_parameters.at("leisure locations transmission rate") - new_tr_rate;
	ini_frac_les = infection_parameters.at("leisure - fraction - initial");
	del_frac_les = infection_parameters.at("leisure - fraction - final")
					-infection_parameters.at("leisure - fraction - initial");
	infection_parameters.at("leisure - fraction") = ini_frac_les;
	for (auto& leisure_location : leisure_locations) {
		if (leisure_location.outside_town()) {
			leisure_location.set_outside_lambda(ini_beta_les*infection_parameters.at("fraction estimated infected"));
		} else {
			leisure_location.change_transmission_rate(ini_beta_les);
		}
	} 
}

// Start with N_inf agents that have COVID-19 in various stages
void ABM::initialize_active_cases(const int N_inf)
{
	// Increase total infected count
	n_infected_tot += N_inf;
	std::vector<int> can_have_covid;
	// Select qualifying agents
	for (auto& agent : agents) {
		if (!agent.symptomatic_non_covid() && !agent.infected()
				&& !agent.exposed() && !agent.symptomatic() 
				&& !agent.removed() && !agent.vaccinated()) {
			can_have_covid.push_back(agent.get_ID());
		}
	}

	// Randomly rearrange, then select first N_inf if available 
	infection.vector_shuffle(can_have_covid);
	// Remove first n_vaccinated
	if (N_inf > can_have_covid.size()) {
		std::cerr << "Requested number of agents to initially have covid "
				  << "larger than number of available agents" << std::endl;
		throw std::invalid_argument("Too many agents to have covid: " + N_inf);
	}
	for (int i=0; i<N_inf; ++i) {
		Agent& agent = agents.at(can_have_covid.at(i)-1);
		// Currently aymptomatic or altogether asymptomatic
		bool never_sy = infection.recovering_exposed(agent.get_age());
		if (never_sy) {
			process_initial_asymptomatic(agent);
		} else {
			process_initial_symptomatic(agent);
		}
		// Saving infected information
		if (collect_data) {
			collect_infected_properties(agent);
		}
	}
}

// Initialize an asymptomatic agent, randomly in the course of disease
void ABM::process_initial_asymptomatic(Agent& agent)
{
	// Temporary transition objects
	RegularTransitions regular_transitions;
	HspEmployeeTransitions hsp_employee_transitions;
    HspPatientTransitions hsp_patient_transitions;

	// Flags
	agent.set_infected(true);
	agent.set_exposed(true);
	agent.set_recovering_exposed(true);

	// Common properties
	// Total latency period offset with a random number from 0 to 1
	double latency = infection.latency()*infection.get_uniform();
	// Portion of latency when the agent is not infectious
	double dt_ninf = std::min(infection_parameters.at("time from exposed to infectiousness"), latency);
	// Set to total latency + infectiousness duration, also offset
	double rec_time = infection_parameters.at("recovery time")*infection.get_uniform();
	agent.set_latency_duration(latency + rec_time);
	agent.set_latency_end_time(time);
	agent.set_infectiousness_start_time(time, dt_ninf);
	// Agent characteristics
	agent.set_inf_variability_factor(infection.inf_variability());
	// Remove from potential flu population if a regular agent
	if (!agent.hospital_employee() && !agent.hospital_non_covid_patient()) {
		flu.remove_susceptible_agent(agent.get_ID());
	}

	// Testing status
	if (testing.started(time)) {
		if (agent.hospital_employee()) {
			hsp_employee_transitions.set_testing_status(agent, infection, time, schools, 
							hospitals, infection_parameters, testing);
		} else if (agent.hospital_non_covid_patient()) {
			hsp_patient_transitions.set_testing_status(agent, infection, time, hospitals, infection_parameters, testing);
		} else {
    		regular_transitions.set_testing_status(agent, infection, time, schools,
        		workplaces, hospitals, retirement_homes, carpools, public_transit, infection_parameters, testing);
		}

		// If tested, randomly choose if pre-test, being tested now, or waiting for results
		std::vector<std::string> testing_stages = {"waiting for test", "getting tested", "waiting for results"};
		if (agent.tested()) {
			std::string cur_stage = testing_stages.at(infection.get_int(0, testing_stages.size()-1));
			double test_time_lag = 0.0;
			// Adjust properties accordingly
			if (cur_stage == "waiting for test") {
				// Just perturb the time to wait
				test_time_lag = infection.get_uniform()*std::max(0.0, agent.get_time_of_test() - time);
				agent.set_time_to_test(test_time_lag);
				agent.set_time_of_test(time);
			} else if (cur_stage == "getting tested") {
				// Adjust the time, transitions will happen on their own 
				agent.set_time_to_test(0.0);
				agent.set_time_of_test(time);
			} else {
				// Reset time to wait for the test 
				test_time_lag = infection.get_uniform()*std::max(0.0, agent.get_time_of_test() - time);
				agent.set_time_to_test(-1.0*test_time_lag);
				agent.set_time_of_test(time);
				// Perturb the time to wait for results
				test_time_lag = infection.get_uniform()*std::max(0.0, agent.get_time_of_results() - time);
				agent.set_time_until_results(test_time_lag);
				agent.set_time_of_results(time);
				// Flags
				agent.set_tested_awaiting_test(false);
				agent.set_tested_awaiting_results(true);
			}		
		}
     }
}

// Initialize a symptomatic agent, randomly in the course of disease
void ABM::process_initial_symptomatic(Agent& agent)
{
	// Temporary transition objects
	RegularTransitions regular_transitions;
	HspEmployeeTransitions hsp_employee_transitions;
    HspPatientTransitions hsp_patient_transitions;
	// Flags
 	agent.set_infected(true);
	agent.set_symptomatic(true);
	// Agent characteristics
	agent.set_inf_variability_factor(infection.inf_variability());
	// Remove from potential flu population if a regular agent
	if (!agent.hospital_employee() && !agent.hospital_non_covid_patient()) {
		flu.remove_susceptible_agent(agent.get_ID());
	}

	// Testing status
	if (agent.hospital_employee()) {
		// Hospital employee will go under IH and test for sure
		hsp_employee_transitions.remove_from_hospitals_and_schools(agent, schools, hospitals, carpools,  public_transit);
		// Removal settings		
		int agent_age = agent.get_age();
		bool is_hsp = true;
		if (infection.will_die_non_icu(agent_age, is_hsp)){
			agent.set_dying(true);
			agent.set_recovering(false);			
			agent.set_time_to_death(infection.time_to_death());
			agent.set_death_time(time);
		} else {
			agent.set_dying(false);
			agent.set_recovering(true);			
			agent.set_recovery_duration(infection_parameters.at("recovery time")*infection.get_uniform());
			agent.set_recovery_time(time);		
		}
		if (testing.started(time)) {
			hsp_employee_transitions.set_testing_status(agent, infection, time, schools,
                        hospitals, infection_parameters, testing);
		}
	} else if (agent.hospital_non_covid_patient()) {
		// Removal settings
		bool is_hsp = true; 		
		int agent_age = agent.get_age();
		if (infection.will_die_non_icu(agent_age, is_hsp)){
			states_manager.set_dying_symptomatic(agent);			
			agent.set_time_to_death(infection.time_to_death());
			agent.set_death_time(time);
		} else {
			states_manager.set_recovering_symptomatic(agent);			
			// This may change if treatment is ICU
			agent.set_recovery_duration(infection_parameters.at("recovery time"));
			agent.set_recovery_time(time);		
		}
		if (testing.started(time)) {
			hsp_patient_transitions.set_testing_status(agent, infection, time, hospitals, infection_parameters, testing);
		}
	} else {
    	regular_transitions.untested_sy_setup(agent, infection, time, dt, households, 
                schools, workplaces, hospitals, retirement_homes,
                carpools, public_transit, infection_parameters, testing);
	}

	// If tested, randomly choose if pre-test, being tested now, or waiting for results
	std::vector<std::string> testing_stages = {"waiting for test", "getting tested", "waiting for results",
												"getting treated"};
	if (agent.tested()) {
		std::string cur_stage = testing_stages.at(infection.get_int(0, testing_stages.size()-1));
		double test_time_lag = 0.0;
		// Adjust properties accordingly
		if (cur_stage == "waiting for test") {
			// Just perturb the time to wait
			test_time_lag = infection.get_uniform()*std::max(0.0, agent.get_time_of_test() - time);
			agent.set_time_to_test(test_time_lag);
			agent.set_time_of_test(time);
		} else if (cur_stage == "getting tested") {
			// Adjust the time, transitions will happen on their own 
			agent.set_time_to_test(0.0);
			agent.set_time_of_test(time);
		} else if (cur_stage == "waiting for results") {
			// Reset time to wait for the test 
			test_time_lag = infection.get_uniform()*std::max(0.0, agent.get_time_of_test() - time);
			agent.set_time_to_test(-1.0*test_time_lag);
			agent.set_time_of_test(time);
			// Perturb the time to wait for results
			test_time_lag = infection.get_uniform()*std::max(0.0, agent.get_time_of_results() - time);
			agent.set_time_until_results(test_time_lag);
			agent.set_time_of_results(time);
			// Flags
			agent.set_tested_awaiting_test(false);
			agent.set_tested_awaiting_results(true);
		} else {
			// Got results (treatment or false negative)
			// Reset time to wait for the test 
			test_time_lag = infection.get_uniform()*std::max(0.0, agent.get_time_of_test() - time);
			agent.set_time_to_test(-1.0*test_time_lag);
			agent.set_time_of_test(time);
			// Set time for results to now
			agent.set_time_until_results(0);
			agent.set_time_of_results(time);
			// Flags
			agent.set_tested(true);
			agent.set_tested_awaiting_test(false);
			agent.set_tested_awaiting_results(true);
		} 		
	}
}

// Vaccinate random members of the population that are not Flu or infected agents
void ABM::vaccinate_random()
{
	std::vector<int> can_be_vaccinated;
	// Select qualifying agents
	for (auto& agent : agents) {
		if (!agent.symptomatic_non_covid() && !agent.infected()
				&& !agent.exposed() && !agent.symptomatic() 
				&& !agent.removed() && !agent.vaccinated()) {
			can_be_vaccinated.push_back(agent.get_ID());
		}
	}
	// Shuffle randomly
	infection.vector_shuffle(can_be_vaccinated);
	// Remove first n_vaccinated
	if (n_vaccinated > can_be_vaccinated.size()) {
		std::cerr << "Requested number of agents to vaccinate larger than number of available agents" 
				  << std::endl;
		if (can_be_vaccinated.size() > 0) {
			n_vaccinated = can_be_vaccinated.size();
		} else {
			return;
		}
	}
	for (int i=0; i<n_vaccinated; ++i) {
		agents.at(can_be_vaccinated.at(i)-1).set_vaccinated(true);
	}	
}

// Vaccinate specific group of agents in the population
void ABM::vaccinate_group()
{
	if (vaccine_group_name == "hospital employees") {
		implement_group_vaccination(&Agent::hospital_employee);
	} else if (vaccine_group_name == "school employees") {
		implement_group_vaccination(&Agent::school_employee);
	} else if (vaccine_group_name == "retirement home employees") {
		implement_group_vaccination(&Agent::retirement_home_employee);
	} else if (vaccine_group_name == "retirement home residents") {
		implement_group_vaccination(&Agent::retirement_home_resident);
	} else{
		throw std::invalid_argument("Wrong vaccination group type: " + vaccine_group_name);
	}			
}

// Assign removed (vaccinated equivalent) flag to the group with specified type
void ABM::implement_group_vaccination(type_getter atype)
{
	int v_final = 0;
	for (auto& agent : agents){
		if ((agent.*atype)() == true && !agent.infected()
				&& !agent.exposed() && !agent.symptomatic()
				&& !agent.removed()){
			agent.set_vaccinated(true);
			++v_final;			
		}
	}
	if (vac_verbose){
		std::cout << "Total number of vaccinated in the group " 
				  << vaccine_group_name << " " << v_final << std::endl;
	}
}

//
// Transmission of infection
//

// Transmit infection 
void ABM::transmit_infection() 
{
	testing.check_switch_time(time);	
	check_events(schools, workplaces);
	distribute_leisure();
	compute_place_contributions();	
	compute_state_transitions();
	reset_contributions();
	advance_in_time();	
}

// Perfect testing, vaccination, and reopening 
void ABM::transmit_ideal_testing_vac_reopening() 
{
	reopen_leisure_locations();
	vaccinate();
	distribute_leisure();
	compute_place_contributions();	
	compute_state_transitions();
	reset_contributions();
	advance_in_time();	
}

// Randomly vaccinate agents based on the daily rate
void ABM::vaccinate()
{
	// Adjust n_vaccinated 
	n_vaccinated = static_cast<int>(infection_parameters.at("vaccination rate")*dt);
	// Apply at random to susceptible and unvaccinated agents
	vaccinate_random();				
}

// Increase transmission rate and visiting frequency of leisure locations 
void ABM::reopen_leisure_locations()
{
	double new_tr_rate = 0.0;
	new_tr_rate = ini_beta_les + infection_parameters.at("leisure reopening rate")*del_beta_les*time;
	new_tr_rate = std::min(new_tr_rate, infection_parameters.at("leisure locations transmission rate"));
	// Increase transmission rate, or the (highest) normal rate 
	for (auto& leisure_location : leisure_locations) {
		if (leisure_location.outside_town()) {
			leisure_location.set_outside_lambda(new_tr_rate*infection_parameters.at("fraction estimated infected"));
		} else {
			leisure_location.change_transmission_rate(new_tr_rate);
		}
	}
	// Fraction of people going to leisure locations - same approach
	double new_frac = 0.0;
	new_frac = ini_frac_les + infection_parameters.at("leisure reopening rate")*del_frac_les*time;
	new_frac = std::min(new_frac, infection_parameters.at("leisure - fraction - final"));
	infection_parameters.at("leisure - fraction") = new_frac;
}

// Assign leisure locations for this step
void ABM::distribute_leisure()
{
	// Remove previous leisure assignments
	// Reset the ID for all that had a location 
	// This includes all agents, passed as well
	int old_loc_ID = 0;
	for (auto& agent : agents) {
		old_loc_ID = agent.get_leisure_ID();
		if ( old_loc_ID > 0) {
			if (agent.get_leisure_type() == "household") {
				households.at(old_loc_ID - 1).remove_agent(agent.get_ID());
			} else if (agent.get_leisure_type() == "public") {
				// Only remove in-town leisure locations
				if(!leisure_locations.at(old_loc_ID -1).outside_town()){
					leisure_locations.at(old_loc_ID - 1).remove_agent(agent.get_ID());
				}
			} else {
				throw std::invalid_argument("Wrong leisure type: " + agent.get_leisure_type());
			}
			agent.set_leisure_ID(0);
		}
	}

	// Assumes one leisure location per
	// whole household - automatically excludes hospital patients (including non-COVID ones)
	// and retirement home residents; also passed agents, alive and removed participate
	int loc_ID = 0, house_ID = 0;
	for (auto& house : households) {
		// Not having one assigned this step
		if (infection.get_uniform() > infection_parameters.at("leisure - fraction")) {
			continue;
		}
		
		// Assign location
		bool is_public = false;
		bool is_house = false;
		house_ID = house.get_ID();
		loc_ID = mobility.assign_leisure_location(infection, house_ID, is_house, is_public);
		assert(loc_ID > 0);
		assert((is_house == true) || (is_public == true));

		// Looping through households automatically excludes 
		// agents that died and that are hospitalized
		std::vector<int> agent_IDs = house.get_agent_IDs();
		for (auto& aID : agent_IDs) {
			// Skip agents that are treated or in home isolation
			// due to waiting for test or flu
			if (agents.at(aID-1).being_treated() || agents.at(aID-1).home_isolated()) {
				continue;
			}
			// Also skip if the agent is being tested at this step
			if ((agents.at(aID-1).tested()) && (agents.at(aID-1).get_time_of_test() <= time)
				&& (agents.at(aID-1).tested_awaiting_test() == true)) {
				continue;	
			}
			// Skip guests
			if (agents.at(aID-1).get_leisure_type() == "household" && 
					agents.at(aID-1).get_leisure_ID() == house_ID) {
				continue;
			}	
			// Register each agent in the household
			if (is_house) {
				households.at(loc_ID-1).add_agent(aID);
				agents.at(aID-1).set_leisure_type("household");
				agents.at(aID-1).set_leisure_ID(loc_ID);
			} else if (is_public) {
   		        // Only add if leisure location is within town
   		        if(!leisure_locations.at(loc_ID-1).outside_town()){
   		            leisure_locations.at(loc_ID-1).add_agent(aID);
   		        }
				agents.at(aID-1).set_leisure_type("public");
				agents.at(aID-1).set_leisure_ID(loc_ID);
			}
		}
	}
}

// Verify if anything that requires parameter changes happens at this step 
void ABM::check_events(std::vector<School>& schools, std::vector<Workplace>& workplaces)
{
	double tol = 1e-3;
	double new_tr_rate = 0.0;

	start_testing_flu_and_vaccination();

	// Closures
	if (equal_floats<double>(time, infection_parameters.at("school closure"), tol)){
		new_tr_rate = 0.0;
		for (auto& school : schools){
			school.change_transmission_rate(new_tr_rate);
			school.change_employee_transmission_rate(new_tr_rate);
		}
	}
	if (equal_floats<double>(time, infection_parameters.at("lockdown"), tol)){
		new_tr_rate = infection_parameters.at("workplace transmission rate")*infection_parameters.at("fraction of ld businesses");
		for (auto& workplace : workplaces){
			if (workplace.outside_town()) {
				workplace.adjust_outside_lambda(infection_parameters.at("fraction of ld businesses"));
			} else {
				workplace.change_transmission_rate(new_tr_rate);
				workplace.change_absenteeism_correction(infection_parameters.at("lockdown absenteeism"));
			}
		}
		// Leisure locations
		new_tr_rate = infection_parameters.at("leisure locations transmission rate")
				*infection_parameters.at("fraction of ld businesses");
		for (auto& leisure_location : leisure_locations) {
			if (leisure_location.outside_town()){
				leisure_location.adjust_outside_lambda(infection_parameters.at("fraction of ld businesses"));
			}else{
				leisure_location.change_transmission_rate(new_tr_rate);
			}
		}
		// Fraction of people going to leisure locations
		infection_parameters.at("leisure - fraction") *= infection_parameters.at("fraction of ld businesses");
		// Carpools
		new_tr_rate = infection_parameters.at("carpool transmission rate")*infection_parameters.at("fraction of ld businesses");
		for (auto& car  : carpools) {
			car.change_transmission_rate(new_tr_rate);
		}
		// Public transit
		new_tr_rate = infection_parameters.at("public transit beta0") 
					+ infection_parameters.at("public transit beta full")
					*infection_parameters.at("public transit current capacity")*infection_parameters.at("fraction of ld businesses");
		for (auto& pt  : public_transit) {
			pt.change_transmission_rate(new_tr_rate);
		}
	}

	// Reopening, phase 1
	if (equal_floats<double>(time, infection_parameters.at("reopening phase 1"), tol)){
		new_tr_rate = infection_parameters.at("workplace transmission rate") * 
				infection_parameters.at("fraction of phase 1 businesses");
		for (auto& workplace : workplaces){
			if (workplace.outside_town()) {
				workplace.adjust_outside_lambda(infection_parameters.at("fraction of phase 1 businesses")
								/infection_parameters.at("fraction of ld businesses"));	
			} else {
				workplace.change_transmission_rate(new_tr_rate);
				workplace.change_absenteeism_correction(infection_parameters.at("lockdown absenteeism"));
			}
		}
		// Leisure locations
		new_tr_rate = infection_parameters.at("leisure locations transmission rate") * 
				infection_parameters.at("fraction of phase 1 businesses");
		for (auto& leisure_location : leisure_locations) {
			if (leisure_location.outside_town()) {
				leisure_location.adjust_outside_lambda(infection_parameters.at("fraction of phase 1 businesses")
								/infection_parameters.at("fraction of ld businesses"));	
			} else {
				leisure_location.change_transmission_rate(new_tr_rate);
			}
		}
		// Fraction of people going to leisure locations
		infection_parameters.at("leisure - fraction") 
				*= (infection_parameters.at("fraction of phase 1 businesses")/infection_parameters.at("fraction of ld businesses"));
		// Carpools
		new_tr_rate = infection_parameters.at("carpool transmission rate")*infection_parameters.at("fraction of phase 1 businesses");
		for (auto& car  : carpools) {
			car.change_transmission_rate(new_tr_rate);
		}
		// Public transit
 		new_tr_rate = infection_parameters.at("public transit beta0") 
					+ infection_parameters.at("public transit beta full")
					*infection_parameters.at("public transit current capacity")*infection_parameters.at("fraction of phase 1 businesses");
		for (auto& pt  : public_transit) {
			pt.change_transmission_rate(new_tr_rate);
		}
	}

	// Reopening, phase 2
	if (equal_floats<double>(time, infection_parameters.at("reopening phase 2"), tol)){
		new_tr_rate = infection_parameters.at("workplace transmission rate") * 
				infection_parameters.at("fraction of phase 2 businesses");
		for (auto& workplace : workplaces){
			if (workplace.outside_town()) {
				workplace.adjust_outside_lambda(infection_parameters.at("fraction of phase 2 businesses")
								/infection_parameters.at("fraction of phase 1 businesses"));	
			} else {
				workplace.change_transmission_rate(new_tr_rate);
				workplace.change_absenteeism_correction(infection_parameters.at("lockdown absenteeism"));
			}
		}
		// Leisure locations
		new_tr_rate = infection_parameters.at("leisure locations transmission rate") * 
				infection_parameters.at("fraction of phase 2 businesses");
		for (auto& leisure_location : leisure_locations) {
			if (leisure_location.outside_town()) {
				leisure_location.adjust_outside_lambda(infection_parameters.at("fraction of phase 2 businesses")
								/infection_parameters.at("fraction of phase 1 businesses"));	
			} else {
				leisure_location.change_transmission_rate(new_tr_rate);
			}
		}
		// Fraction of people going to leisure locations
		infection_parameters.at("leisure - fraction") 
				*= (infection_parameters.at("fraction of phase 2 businesses")/infection_parameters.at("fraction of phase 1 businesses"));
		// Carpools
		new_tr_rate = infection_parameters.at("carpool transmission rate")*infection_parameters.at("fraction of phase 2 businesses");
		for (auto& car  : carpools) {
			car.change_transmission_rate(new_tr_rate);
		}
		// Public transit 
		new_tr_rate = infection_parameters.at("public transit beta0") 
					+ infection_parameters.at("public transit beta full")
					*infection_parameters.at("public transit current capacity")*infection_parameters.at("fraction of phase 2 businesses");
		for (auto& pt  : public_transit) {
			pt.change_transmission_rate(new_tr_rate);
		}
	}

	// Reopening, phase 3
	if (equal_floats<double>(time, infection_parameters.at("reopening phase 3"), tol)){
		new_tr_rate = infection_parameters.at("workplace transmission rate") * 
				infection_parameters.at("fraction of phase 3 businesses");
		for (auto& workplace : workplaces){
			if (workplace.outside_town()) {
				workplace.adjust_outside_lambda(infection_parameters.at("fraction of phase 3 businesses")
								/infection_parameters.at("fraction of phase 2 businesses"));	
			} else {
				workplace.change_transmission_rate(new_tr_rate);
				workplace.change_absenteeism_correction(infection_parameters.at("lockdown absenteeism"));
			}
		}
		// Leisure locations
		new_tr_rate = infection_parameters.at("leisure locations transmission rate") * 
				infection_parameters.at("fraction of phase 3 businesses");
		for (auto& leisure_location : leisure_locations) {
			if (leisure_location.outside_town()) {
				leisure_location.adjust_outside_lambda(infection_parameters.at("fraction of phase 3 businesses")
								/infection_parameters.at("fraction of phase 2 businesses"));	
			} else {
				leisure_location.change_transmission_rate(new_tr_rate);
			}
		}
		// Fraction of people going to leisure locations
		infection_parameters.at("leisure - fraction") 
				*= (infection_parameters.at("fraction of phase 3 businesses")/infection_parameters.at("fraction of phase 2 businesses"));
		// Carpools
		new_tr_rate = infection_parameters.at("carpool transmission rate")*infection_parameters.at("fraction of phase 3 businesses");
		for (auto& car  : carpools) {
			car.change_transmission_rate(new_tr_rate);
		}
		// Public transit 
		new_tr_rate = infection_parameters.at("public transit beta0") 
					+ infection_parameters.at("public transit beta full")
					*infection_parameters.at("public transit current capacity")*infection_parameters.at("fraction of phase 3 businesses");
		for (auto& pt  : public_transit) {
			pt.change_transmission_rate(new_tr_rate);
		}
	}
}

// Update transmission dynamics in wokplaces outside of the town
void ABM::set_outside_workplace_transmission()
{
	for (auto& workplace : workplaces) {
		if (workplace.outside_town()) {
			workplace.set_outside_lambda(infection_parameters.at("fraction estimated infected"));
		}
	}
}

// Update transmission dynamics in leisure locations outside of the town
void ABM::set_outside_leisure_transmission()
{
	for (auto& leisure_location : leisure_locations) {
		if (leisure_location.outside_town()) {
			leisure_location.set_outside_lambda(infection_parameters.at("out-of-town leisure transmission"));
		}
	}
}

// Count contributions of all infectious agents in each place
void ABM::compute_place_contributions()
{
	for (const auto& agent : agents){

		// Removed and vaccinated don't contribute
		if (agent.removed() == true || agent.vaccinated() == true){
			continue;
		}

		// If susceptible and being tested - add to hospital's
		// total number of people present at this time step
		if (agent.infected() == false){
			if ((agent.tested() == true) && 
				(agent.tested_in_hospital() == true) &&
				(agent.get_time_of_test() <= time) && 
		 		(agent.tested_awaiting_test() == true)){
					hospitals.at(agent.get_hospital_ID() - 1).increase_total_tested();
			}			
			continue;
		}

		// Consider all infectious cases, raise 
		// exception if no existing case
		if (agent.exposed() == true){
			contributions.compute_exposed_contributions(agent, time, households, 
							schools, workplaces, hospitals, retirement_homes,
							carpools, public_transit, leisure_locations);
		}else if (agent.symptomatic() == true){
			contributions.compute_symptomatic_contributions(agent, time, households, 
							schools, workplaces, hospitals, retirement_homes,
							carpools, public_transit, leisure_locations);
		}else{
			throw std::runtime_error("Agent does not have any state");
		}
	}
	contributions.total_place_contributions(households, schools, 
											workplaces, hospitals, retirement_homes,
											carpools, public_transit, leisure_locations);
}

// Determine infection propagation and
// state changes 
void ABM::compute_state_transitions()
{
	int newly_infected = 0, is_recovered = 0;
	// Infected state change flags: 
	// recovered - healthy, recovered - dead, tested at this step,
	// tested positive at this step, tested false negative
	std::vector<int> state_changes = {0, 0, 0, 0, 0};
	// Susceptible state changes
	// infected, tested, tested negative, tested false positive
	std::vector<int> s_state_changes = {0, 0, 0, 0};
	// First entry is one if agent recovered, second if agent died
	std::vector<int> removed = {0,0};

	// Store information for that day
	n_infected_day.push_back(0);
	tested_day.push_back(0);
	tested_pos_day.push_back(0);
	tested_neg_day.push_back(0);
	tested_false_pos_day.push_back(0);
	tested_false_neg_day.push_back(0);

	for (auto& agent : agents){

		// Skip the removed and the vaccinated 
		if (agent.removed() == true || agent.vaccinated() == true){
			continue;
		}
		
		std::fill(state_changes.begin(), state_changes.end(), 0);
		std::fill(s_state_changes.begin(), s_state_changes.end(), 0);

		if (agent.infected() == false){
			s_state_changes = transitions.susceptible_transitions(agent, time,
							dt, infection, households, schools, workplaces, 
							hospitals, retirement_homes, carpools, public_transit,
						   	leisure_locations, infection_parameters, 
							agents, flu, testing);
			n_infected_tot += s_state_changes.at(0);
			// True infected by timestep, from the first time step
			if (s_state_changes.at(0) == 1){
				++n_infected_day.back();
				// Saving infected information
				if (collect_data) {
					collect_infected_properties(agent);
				}
			}
		}else if (agent.exposed() == true){
			state_changes = transitions.exposed_transitions(agent, infection, time, dt, 
										households, schools, workplaces, hospitals,
										retirement_homes, carpools, public_transit,
						   				infection_parameters, testing);
			n_recovering_exposed += state_changes.at(0);
			n_recovered_tot += state_changes.at(0);
		}else if (agent.symptomatic() == true){
			state_changes = transitions.symptomatic_transitions(agent, time, dt,
						infection, households, schools, workplaces, hospitals,
							retirement_homes, carpools, public_transit,
						   	infection_parameters);
			n_recovered_tot += state_changes.at(0);
			// Collect only after a specified time
			if (time >= infection_parameters.at("time to start data collection")){
				if (state_changes.at(1) == 1){
					// Dead after testing
					++n_dead_tested;
					++n_dead_tot;
				} else if (state_changes.at(1) == 2){
					// Dead with no testing
					++n_dead_not_tested;
					++n_dead_tot;
				}
			}
		}else{
			throw std::runtime_error("Agent does not have any infection-related state");
		}

		// Recording testing changes for this agent
		if (time >= infection_parameters.at("time to start data collection")){
			if (agent.exposed() || agent.symptomatic()){
				if (state_changes.at(2) == 1){
					++tested_day.back();
					++tot_tested;
				}
				if (state_changes.at(3) == 1){
					++tested_pos_day.back();
					++tot_tested_pos;
				}
				if (state_changes.at(4) == 1){
					++tested_false_neg_day.back();
					++tot_tested_false_neg;
				}
			} else {
				// Susceptible
				if (s_state_changes.at(1) == 1){
					++tested_day.back();
					++tot_tested;
				}
				if (s_state_changes.at(2) == 1){
					++tested_neg_day.back();
					++tot_tested_neg;
				}
				if (s_state_changes.at(3) == 1){
					++tested_false_pos_day.back();
					++tot_tested_false_pos;
				}
			}
		}
	}
}

// Start detection, initialize agents with flu, vaccinate
void ABM::start_testing_flu_and_vaccination()
{
	double tol = 1e-3;
	// Initialize agents with flu the time step the testing starts 
	// Optionally also vaccinate part of the population or/and specific groups
	if (equal_floats<double>(time, infection_parameters.at("start testing"), tol)){
		// Vaccinate
		if (random_vaccines == true){
			vaccinate_random();
		}
		if (group_vaccines == true){
			vaccinate_group();
		}
		if ((group_vaccines == true) && (random_vaccines == true)){
			vaccinate_random();
			vaccinate_group();
		}
		// Initialize flu agents
		for (const auto& agent : agents){
			if (!agent.infected() && !agent.removed() && !agent.vaccinated()){
				// If not patient or hospital employee
				// Add to potential flu group
				if (!agent.hospital_employee() && !agent.hospital_non_covid_patient()){
					flu.add_susceptible_agent(agent.get_ID());
				}
			}
		}
		// Randomly assign portion of susceptible with flu
		// The set agents flags
		std::vector<int> flu_IDs = flu.generate_flu();
		for (const auto& ind : flu_IDs){
			Agent& agent = agents.at(ind-1);
			const int n_hospitals = hospitals.size();
			transitions.process_new_flu(agent, n_hospitals, time,
					   		 schools, workplaces, retirement_homes,
							 carpools, public_transit, infection, 
							 infection_parameters, flu, testing);
		}
	}
}

//
// Getters
//

// Retrieve number of infected agents at this time step
int ABM::get_num_infected() const
{
	int infected_count = 0;
	for (const auto& agent : agents){
		if (agent.infected())
			++infected_count;
	}
	return infected_count;
}

// Retrieve number of exposed agents at this time step
int ABM::get_num_exposed() const
{
	int exposed_count = 0;
	for (const auto& agent : agents){
		if (agent.exposed())
			++exposed_count;
	}
	return exposed_count;
}

// Number of infected - confirmed
int ABM::get_num_active_cases() const
{
	int active_count = 0;
	for (const auto& agent : agents){
		if ((agent.infected() && agent.tested_covid_positive())
			 || (agent.symptomatic_non_covid() && agent.home_isolated()
					 && agent.tested_false_positive())){
			++active_count;
		}
	}
	return active_count;
}

// Number of infected - confirmed
std::vector<int> ABM::get_treatment_data() const
{
	// IH, HN, ICU
	std::vector<int> treatments(3,0);
	for (const auto& agent : agents){
		if (agent.home_isolated()){
			++treatments.at(0);
		}else if (agent.hospitalized()){
			++treatments.at(1);
		}else if (agent.hospitalized_ICU()){
			++treatments.at(2);
		}
	}
	return treatments;
}

// Calculate the average number of contacts an agent makes
void ABM::collect_contacts()
{
	int n_tot = 0, les_loc = 0, counter = 0;
	std::vector<int> temp(agents.size(),0);
	for (const auto& agent : agents){
		n_tot = 0;
		// Removed - dead
		if (agent.removed_dead()) {
			n_tot = -1;
			temp.at(counter) = n_tot;
			++counter;
			continue;
		}
		
		// Tested (and isolated)
		if (agent.tested()){
			if (agent.get_time_of_test() <= time 
					&& agent.tested_awaiting_test() == true){
				// Time of test
				if (agent.tested_in_hospital()) {
					n_tot += hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents();
				} else {
					if (agent.retirement_home_resident()) {
						n_tot += retirement_homes.at(agent.get_household_ID()-1).get_number_of_agents();
					} else {
						n_tot += households.at(agent.get_household_ID()-1).get_number_of_agents();
					}
				}
				temp.at(counter) = n_tot;
				++counter;
				continue;
			}
			// Waiting for test or the results, home isolated unless an exposed hospital employee
			// Non-covid hospital patients - just hospital
			if (!(agent.hospital_employee() && agent.exposed())) {
				if (agent.retirement_home_resident()) {
					n_tot += retirement_homes.at(agent.get_household_ID()-1).get_number_of_agents();
				} else if (agent.hospital_non_covid_patient()) {
					n_tot += hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents();
				} else {
					n_tot += households.at(agent.get_household_ID()-1).get_number_of_agents();
				}
				temp.at(counter) = n_tot;
				++counter;
				continue;
			}
		}

		// Treated
		if (agent.being_treated()){
			if (agent.home_isolated()){
				if (agent.retirement_home_resident()) {
					n_tot += retirement_homes.at(agent.get_household_ID()-1).get_number_of_agents();
				} else {
					n_tot += households.at(agent.get_household_ID()-1).get_number_of_agents();
				}
			} else if (agent.hospitalized()){
				n_tot += hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents();
			} else {
				// ICU
				n_tot += hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents();
			}
			temp.at(counter) = n_tot;
			++counter;
			continue;
		}

		if (agent.hospital_employee()) {
			// This also includes private guests (just don't count them double)
			n_tot += households.at(agent.get_household_ID()-1).get_number_of_agents();
			if (agent.student()) {
				if (time < infection_parameters.at("school closure")) { 
					n_tot += schools.at(agent.get_school_ID()-1).get_number_of_agents();
				}
			}
			n_tot += hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents();
		} else if (agent.hospital_non_covid_patient()) {
			n_tot += hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents();
		} else {
			if (agent.retirement_home_resident()) {
				n_tot += retirement_homes.at(agent.get_household_ID()-1).get_number_of_agents();
			} else {
				n_tot += households.at(agent.get_household_ID()-1).get_number_of_agents();
			}

			if (agent.student()) {
				if (time < infection_parameters.at("school closure")) {
					n_tot += schools.at(agent.get_school_ID()-1).get_number_of_agents();
				}
			}

			if (agent.works()) {
				if (agent.retirement_home_employee()) {
					n_tot += retirement_homes.at(agent.get_work_ID()-1).get_number_of_agents();
				} else if (agent.school_employee()) {
					if (time < infection_parameters.at("school closure")) {
						n_tot += schools.at(agent.get_work_ID()-1).get_number_of_agents();
					}
				} else {
					if (!agent.works_from_home()) {
						n_tot += workplaces.at(agent.get_work_ID()-1).get_number_of_agents();
					}
				}
			}
		}

		// Transit
		if (agent.get_work_travel_mode() == "carpool") {
			n_tot +=  carpools.at(agent.get_carpool_ID()-1).get_number_of_agents();
		} else if (agent.get_work_travel_mode() == "public") {
			n_tot +=  public_transit.at(agent.get_public_transit_ID()-1).get_number_of_agents();
		}

		// Leisure locations
		les_loc = agent.get_leisure_ID();	
		if (les_loc > 0) {
			if (agent.get_leisure_type() == "public") {
				n_tot += leisure_locations.at(les_loc - 1).get_number_of_agents();
			} else {
				// This is not duplicating because it adds the number of agents 
				// in the household this agent visited
				n_tot += households.at(les_loc - 1).get_number_of_agents();
			}
		}
		temp.at(counter) = n_tot;
		++counter;	
	}
	//return (static_cast<double>(n_tot))/(static_cast<double>(agents.size()));
	agent_contacts.push_back(temp);
}

// Calculate the average number of contacts an agent makes
double ABM::get_average_contacts()
{
	int n_tot = 0, les_loc = 0;
	for (const auto& agent : agents){
		if (agent.hospital_employee()) {
			n_tot += households.at(agent.get_household_ID()-1).get_number_of_agents();
			
		if (agent.student()) {
				if (time < infection_parameters.at("school closure")) { 
					n_tot += std::min(schools.at(agent.get_school_ID()-1).get_number_of_agents(),
									static_cast<int>(infection_parameters.at("max contacts at school")));
				}
			}

			n_tot += std::min(hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents(),
								static_cast<int>(infection_parameters.at("max contacts at hospital")));
		} else if (agent.hospital_non_covid_patient()) {
			n_tot += std::min(hospitals.at(agent.get_hospital_ID()-1).get_number_of_agents(),
								static_cast<int>(infection_parameters.at("max contacts at hospital")));
		} else {
			if (agent.retirement_home_resident()) {
				n_tot += std::min(retirement_homes.at(agent.get_household_ID()-1).get_number_of_agents(),
									static_cast<int>(infection_parameters.at("max contacts at RH")));
			} else {
				n_tot += households.at(agent.get_household_ID()-1).get_number_of_agents();
			}

			if (agent.student()) {
				if (time < infection_parameters.at("school closure")) {
					n_tot += std::min(schools.at(agent.get_school_ID()-1).get_number_of_agents(),
									static_cast<int>(infection_parameters.at("max contacts at school")));
				}
			}

			if (agent.works()) {
				if (agent.retirement_home_employee()) {
					n_tot +=  std::min(retirement_homes.at(agent.get_work_ID()-1).get_number_of_agents(),
									static_cast<int>(infection_parameters.at("max contacts at RH")));
				} else if (agent.school_employee()) {
					if (time < infection_parameters.at("school closure")) {
						n_tot += std::min(schools.at(agent.get_work_ID()-1).get_number_of_agents(),
									static_cast<int>(infection_parameters.at("max contacts at school")));
					}
				} else {
					if (!agent.works_from_home()) {
						n_tot +=  std::min(workplaces.at(agent.get_work_ID()-1).get_number_of_agents(),
							static_cast<int>(infection_parameters.at("max contacts at workplace")));
					}
				}
			}
		}

		// Transit
		if (agent.get_work_travel_mode() == "carpool") {
			n_tot +=  carpools.at(agent.get_carpool_ID()-1).get_number_of_agents();
		} else if (agent.get_work_travel_mode() == "public") {
			n_tot +=  public_transit.at(agent.get_public_transit_ID()-1).get_number_of_agents();
		}

		// Leisure locations
		les_loc = agent.get_leisure_ID();	
		if (les_loc > 0) {
			if (agent.get_leisure_type() == "public") {
				n_tot += leisure_locations.at(les_loc - 1).get_number_of_agents();
			} else {
				n_tot += households.at(les_loc - 1).get_number_of_agents();
			}
		}	
	}
	return (static_cast<double>(n_tot))/(static_cast<double>(agents.size()));
}

//
// I/O
//

// General function for reading an object from a file
std::vector<std::vector<std::string>> ABM::read_object(std::string fname)
{
	// AbmIO settings
	std::string delim(" ");
	bool sflag = true;
	std::vector<size_t> dims = {0,0,0};

	// 2D vector with a parsed line in each inner
	// vector
	std::vector<std::vector<std::string>> obj_vec_2D;

	// Read and return a copy
	AbmIO abm_io(fname, delim, sflag, dims);
	obj_vec_2D = abm_io.read_vector<std::string>();

	return obj_vec_2D;
}

//
// Saving simulation state
//

// Save infection information
void ABM::print_infection_parameters(const std::string filename) const
{
	FileHandler file(filename, std::ios_base::out | std::ios_base::trunc);
	std::fstream &out = file.get_stream();	

	for (const auto& entry : infection_parameters){
		out << entry.first << " " << entry.second << "\n";
	}	
}

// Save age-dependent distributions
void ABM::print_age_dependent_distributions(const std::string filename) const
{
	FileHandler file(filename, std::ios_base::out | std::ios_base::trunc);
	std::fstream &out = file.get_stream();	

	for (const auto& entry : age_dependent_distributions){
		out << entry.first << "\n";
		for (const auto& e : entry.second)
			out << e.first << " " << e.second << "\n";
	}	
}


// Save current household information to file 
void ABM::print_households(const std::string fname) const
{
	print_places<Household>(households, fname);
}	

// Save current school information to file 
void ABM::print_schools(const std::string fname) const
{
	print_places<School>(schools, fname);
}


// Save current workplaces information to file 
void ABM::print_workplaces(const std::string fname) const
{
	print_places<Workplace>(workplaces, fname);
}

// Save current hospital information to file 
void ABM::print_hospitals(const std::string fname) const
{
	print_places<Hospital>(hospitals, fname);
}

// Save current retirement home information to file 
void ABM::print_retirement_home(const std::string fname) const
{
	print_places<RetirementHome>(retirement_homes, fname);
}

// Save current information on leisure locations to file 
void ABM::print_leisure_locations(const std::string& fname) const
{
	print_places<Leisure>(leisure_locations, fname);
}

// Save current information on transit type 
void ABM::print_transit(const std::string& fname, const std::string& mode) const
{
	if (mode == "carpool") {
		print_places<Transit>(carpools, fname);
	} else if (mode == "public") {
		print_places<Transit>(public_transit, fname);
	} else {
		throw std::invalid_argument("Wrong type of transit to print. Acceptable types: carpool or public");
	}	
}

// Save IDs of all agents in all households
void ABM::print_agents_in_households(const std::string filename) const
{
	print_agents_in_places<Household>(households, filename);
}

// Save IDs of all agents in all schools
void ABM::print_agents_in_schools(const std::string filename) const
{
	print_agents_in_places<School>(schools, filename);
}

// Save IDs of all agents in all workplaces 
void ABM::print_agents_in_workplaces(const std::string filename) const
{
	print_agents_in_places<Workplace>(workplaces, filename);
}

// Save IDs of all agents in all hospitals 
void ABM::print_agents_in_hospitals(const std::string filename) const
{
	print_agents_in_places<Hospital>(hospitals, filename);
}

// Save current agent information to file 
void ABM::print_agents(const std::string fname) const
{
	// AbmIO settings
	std::string delim(" ");
	bool sflag = true;
	std::vector<size_t> dims = {0,0,0};	

	// Write data to file
	AbmIO abm_io(fname, delim, sflag, dims);
	abm_io.write_vector<Agent>(agents);	
}

// Save information on the newly infected agent
void ABM::collect_infected_properties(const Agent& agent)
{
	std::map<std::string, std::string> s_temp;
	std::map<std::string, bool> b_temp;
	std::map<std::string, double> d_temp;

	// Residence			
	if (agent.retirement_home_resident()) {
		const RetirementHome& res = retirement_homes.at(agent.get_household_ID()-1);
		d_temp["number of housemates"] = static_cast<double>(res.get_number_of_agents());
		d_temp["residence lambda"] = res.get_infected_contribution();
	} else if (agent.hospital_non_covid_patient()) {
		const Hospital& res = hospitals.at(agent.get_hospital_ID()-1);
		d_temp["number of housemates"] = static_cast<double>(res.get_number_of_agents());
		d_temp["residence lambda"] = res.get_infected_contribution();
	} else {
		const Household& res = households.at(agent.get_household_ID()-1);
		d_temp["number of housemates"] = static_cast<double>(res.get_number_of_agents());
		d_temp["residence lambda"] = res.get_infected_contribution();
	}

	// Work-related data collection
	if (agent.works() && !agent.works_from_home()) {
		// Check for each workplace location
		if (agent.hospital_employee()) {
			d_temp["work lambda"] = hospitals.at(agent.get_hospital_ID()-1).get_infected_contribution();
			b_temp["works outside"] = false;
		} else if (agent.retirement_home_employee()) {
			d_temp["work lambda"] = retirement_homes.at(agent.get_work_ID()-1).get_infected_contribution();
			b_temp["works outside"] = false;
		} else if (agent.school_employee()) {
			d_temp["work lambda"] = schools.at(agent.get_work_ID()-1).get_infected_contribution();
			b_temp["works outside"] = false;
		} else {
			const Workplace& work = workplaces.at(agent.get_work_ID()-1);
			d_temp["work lambda"] = work.get_infected_contribution();
			// Out-of-town work
			if (work.outside_town()) {
				b_temp["works outside"] = true;
			} else {
				b_temp["works outside"] = false;
			}
		}
	} else {
		b_temp["works outside"] = false;
		d_temp["work lambda"] = 0.0;
	}

	//----Lambdas
	// Transit
	if (agent.works() && !agent.works_from_home()) {
		if (agent.get_work_travel_mode() == "carpool") {
			d_temp["transit lambda"] = carpools.at(agent.get_carpool_ID()-1).get_infected_contribution();
		} else if (agent.get_work_travel_mode() == "public") {
			d_temp["transit lambda"] = public_transit.at(agent.get_public_transit_ID()-1).get_infected_contribution();
		}
	} else {
		d_temp["transit lambda"] = 0.0;
	}
	// School
	if (agent.student()){
		d_temp["school lambda"] = schools.at(agent.get_school_ID()-1).get_infected_contribution();
	} else{d_temp["school lambda"] = 0.0;}
	// Leisure locations
	if (agent.get_leisure_ID() > 0){
		if (agent.get_leisure_type() == "public") {
			s_temp["leisure type"] == "public";
			d_temp["leisure lambda"] = leisure_locations.at(agent.get_leisure_ID()-1).get_infected_contribution();
		} else if (agent.get_leisure_type() == "household") {
			s_temp["leisure type"] == "private";
			d_temp["leisure lambda"] = households.at(agent.get_leisure_ID()-1).get_infected_contribution();
		} else {
			throw std::invalid_argument("Wrong leisure type: " + agent.get_leisure_type());
		}
	} else {
		s_temp["leisure type"] == "none";
		d_temp["leisure lambda"] = 0.0;
	}

	//----Collecting remaining string properties
	// Transit mode, by default accounts for no work and work from home
	s_temp["transit mode"] = agent.get_work_travel_mode();
	// Occupation, default is "none" if no work
	s_temp["occupation"] = agent.get_occupation();
	
	//----Collecting remaining boolean properties
	b_temp["non-covid hospital patient"] = agent.hospital_non_covid_patient();
	b_temp["false positive"] = agent.tested_false_positive();
	b_temp["retirement home resident"] = agent.retirement_home_resident();
	b_temp["school employee"] = agent.school_employee();
	b_temp["retirement home employee"] = agent.retirement_home_employee();
	b_temp["hospital employee"] = agent.hospital_employee();
	b_temp["retirement home employee"] = agent.retirement_home_employee();
	b_temp["false positive"] = agent.tested_false_positive();
	b_temp["flu"] = agent.symptomatic_non_covid();

	//----Collecting remaining double properties
	d_temp["time infected"] = time;
	d_temp["agent ID"] = static_cast<double>(agent.get_ID());
	d_temp["age"] = static_cast<double>(agent.get_age());
	d_temp["household ID"] = static_cast<double>(agent.get_household_ID()-1);
	d_temp["residence x coordinate"] = agent.get_x_location();
	d_temp["residence y coordinate"] = agent.get_y_location();
	
	// Add temporary maps to the relevant vectors
	agent_prop_bool_flags.push_back(b_temp);
	agent_prop_string_values.push_back(s_temp);
	agent_prop_doubles.push_back(d_temp);
}

void ABM::save_infected_data(const std::string& filename) const 
{
	std::vector<std::string> infected_agents;
	std::string entry = "";

	// Collect keys as header titles
	for (const auto& pair: agent_prop_string_values.at(0)){
		entry += pair.first + "\t";
	}
	for (const auto& pair: agent_prop_doubles.at(0)){
		entry += pair.first + "\t";
	}
	for (const auto& pair: agent_prop_bool_flags.at(0)){
		entry += pair.first + "\t";
	}
	entry += "\n";
	// Save keys as first entry
	infected_agents.push_back(entry);
	entry = "";

	// Check size for one of the vectors, all three should have the same size
	size_t n_agents = agent_prop_string_values.size();

	// Collect all map values for each vector entry
	for (int i=0; i<n_agents; i++){
		for (const auto& pair: agent_prop_string_values.at(i)){
			entry += pair.second + "\t";
		}
		for (const auto& pair: agent_prop_doubles.at(i)){
			entry += std::to_string(pair.second) + "\t";
		}
		for (const auto& pair: agent_prop_bool_flags.at(i)){
			entry += std::to_string(pair.second) + "\t";
		}
		entry += "\n";
		infected_agents.push_back(entry);
		entry = "";
	}

	// AbmIO settings
	std::string delim(" ");
	bool sflag = true;
	std::vector<size_t> dims = {0,0,0};	

	// Write data to file
	AbmIO abm_io(filename, delim, sflag, dims);
	abm_io.write_vector<std::string>(infected_agents);	

	// Clear vector
	infected_agents.clear();
}
