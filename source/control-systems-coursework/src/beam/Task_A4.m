clear; clc; close all;

%% A1
betaL = linspace(0, 12, 2000);
f = sin(betaL);

n = 1:3;
roots_betaL = n*pi;
roots_betaL_sq = roots_betaL.^2;

figure('Color','w','Position',[100 100 800 450]);
plot(betaL, f, 'b-', 'LineWidth', 1.6); hold on;
yline(0, 'k--', 'LineWidth', 0.8);

plot(roots_betaL, zeros(size(roots_betaL)), 'ro', ...
     'MarkerSize', 10, 'MarkerFaceColor', 'r', 'LineWidth', 1.5);

for k = 1:length(roots_betaL)
    txt = sprintf('  n=%d\n  \\betaL = %d\\pi\n  (\\betaL)^2 = %.3f', ...
                  k, k, roots_betaL_sq(k));
    yoff = 0.45 * (-1)^(k+1);   % alternate above/below
    text(roots_betaL(k), yoff, txt, ...
         'FontSize', 10, 'HorizontalAlignment', 'left');
end

xlabel('\betaL', 'FontSize', 12);
ylabel('sin(\betaL)', 'FontSize', 12);
title('Characteristic equation for simply supported beam: sin(\betaL) = 0', ...
      'FontSize', 12);
grid on;
xlim([0 12]); ylim([-1.4 1.4]);
legend('sin(\betaL)', 'zero line', 'Roots (modes 1-3)', ...
       'Location', 'southeast');

fprintf('\n--- Task A.1 Results ---\n');
fprintf(' Mode n   betaL      (betaL)^2\n');
fprintf('-------------------------------\n');
for k = 1:3
    fprintf('   %d    %d*pi=%.4f   %.4f\n', ...
            k, k, roots_betaL(k), roots_betaL_sq(k));
end

%% A3
L = 6;
x = linspace(0, L, 500);

figure('Color','w','Position',[100 100 800 500]);
hold on;

colors = {'b', 'r', 'g'};
for n = 1:3
    Yn = sin(n*pi*x/L);
    plot(x, Yn, 'LineWidth', 1.8, 'Color', colors{n}, ...
         'DisplayName', sprintf('Mode %d: Y_%d(x) = sin(%d\\pi x/L)', n, n, n));
end

yline(0, 'k--', 'LineWidth', 0.6, 'HandleVisibility','off');
plot(0, 0, 'k^', 'MarkerSize', 12, 'MarkerFaceColor', 'k', ...
     'HandleVisibility','off');
plot(L, 0, 'k^', 'MarkerSize', 12, 'MarkerFaceColor', 'k', ...
     'HandleVisibility','off');

xlabel('x (m)', 'FontSize', 12);
ylabel('Y_n(x)  (normalised)', 'FontSize', 12);
title('First three mode shapes of simply supported beam', 'FontSize', 12);
legend('Location', 'best', 'FontSize', 10);
grid on;
xlim([-0.2 L+0.2]);
ylim([-1.3 1.3]);

%% Task A4
h = 0.1; d = 0.05; L = 6; rho = 7800; E = 210e9;
I  = (1/12)*d*h^3;
A  = d*h;
m  = rho*A;
EI = E*I;

Y    = @(x) sin(pi*x/L);
Ypp  = @(x) -(pi/L)^2 * sin(pi*x/L);

num  = integral(@(x) EI * Ypp(x).^2, 0, L);
den  = integral(@(x) m  * Y(x).^2,   0, L);

omega_rayleigh = sqrt(num/den);
omega_exact    = pi^2 * sqrt(EI/(m*L^4));

fprintf('Rayleigh estimate : omega_1 = %.4f rad/s, f_1 = %.4f Hz\n', ...
        omega_rayleigh, omega_rayleigh/(2*pi));
fprintf('Exact (Task A.2)  : omega_1 = %.4f rad/s, f_1 = %.4f Hz\n', ...
        omega_exact, omega_exact/(2*pi));
fprintf('Relative error    : %.2e %%\n', ...
        100*abs(omega_rayleigh - omega_exact)/omega_exact);