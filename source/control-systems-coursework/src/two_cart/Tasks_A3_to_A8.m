%%%%%%% A3_6 adjusted for A8

m1 = 2;
m2 = 1;
k1 = 1000;
k2 = 1500;
b1 = 0.5;
b2 = 1.0;

A = [0, 0, k1, 0;
     0, 0, -k2, k2;
     -1/m1, 1/m1, -b1/m1, 0;
     0, -1/m2, 0, -b2/m2];
B = [0, 0;
     0, 0;
     1/m1, 0;
     0, 1/m2];
C = [1/k1, 0, 0, 0;
     1/k1, 1/k2, 0, 0];
D = [0, 0;
     0, 0];

x0_1_pos = [0.1; 0; 0; 0];
x0_1 = [k1 * x0_1_pos(1);          
        k2 * (x0_1_pos(2) - x0_1_pos(1)); 
        x0_1_pos(3); 
        x0_1_pos(4)];
x0_2_pos = [0.5; -0.4; 0; 0];
x0_2 = [k1 * x0_2_pos(1); 
        k2 * (x0_2_pos(2) - x0_2_pos(1)); 
        x0_2_pos(3); 
        x0_2_pos(4)];

sys = ss(A, B, C, D); % state space system

t = 0:0.01:20;
[y1, t1, x1] = initial(sys, x0_1, t); 
[y2, t2, x2] = initial(sys, x0_2, t);
figure;
subplot(2,1,1); plot(t, y1); legend('x1', 'x2'); title('Initial Condition 1');
subplot(2,1,2); plot(t, y2); legend('x1', 'x2'); title('Initial Condition 2');

eigenvalues = eig(A); %extract eigenvalues to measure stability
disp('Eigenvalues of A');
disp(eigenvalues);

omega_values = [1, 10, 20, 0, 40, 50, 60, 100];
time_forced = 0:0.01:100;

figure; hold on;
for i = 1:length(omega_values)
    w = omega_values(i);
    u = [sin(w*time_forced)', zeros(length(time_forced), 1)]; 
    [y, t, x] = lsim(sys, u, time_forced, x0_1);
    subplot(4,2,i);
    plot(t, y(:,1));
    title(['Omega = ' num2str(w)]);
end

figure;
bode(sys);
grid on;

%sine of f applied on body-1
w_test = 20;
amp = 10;

t_compare = 0:0.01:10; 

u_compare = [amp*sin(w_test*t_compare)', zeros(length(t_compare), 1)];

[y_compare, t_out] = lsim(sys, u_compare, t_compare, [0;0;0;0]);

%% RUN THE .SLX FILE FIRST, OTHERWISE IT WON'T PLOT THE SCOPE OUTPUT FOR X1
figure;
plot(t_out, y_compare); hold on;
plot(out.Scope_x1.time, out.Scope_x1.signals.values, 'r--', 'LineWidth', 1.5);
title('Task A.8: MATLAB Prediction (2-DOF System)');
xlabel('Time (s)'); 
ylabel('Position (m)');
grid on;