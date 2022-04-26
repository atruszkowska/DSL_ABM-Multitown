clear
load('fixed_vac_var_reopening.mat')

counter = 0;
arr_counter = 1;
for ai = 1:n_agents
   ind = find(average_contacts(:,ai) == -1);
   if (isempty(ind))
       active_contacts(:,arr_counter) = average_contacts(:,ai);
       arr_counter = arr_counter + 1;
   else
       counter = coutner + 1;
   end
end