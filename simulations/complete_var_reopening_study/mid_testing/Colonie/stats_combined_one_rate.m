%
% Active cases, total cases, and total deaths for the three towns
%   at a fixed reopening rate as a function of time
%

clear
close all

% Reopening rate to select
re_rate = 0.0;

% Number of independent simulations in each set
num_sim = 400;
% Number of time steps
num_steps = 361;
% Number of towns
num_towns = 3;

% Plot colors (Blue - Colonie, Red - Coarse New Rochelle, Orange - Utica)
% Color of the mean
clrm = [0, 0, 127/255; 170/255, 0/255, 0/255; 255/255, 85/255, 0/255;];
% Color of the background plots (individual realizations)
clrf = [181/255, 192/255, 255/255; 252/255, 189/255, 187/255; 255/255, 189/255, 144/255;];

% Town populations
pop_C = 82797;
pop_NR = 79205;
pop_U = 59750;

% Data for all three towns
active_all = zeros(num_sim, num_steps, num_towns);
cases_all = zeros(num_sim, num_steps, num_towns);
deaths_all = zeros(num_sim, num_steps, num_towns);

% Load the data
% Directory name (may need formatting adjustments)
base_data_dir = sprintf('dir_%.1f/fixed_vac_var_reopening.mat', re_rate);

% Colonie
data_dir = base_data_dir;
nt = 1;
temp = load(data_dir);
active_all(:,:,nt) = temp.cur_infected/pop_C*100;
cases_all(:,:,nt) = temp.tot_infected/pop_C*100;
deaths_all(:,:,nt) = temp.tot_deaths/pop_C*100;

% Time - common
time = temp.time;

% New Rochelle
data_dir = strcat('../Coarse_New_Rochelle/', base_data_dir);
nt = 2;
temp = load(data_dir);
active_all(:,:,nt) = temp.cur_infected/pop_NR*100;
cases_all(:,:,nt) = temp.tot_infected/pop_NR*100;
deaths_all(:,:,nt) = temp.tot_deaths/pop_NR*100;

% Utica
data_dir = strcat('../Utica/', base_data_dir);
nt = 3;
temp = load(data_dir);
active_all(:,:,nt) = temp.cur_infected/pop_U*100;
cases_all(:,:,nt) = temp.tot_infected/pop_U*100;
deaths_all(:,:,nt) = temp.tot_deaths/pop_U*100;

% Plots
plot_towns_stats_w_time(time, active_all, 1, num_towns, num_sim, clrf, clrm, 'Active cases (\% population)')
plot_towns_stats_w_time(time, cases_all, 2, num_towns, num_sim, clrf, clrm, 'Infections (\% population)')
plot_towns_stats_w_time(time, deaths_all, 3, num_towns, num_sim, clrf, clrm, 'Deaths (\% population)')

function plot_towns_stats_w_time(time, data, ifig, ntowns, nsim, clrf, clrm, ylab)
    
    % Create figure
    figure1 = figure(ifig);

    % Create axes
    axes1 = axes('Parent',figure1);

    for it = 1:ntowns
%        
% Largest and smallest
%

%         min_peak = 1000000;
%         max_peak = -min_peak;
%         min_js = 0;
%         max_js = 0;
%         for js=1:nsim
%             % Find the largest and the smallest then plot
%             min_temp = max(data(js,:,it));
%             max_temp = max(data(js,:,it));
%             if min_temp < min_peak
%                 min_peak = min_temp;
%                 min_js = js;
%             end
%             if max_temp > max_peak
%                 max_peak = max_temp;
%                 max_js = js;                
%             end
%         end
%         plot(time, data(max_js,:,it), '--', 'LineWidth', 3, 'Color', clrf(it,:))
%         hold on
%         plot(time, data(min_js,:,it), '--', 'LineWidth', 3, 'Color', clrf(it,:))

%
% 25th and 75th percentile
% 

        plot(time, prctile(data(:,:,it),25), '--', 'LineWidth', 3, 'Color', clrf(it,:))
        hold on
        plot(time, prctile(data(:,:,it),75), '--', 'LineWidth', 3, 'Color', clrf(it,:))


        plot(time, mean(data(:,:,it),1), 'LineWidth', 2, 'Color', clrm(it,:))    
    end

    % Formatting

    ylabel(ylab,'Interpreter','latex');
%     ylabel(ylab);

    % Create xlabel
    xlabel('Time (days)','Interpreter','latex');
%     xlabel('Time, days');

    % Ticks
    xticks([0 30 60 90])
%     xticklabels({'Sept 7','Nov 6','Jan 5','Mar 6'})
    %xtickangle(45)

    % Uncomment the following line to preserve the Y-limits of the axes
    % ylim(axes1,[0 5]);
    box(axes1,'on');
    
    % Set the remaining axes properties
    set(axes1,'FontSize',36,'TickLabelInterpreter','latex','XGrid','on','YGrid',...
        'on'); 
%     set(axes1,'FontSize',28,'FontName', 'Arial','XGrid','on','YGrid',...
%         'on'); 
    xlim([0,90])
    
    % Size
    set(gcf,'Position',[200 500 950 750])
    
    
end