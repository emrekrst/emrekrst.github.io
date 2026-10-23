% X-Breakpoints (Actual RPM %)
actual_rpm_pts = [40 50 60 70 80 90 100];

% X-Breakpoints (Delta RPM %)
delta_rpm_pts = [0 10 20 30 40 50 60];

% Y-Data (Min Fuel Flow allowed)
decel_limit_fuel = [180 180 180 180 180 180 180]; 

% Y-Data (Max Fuel Flow allowed)
accel_limit_fuel = [500 1200 1800 2200 2500 2600 2700];

% Y-Data (Fuel Flow in pph or kg/hr required to hold that RPM)
steady_state_fuel = [200 300 450 650 900 1300 1800];


% Breakpoints
throttle_pct = [0 10 20 30 40 50 60 70 80 90 100];  % Throttle Lever Angle (%)
altitude_ft  = [0 10000 20000 30000 40000];          % Altitude (ft)

% Data matrix (Rows = Altitude, Columns = Throttle)
% Idle is ~40% RPM, Max is 100% RPM. At higher altitudes, target is often slightly higher.
target_rpm_map = ...
   [40, 55, 65, 75, 80, 85, 90, 92, 95, 98, 100;   % 0ft
    42, 57, 67, 77, 82, 86, 91, 93, 96, 99, 100;   % 10kft
    44, 59, 69, 79, 84, 87, 92, 94, 97, 99, 100;   % 20kft
    45, 60, 70, 80, 85, 88, 92, 94, 97, 99, 100;   % 30kft
    46, 61, 71, 81, 86, 89, 93, 95, 98, 99, 100];  % 40kft
% Independent nonlinear spool plant: n = RPM/100
% dn/dt = spool_kf*Wf - spool_k2*n^2 - spool_k4*n^4
spool_kf = 1.3324450366422388e-4;
spool_k2 = 0.15259668157736112;
spool_k4 = 0.08724342501824187;
spool_n0 = 0.40;