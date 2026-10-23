%% PART B.1
syms m1 m2 m k1 k2 b1 b2 l g real
syms x1 x2 th1 th2 dx1 dx2 dth1 dth2 ddx1 ddx2 ddth1 ddth2 real

q = [x1; x2; th1; th2];
dq = [dx1; dx2; dth1; dth2];
ddq = [ddx1; ddx2; ddth1; ddth2];

p1 = [x1 + l*sin(th1); -l*cos(th1)];
p2 = [x2 + l*sin(th2); -l*cos(th2)];
dp1 = simplify(jacobian(p1, q)*dq);
dp2 = simplify(jacobian(p2, q)*dq);

T = 0.5*m1*dx1^2 + 0.5*m2*dx2^2 + 0.5*m*(dp1.'*dp1) + 0.5*m*(dp2.'*dp2);
V = 0.5*k1*x1^2 + 0.5*k2*(x2-x1)^2 - m*g*l*cos(th1) - m*g*l*cos(th2);
L = T - V;

Qnc = [-b1*dx1; -b2*dx2; 0; 0];

L_dq = simplify(jacobian(L, dq).');
L_q = simplify(jacobian(L, q).');
Mterm = simplify(jacobian(L_dq, dq));
Cterm = simplify(jacobian(L_dq, q));

ddq_solved = simplify(Mterm \ (L_q + Qnc - Cterm*dq));

dX = [dq; ddq_solved];
matlabFunction(dX, 'File', 'pendulum_ode', 'Vars', {q, dq, m1, m2, m, k1, k2, b1, b2, l, g});

%% PART B2
m1_val=2; m2_val=1; m_val=2; 
k1_val=1000; k2_val=1500; 
b1_val=0.5; b2_val=1.0; 
l_val=0.3; g_val=9.81;

X0 = [0.1; 0; 0; 0; 0; 0; 0; 0];
tspan = [0 100];

[t, X] = ode45(@(t, X) pendulum_ode(X(1:4), X(5:8), m1_val, m2_val, m_val, k1_val, k2_val, b1_val, b2_val, l_val, g_val), tspan, X0);

figure;
subplot(2,1,1);
plot(t, X(:,1), 'b', t, X(:,2), 'r');
legend('x_1', 'x_2'); title('Mass Positions'); xlabel('Time (s)'); ylabel('Position (m)');

subplot(2,1,2);
plot(t, X(:,3), 'b', t, X(:,4), 'r');
legend('\theta_1', '\theta_2'); title('Pendulum Angles'); xlabel('Time (s)'); ylabel('Angle (rad)');

% Display the equations of motion (b1)
fprintf('\n--- Mass/Inertia Matrix M ---\n');
pretty(Mterm)

fprintf('\n--- Coriolis/Centripetal Matrix C ---\n');
pretty(Cterm)

fprintf('\n--- Potential/Gravity Terms (dL/dq) ---\n');
pretty(L_q)

fprintf('\n--- Equations of Motion: M*ddq = L_q + Qnc - C*dq ---\n');
for i = 1:4
    fprintf('\nddq(%d) = \n', i);
    pretty(ddq_solved(i))
end

 disp(ddq_solved)

%% PART B3
eq_x1 = -0.55; 
eq_x2 = 0.55; 

figure('Position', [100, 100, 800, 600]); 

subplot(2,1,1);
plot(t, X(:,1), 'b-', 'LineWidth', 1.5); hold on;
plot(t, X(:,2), 'r-', 'LineWidth', 1.5);
%% IF THERE IS AN ERROR HERE, PLEASE RUN THE B3 SIMULINK SIMULATION
%  first so that matlab has the data to actually compare
plot(out.tout, out.x1 - eq_x1, 'b--', 'LineWidth', 2.5);
plot(out.tout, out.x2 - eq_x2, 'r--', 'LineWidth', 2.5);

legend('x_1 (MATLAB)', 'x_2 (MATLAB)', 'x_1 (Simscape)', 'x_2 (Simscape)');
title('Task B.3: Cart Displacements (MATLAB vs Simscape)');
xlabel('Time (s)'); ylabel('Displacement (m)');
grid on;

subplot(2,1,2);
plot(t, X(:,3), 'b-', 'LineWidth', 1.5); hold on;
plot(t, X(:,4), 'r-', 'LineWidth', 1.5);

plot(out.tout, out.th1, 'b--', 'LineWidth', 2.5);
plot(out.tout, out.th2, 'r--', 'LineWidth', 2.5);

legend('\theta_1 (MATLAB)', '\theta_2 (MATLAB)', '\theta_1 (Simscape)', '\theta_2 (Simscape)');
title('Task B.3: Pendulum Angles (MATLAB vs Simscape)');
xlabel('Time (s)'); ylabel('Angle (rad)');
grid on;