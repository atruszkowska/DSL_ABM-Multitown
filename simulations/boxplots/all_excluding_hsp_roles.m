%
% Boxplots for all the towns and each case where a location type was
%   selectively excluded 
%

clear
close all

% Rate to plot
% 5: 1%
num_rate = 1;

% Number of independent simulations in each set
num_sim = 400;

% Plot colors (Blue - Colonie, Red - Coarse New Rochelle, Orange - Utica)
colors = [0, 0, 127/255; 170/255, 0/255, 0/255; 255/255, 85/255, 0/255;];

% Town populations
pop_C = 82797;
pop_NR = 79205;
pop_U = 59750;

% Directories to consider
main_data_dirs = {  '../hsp_nc_patients/perfect_testing/Colonie/';...
                    '../hsp_tested/perfect_testing/Colonie/';...
                    '../hsp_staff/perfect_testing/Colonie/';...
                    '../hsp_covid_patients/perfect_testing/Colonie/';...
                    '../hsp_icu_patients/perfect_testing/Colonie/'};
% Boxplot labels
box_labels = {'Other', 'Tested', 'Staff', ...
                'Regular', 'ICU'};
                
% Number of datasets
num_studies = size(main_data_dirs,1);            

Colonie_all_cases = zeros(num_sim, num_studies);
Coarse_New_Rochelle_all_cases = zeros(num_sim, num_studies);
Utica_all_cases = zeros(num_sim,num_studies);

Colonie_all_deaths = zeros(num_sim, num_studies);
Coarse_New_Rochelle_all_deaths = zeros(num_sim, num_studies);
Utica_all_deaths = zeros(num_sim,num_studies);

% Collect all the data                
for jd = 1:num_studies
    dataset = cell2mat(main_data_dirs(jd,:));
    
    % Colonie
    cur_dataset = strcat(dataset, 'ColonieData');
    Colonie_all = load(cur_dataset);
    Colonie_all_cases(:,jd) = Colonie_all.total_cases(:,num_rate);
    Colonie_all_deaths(:,jd) = Colonie_all.total_deaths(:,num_rate);
    
    % Coarse New Rochelle
    cur_dataset = strcat(dataset, 'CoarseNewRochelleData');
    Coarse_New_Rochelle_all = load(cur_dataset);
    Coarse_New_Rochelle_all_cases(:,jd) = Coarse_New_Rochelle_all.total_cases(:,num_rate);
    Coarse_New_Rochelle_all_deaths(:,jd) = Coarse_New_Rochelle_all.total_deaths(:,num_rate);
    
    % Utica
    cur_dataset = strcat(dataset, 'UticaData');
    Utica_all = load(cur_dataset);
    Utica_all_cases(:,jd) = Utica_all.total_cases(:,num_rate);
    Utica_all_deaths(:,jd) = Utica_all.total_deaths(:,num_rate);
end

% Flatten for the box plots
Colonie_all_cases = reshape(Colonie_all_cases, [num_sim*num_studies, 1]);
Coarse_New_Rochelle_all_cases = reshape(Coarse_New_Rochelle_all_cases, [num_sim*num_studies, 1]);
Utica_all_cases = reshape(Utica_all_cases, [num_sim*num_studies, 1]);

Colonie_all_deaths = reshape(Colonie_all_deaths, [num_sim*num_studies, 1]);
Coarse_New_Rochelle_all_deaths = reshape(Coarse_New_Rochelle_all_deaths, [num_sim*num_studies, 1]);
Utica_all_deaths = reshape(Utica_all_deaths, [num_sim*num_studies, 1]);

% Generate sequential box lables for all studies (boxplots) of one town
for jl = 1:num_studies
    box_label_array(jl) = {repmat({cell2mat(box_labels(jl))},num_sim,1)};
end
box_label_array = vertcat(box_label_array{:});

ylims = [0,6];
plot_one_town(box_label_array, Colonie_all_cases/pop_C*100, Coarse_New_Rochelle_all_cases/pop_NR*100, Utica_all_cases/pop_U*100, colors, 'Infected (\% population)', ylims, 1);

ylims = [0,0.08];
plot_one_town(box_label_array, Colonie_all_deaths/pop_C*100, Coarse_New_Rochelle_all_deaths/pop_NR*100, Utica_all_deaths/pop_U*100, colors, 'Deaths (\% population)', ylims, 2);

% Plot box plots for all studies and three towns
function plot_one_town(labels, data1, data2, data3, colors, ylab, ylims, ifig)

    % Create figure
    figure1 = figure(ifig);
   
    % Create axes
    axes1 = axes('Parent',figure1);

    % Town 1    
    boxplot(data1, labels,'BoxStyle', 'filled', 'Colors', colors(1,:), 'Symbol', '')
    % Box width
    a = get(get(gca,'children'),'children');   % Get the handles of all the objects
    t = get(a,'tag');   % List the names of all the objects 
    idx=strcmpi(t,'box');  % Find Box objects
    boxes=a(idx);          % Get the children you need
    set(boxes,'linewidth',10); % Set width
    % Median color
    lines = findobj(gcf, 'type', 'line', 'Tag', 'Median');
    set(lines, 'Color', colors(1,:), 'LineWidth', 2);
    % Outliers
%     ho = findobj(gcf,'tag','Outliers');
%     set(ho, 'MarkerEdgeColor', clrm(1,:), 'LineWidth', 2)
    hold on
    
    % Coarse New Rochelle
    boxplot(data2, labels,'BoxStyle', 'filled', 'Colors', colors(2,:), 'Symbol', '')
    % Box width
    a = get(get(gca,'children'),'children');   % Get the handles of all the objects
    a = a{1};
    t = get(a,'tag');   % List the names of all the objects 
    idx=strcmpi(t,'box');  % Find Box objects
    boxes=a(idx);          % Get the children you need
    set(boxes,'linewidth',10); % Set width
    % Median color
    lines = findobj(gcf, 'type', 'line', 'Tag', 'Median');
    lines = lines(1:5);
    set(lines, 'Color', colors(2,:), 'LineWidth', 2);
    % Outliers
%     ho = findobj(gcf,'tag','Outliers');
%     ho = ho(1:12);
%     set(ho, 'MarkerEdgeColor', colors(2,:), 'LineWidth', 2)

    % Utica
    boxplot(data3, labels,'BoxStyle', 'filled', 'Colors', colors(3,:), 'Symbol', '')
    % Box width
    a = get(get(gca,'children'),'children');   % Get the handles of all the objects
    a = a{1};
    t = get(a,'tag');   % List the names of all the objects 
    idx=strcmpi(t,'box');  % Find Box objects
    boxes=a(idx);          % Get the children you need
    set(boxes,'linewidth',10); % Set width
    % Median color
    lines = findobj(gcf, 'type', 'line', 'Tag', 'Median');
    lines = lines(1:5);
    set(lines, 'Color', colors(3,:), 'LineWidth', 2);
%     % Outliers
%     ho = findobj(gcf,'tag','Outliers');
%     ho = ho(1:12);
%     set(ho, 'MarkerEdgeColor', clrm(4,:), 'LineWidth', 2)
%     

    % Ticks and tick labesl
    ylabel(ylab, 'Interpreter','latex');
    ylim(ylims)
    
    % Create xlabel
%     xlabel('Reopening rate, \%/day','Interpreter','latex');
%     xlabel('Reopening rate, %/day');

    % Create title
%     title(plot_title,'Interpreter','latex');

    % Uncomment the following line to preserve the Y-limits of the axes
    % ylim(axes1,[0 5]);
    box(axes1,'on');
    % Set the remaining axes properties
    set(axes1,'FontSize',24,'TickLabelInterpreter','latex','XGrid','on','YGrid',...
        'on'); 
%      set(axes1,'FontSize',28,'FontName','SanSerif','XGrid','off','YGrid',...
%         'off'); 
  
%     set(axes1, 'XScale', 'log');
    ax = gca;
    
%     ax.YAxis.TickLabelFormat = '%.0e';
%     ax.YAxis.Exponent = 0;
%     ax.YAxis.TickLabelFormat = '%,.0f';
    
%     xlim([0 10])
%     xticklabels({'0','1e-e3','1e-2','0.1','1','2','3','4','5','6','7', '8', '9', '10'})
%     ylim(ylimits)
    
    set(gcf,'Position',[2360 364 889 622])

end



















