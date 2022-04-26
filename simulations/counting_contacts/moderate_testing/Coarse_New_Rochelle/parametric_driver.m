%% Run a number of COVID simulations and store results

clear
close all

num_sim = 100;
num_steps = 400;
dt = 0.25;
time=0:dt:dt*num_steps;
n_agents = 79205;

% All the collected data
cur_infected = zeros(num_sim, num_steps+1);
tot_infected = zeros(num_sim, num_steps+1);
tot_deaths = zeros(num_sim, num_steps+1);

% Contacts
average_contacts = zeros(num_steps+1, n_agents);

for i=1:num_sim
   !./covid_exe >> output/simulation.log
   % Collect 
   cur_infected(i,:) = load('output/infected_with_time.txt');
   tot_infected(i,:) = load('output/total_infected.txt');
   tot_deaths(i,:) = load('output/dead_with_time.txt');
   
   temp = load('output/contacts_with_time.txt');
   
   average_contacts = average_contacts + temp;
end

average_contacts = average_contacts/num_sim;
save('fixed_vac_var_reopening.mat')
