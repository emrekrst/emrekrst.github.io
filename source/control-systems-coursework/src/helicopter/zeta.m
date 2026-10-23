clear; clc; close all;

J = 0.073824;
Kp = 40;  
Kd = 2.42979296119; 

num_pd = [Kd Kp];
den_plant = [J 0 0];
L_ideal = tf(num_pd, den_plant);

figure(1);
margin(L_ideal);
title('Part 2.7: Ideal System Bode Plot (PD Ctrl only)');
grid on;

[Gm_ideal, Pm_ideal, Wcg_ideal, Wcp_ideal] = margin(L_ideal);

num_motor = [1];
den_motor = [0.1 1];
Gm = tf(num_motor, den_motor);

L_real = L_ideal * Gm;

figure(2);
margin(L_real);
title('Part 2.8: Real System Bode Plot w/ Motor Dynamics');
grid on;

[Gm_real, Pm_real, Wcg_real, Wcp_real] = margin(L_real);