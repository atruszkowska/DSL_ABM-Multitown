% Collects data for each town

clear; close all

% Input
% Number of independent simulations in each set
num_sim = 400;
% Common file name
mname = 'dir';

% Files to consider
dir_names = dir([mname, '_*']);
dir_names = {dir_names.name};
ndirs = length(dir_names);
% Extract the numbers in file endings (these are reopening rates)
str=sprintf('%s#', dir_names{:});
num = sscanf(str, [mname,'_%f#']);
% Sort by the numbers - reopening rates
[~,idx]=sort(num);

total_cases = zeros(num_sim, ndirs);
total_deaths = zeros(num_sim, ndirs);
reopening_rates = num;

for ii = 1:length(idx)
    % To see the current file
    fprintf('Processing: %s\n', dir_names{idx(ii)})
    temp = load([dir_names{idx(ii)},'/fixed_vac_var_reopening.mat']);
    total_deaths(:,ii) = temp.tot_deaths(:,end);
    total_cases(:,ii) = temp.tot_infected(:,end);
end

save('CoarseNewRochelleData')