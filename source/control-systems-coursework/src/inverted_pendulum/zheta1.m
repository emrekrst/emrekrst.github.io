%% 2.2
%
g = 9.81;
L = 0.20;
%
omega_sq = g / L;
%
num = [-omega_sq];
den = [1, 0, -omega_sq];
pendulum = tf(num, den);
%
step(pendulum);
title('Step Response of the Inverted Pendulum');

%%3.2 
%
for k = [0, 1, 2, 10]
    sysclp1 = feedback(pendulum, k);
    figure;
    step(sysclp1);
    title(['Negative Feedback (k = ', num2str(k), ')']);
end
%
for k = [0, 1, 2, 10]
    sysclp2 = feedback(pendulum, k, +1);
    figure;
    step(sysclp2);
    title(['Positive Feedback (k = ', num2str(k), ')']);
end
%
figure; rlocus(pendulum); title('Root Locus - Negative Feedback');
figure; rlocus(-pendulum); title('Root Locus - Positive Feedback');
%
%
%
c = 10;
tau = 1 /(c*sqrt(omega_sq));
compensator = tf([c*tau, 1], [tau, 1]);

sys2 = series(-compensator, pendulum);
figure;
rlocus(sys2);
title('Q10, Root Locus w/ Exact Cancellation Compensator');
%
compensator2 = tf([0.1, 1], [0.01, 1]);
sys3 = series(-compensator2, pendulum); % [cite: 194]
figure;
rlocus(sys3);
title('Q11: Root Locus with Hardware Lead Compensator H1(s)');
%
%
servo = tf([1], [0.00025, 0.02, 1]);
actual = series(servo, pendulum);
sys4 = series(-compensator2, actual);

figure;
rlocus(sys4);
title('Q13 Root Locus with Servo Dynamics');
%
k_stable = 1.2; 
sysclp3 = feedback(sys4, k_stable);
figure;
bode(sysclp3);
title('Q14: Bode Plot of Closed-Loop Sys');
figure;
margin(sysclp3);
title('Q14 gain n phase margins');
%
%
%
%
%

%

%

