function plot_results
% Plot precomputed means with half-range bars; invalid sentinels have no mean.
root=fileparts(mfilename('fullpath'));
opts=detectImportOptions(fullfile(root,'data','summary.csv'),'TextType','string');
opts=setvartype(opts,'condition','string');
t=readtable(fullfile(root,'data','summary.csv'),opts);
f=figure('Visible','off','Position',[100 100 1300 600]);
cleanup=onCleanup(@()close(f));
experiments=["material" "incidence_deg" "offset_cm"];
titles=["Target material" "Incidence angle (degrees)" "Lateral offset (cm)"];
for k=1:3
 subplot(1,3,k);hold on;
 for sensor=["ToF" "Ultrasonic"]
  rows=t(t.experiment==experiments(k) & t.sensor==sensor,:);
  errorbar(1:height(rows),rows.mean_cm,rows.half_range_cm,'o-','DisplayName',sensor,'LineWidth',1.2);
 end
 xticks(1:height(rows));xticklabels(rows.condition);if k==1,xtickangle(60);end
 title(titles(k));ylabel('Mean distance (cm)');grid on;
end
legend('Location','northwest');sgtitle({'Rover sensor bench measurements: five trials per condition', ...
 'Bars: half range. ToF 6 cm offset and ultrasonic sponge: all five readings invalid, omitted.'});
exportgraphics(f,fullfile(root,'results.png'),'Resolution',180);
end
