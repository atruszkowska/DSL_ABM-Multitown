%
% Total cases and total deaths vs. reopening rate
% Loads and plots the final numbers vs. the reopening rate 
%

clear; close all

% Input - Colonie
% Number of independent simulations in each set
num_sim = 100;
% Common file name
mname = 'dir';

pop_NR = 79205;

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

%
% Plot results from all simulations
%

% Plot settings
clrf = [0, 0, 127/255; 170/255, 0/255, 0/255; 255/255, 85/255, 0/255];
clrm = [0, 0, 127/255; 170/255, 0/255, 0/255; 255/255, 85/255, 0/255];

% When to start plotting (w.r.t. x)
% This skips the very small entries
nst = 3;

% New Rochelle
NR_all = load('NewRochelleData');
NR_deaths = NR_all.total_deaths;
NR_cases = NR_all.total_cases;

plot_title = '';
ylab = 'Total deaths, %population';
ylimits = [0,0.3];
plot_all_and_mean(reopening_rates(nst:end), total_deaths(:,nst:end)/pop_NR*100, NR_deaths(:,nst:end)/pop_NR*100, 1, clrf, clrf, plot_title, ylab, ylimits)

ylab = 'Total infected, %population';
ylimits = [0,30];
plot_all_and_mean(reopening_rates(nst:end), total_cases(:,nst:end)/pop_NR*100, NR_cases(:,nst:end)/pop_NR*100, 2, clrf, clrf, plot_title, ylab, ylimits)

function plot_all_and_mean(x, y, y2, i, clrm, clrf, plot_title, ylab, ylimits)

    % Create figure
    figure1 = figure(i);

   
    % Create axes
    axes1 = axes('Parent',figure1);
 
    % Convert to %
    x = x'*100;

    % Coarse NR
    boxplot(y, x,'BoxStyle', 'filled', 'Colors', clrf(1,:), 'Symbol', '')
    % Box width
    a = get(get(gca,'children'),'children');   % Get the handles of all the objects
    t = get(a,'tag');   % List the names of all the objects 
    idx=strcmpi(t,'box');  % Find Box objects
    boxes=a(idx);          % Get the children you need
    set(boxes,'linewidth',10); % Set width
    % Median color
    lines = findobj(gcf, 'type', 'line', 'Tag', 'Median');
    set(lines, 'Color', clrm(1,:), 'LineWidth', 2);
    % Outliers
    ho = findobj(gcf,'tag','Outliers');
    set(ho, 'MarkerEdgeColor', clrm(1,:), 'LineWidth', 2)
    hold on
    
    % Fine NR
    boxplot(y2, x,'BoxStyle', 'filled', 'Colors', clrf(2,:), 'Symbol', '')
    % Box width
    a = get(get(gca,'children'),'children');   % Get the handles of all the objects
    a = a{2};
    t = get(a,'tag');   % List the names of all the objects 
    idx=strcmpi(t,'box');  % Find Box objects
    boxes=a(idx);          % Get the children you need
    set(boxes,'linewidth',10); % Set width
    % Median color
    lines = findobj(gcf, 'type', 'line', 'Tag', 'Median')
    lines = lines(1:11);
    set(lines, 'Color', clrm(2,:), 'LineWidth', 2);
    % Outliers
%     ho = findobj(gcf,'tag','Outliers');
%     ho = ho(12:end);
%     set(ho, 'MarkerEdgeColor', clrm(2,:), 'LineWidth', 2)
  
       % Ticks and tick labesl
    ylabel(ylab);

    % Create xlabel
%     xlabel('Reopening rate, \%/day','Interpreter','latex');
    xlabel('Reopening rate, %/day');

    % Create title
    title(plot_title,'Interpreter','latex');

    % Uncomment the following line to preserve the Y-limits of the axes
    % ylim(axes1,[0 5]);
    box(axes1,'on');
    % Set the remaining axes properties
%     set(axes1,'FontSize',24,'TickLabelInterpreter','latex','XGrid','on','YGrid',...
%         'on'); 
     set(axes1,'FontSize',28,'FontName','SanSerif','XGrid','off','YGrid',...
        'off'); 
  
%     set(axes1, 'XScale', 'log');
    ax = gca;
%         ax.YAxis.TickLabelFormat = '%.0e';
%     ax.YAxis.Exponent = 0;
%     ax.YAxis.TickLabelFormat = '%,.0f';
    
%     xlim([0 10])
%     xticks([0 2.5 5 7.5 10])
    ylim(ylimits)
    
    set(gcf,'Position',[200 500 950 750])

end
