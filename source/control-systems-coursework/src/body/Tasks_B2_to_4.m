m1 = 6.15;
m2 = 6.0;
m3 = 12.58;
m4 = 50.34;
ms = 0.3;

k1 = 6e3;
k2 = 6e3;
k3 = 10e3;
k4 = 10e3;
k5 = 18e3; 
ks = 403e3;

b1 = 0.3e3;
b2 = 0.65e3;
b4 = 1.9e3; 
bs = 2170e3; 

kg_soft = 99e3;
kg_hard = 359e3;
kg_values = [kg_soft, kg_hard];
soil_names = {'Soft Soil (99 kN/m)', 'Hard Soil (359 kN/m)'};

M = diag([ms, m1, m2, m3, m4]);

B = [bs,        -bs,        0,   0,         0;
    -bs,  bs + b1 + b2,   -b2, -b1,         0;
      0,        -b2,       b2,   0,         0;
      0,        -b1,        0, b1 + b4,    -b4;
      0,          0,        0,  -b4,        b4];

freq_Hz = logspace(log10(0.01), log10(1000), 1000);
freq_rads = 2 * pi * freq_Hz;

for i = 1:length(kg_values)
    kg = kg_values(i);
    
    K = [ks + kg,      -ks,              0,              0,            0;
         -ks,    ks + k1 + k2,         -k2,            -k1,            0;
           0,          -k2,        k2 + k3,            -k3,            0;
           0,          -k1,            -k3,     k1+k3+k4+k5,  -(k4 + k5);
           0,            0,              0,      -(k4 + k5),    k4 + k5];
         
    F = [kg; 0; 0; 0; 0];
    
    A_sys = [zeros(5,5), eye(5,5);
            -inv(M)*K,  -inv(M)*B];
    B_sys = [zeros(5,1);
             inv(M)*F];
    C_sys = [eye(5,5), zeros(5,5)];
    D_sys = zeros(5,1);
    
    sys = ss(A_sys, B_sys, C_sys, D_sys);
    figure('Name', ['Frequency Response: ', soil_names{i}]);
    opts = bodeoptions('cstprefs');
    opts.FreqUnits = 'Hz';
    opts.MagUnits = 'dB';
    bode(sys, freq_rads, opts);
    grid on;
    title(['Bode Plot: Human Body Model on ', soil_names{i}]);
end

disp('B.3: Undamped Natural Frequencies (Hz)');

for i = 1:length(kg_values)
    kg = kg_values(i);
    
    K = [ks + kg,      -ks,              0,              0,            0;
         -ks,    ks + k1 + k2,         -k2,            -k1,            0;
           0,          -k2,        k2 + k3,            -k3,            0;
           0,          -k1,            -k3, k1+k3+k4+k5,  -(k4 + k5);
           0,            0,              0,     -(k4 + k5),    k4 + k5];
           
    [~, D] = eig(K, M); 
    omega_n_rads = sqrt(diag(D));
    omega_n_Hz = omega_n_rads / (2 * pi);
    omega_n_Hz = sort(omega_n_Hz);
    disp(['Natural Frequencies for ', soil_names{i}, ':']);
    disp(omega_n_Hz);
end

% Task B.4
disp('B.4: Largest Two Eigenfrequencies (Hz)');

for i = 1:length(kg_values)
    kg = kg_values(i);
    
    K = [ks + kg,      -ks,              0,              0,            0;
         -ks,    ks + k1 + k2,         -k2,            -k1,            0;
           0,          -k2,        k2 + k3,            -k3,            0;
           0,          -k1,            -k3, k1+k3+k4+k5,  -(k4 + k5);
           0,            0,              0,     -(k4 + k5),    k4 + k5];
           
    A_sys = [zeros(5,5), eye(5,5);
            -inv(M)*K,  -inv(M)*B];
            
    eigenvalues_A = eig(A_sys);
    eigenfreq_rads = abs(imag(eigenvalues_A));
    eigenfreq_Hz = eigenfreq_rads / (2 * pi);
    unique_eigen_Hz = unique(round(eigenfreq_Hz, 4));
    largest_two_Hz = maxk(unique_eigen_Hz, 2);
    
    disp(['Largest two eigenfrequencies for ', soil_names{i}, ':']);
    disp(largest_two_Hz);
end