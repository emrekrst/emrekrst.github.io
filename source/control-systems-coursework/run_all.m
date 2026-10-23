function r = run_all(include_simulink, artifact_dir)
% Reproduce analytical results without manual base-workspace dependencies.
% Original coursework source and models remain in src/ with original equations.
if nargin<1, include_simulink=true; end
if nargin<2, artifact_dir=fullfile(tempdir,'controls-release-results'); end
if ~exist(artifact_dir,'dir'), mkdir(artifact_dir); end
root=fileparts(mfilename('fullpath'));
addpath(genpath(fullfile(root,'src')));
Simulink.fileGenControl('set','CacheFolder',fullfile(artifact_dir,'cache'), ...
 'CodeGenFolder',fullfile(artifact_dir,'codegen'),'createDir',true);
r.matlab_version=version;
% Mechanical state [x1 x2 v1 v2], independently equivalent to original force states.
M=diag([2 1]); K=[2500 -1500;-1500 1500]; B=diag([0.5 1]);
A=[zeros(2) eye(2);-M\K -M\B];
sys=ss(A,[zeros(2);M\eye(2)],[eye(2) zeros(2)],zeros(2));
w=linspace(1,70,100000); mag=squeeze(abs(freqresp(sys(1,1),w)));
[~,loc]=findpeaks(mag); r.cart_resonance_rads=w(loc);
r.cart_undamped_rads=sort(sqrt(eig(K,M)))';
assert(all(real(eig(A))<0),'Cart model must dissipate energy');
% Five DOF matrices retained from Tasks_B2_to_4, with linear solves replacing inv.
M=diag([.3 6.15 6 12.58 50.34]); bs=2170e3; b1=300;b2=650;b4=1900;
B=[bs -bs 0 0 0;-bs bs+b1+b2 -b2 -b1 0;0 -b2 b2 0 0;0 -b1 0 b1+b4 -b4;0 0 0 -b4 b4];
for i=1:2
 kg=[99e3 359e3];kg=kg(i);ks=403e3;k1=6000;k2=6000;k3=10000;k4=10000;k5=18000;
 K=[ks+kg -ks 0 0 0;-ks ks+k1+k2 -k2 -k1 0;0 -k2 k2+k3 -k3 0;0 -k1 -k3 k1+k3+k4+k5 -(k4+k5);0 0 0 -(k4+k5) k4+k5];
 A=[zeros(5) eye(5);-M\K -M\B];
 r.body_undamped_hz(i,:)=sort(sqrt(eig(K,M)))'/(2*pi);
 f=unique(round(abs(imag(eig(A)))/(2*pi),8));f=f(f>1e-6);
 r.body_damped_hz(i,:)=maxk(f,2)';
end
E=210e9;rho=7800;L=6;h=.1;d=.05;I=d*h^3/12;mu=rho*d*h;
r.beam_frequencies_hz=((1:3)*pi).^2*sqrt(E*I/(mu*L^4))/(2*pi);
Y=@(x)sin(pi*x/L);Ypp=@(x)-(pi/L)^2*sin(pi*x/L);
wr=sqrt(integral(@(x)E*I*Ypp(x).^2,0,L)/integral(@(x)mu*Y(x).^2,0,L));
r.beam_rayleigh_relative_error=abs(wr/(2*pi)-r.beam_frequencies_hz(1))/r.beam_frequencies_hz(1);
% Rayleigh modal mass is half beam mass; reducing omega by half needs 3 modal masses.
r.midspan_mass_ratio=(1/.5^2-1)/2;
r.beam_mass_kg=mu*L;r.midspan_mass_kg=r.midspan_mass_ratio*r.beam_mass_kg;
r.cantilever_reference_hz=1.875104068711961^2*sqrt(E*I/(mu*L^4))/(2*pi);
J=.073824;kp=40;kd=2.42979296119;s=tf('s');Loop=(kd*s+kp)/(J*s^2);
[gm,pm,~,wc]=margin(Loop);r.helicopter_margin(1,:)=[pm wc gm];
[gm,pm,~,wc]=margin(Loop/(.1*s+1));r.helicopter_margin(2,:)=[pm wc gm];
% Derivative on feedback gives numerator Kp, unlike feedback(Loop,1).
closed=kp/(J*s^2+kd*s+kp);info=stepinfo(.25*closed);
r.helicopter_ideal_settling_s=info.SettlingTime;r.helicopter_ideal_overshoot_pct=info.Overshoot;
r.inverted_pendulum_poles=sort(pole(tf(-9.81/.2,[1 0 -9.81/.2])))';
X0=[.1;zeros(7,1)]; t=(0:.01:100)';
[t,X]=ode45(@(~,X)pendulum_ode(X(1:4),X(5:8),2,1,2,1000,1500,.5,1,.3,9.81),t,X0,odeset('RelTol',1e-9,'AbsTol',1e-11));
energy=.5*2*X(:,5).^2+.5*X(:,6).^2;
for j=1:2
 v=X(:,4+j);th=X(:,2+j);dth=X(:,6+j);
 energy=energy+.5*2*((v+.3*cos(th).*dth).^2+(.3*sin(th).*dth).^2)+2*9.81*.3*(1-cos(th));
end
energy=energy+.5*1000*X(:,1).^2+.5*1500*(X(:,2)-X(:,1)).^2;
r.nonlinear_energy_increase_max=max(diff(energy));r.nonlinear_duration_s=t(end);
assert(all(isfinite(X(:))) && r.nonlinear_energy_increase_max<1e-5,'Nonlinear passivity regression');
if include_simulink
 models={'two_cart/Task_A8_was_A7','beam/beam_A6_1','beam/beam_A6_2','beam/beam_A7','nonlinear/Task_B3','helicopter/PI_D_controller'};
 for j=1:numel(models)
  [~,name]=fileparts(models{j});fprintf('SIMULATING %s\n',name);
  simout=simulate_logged(fullfile(root,'src',[models{j} '.slx']));
  save(fullfile(artifact_dir,[name '.mat']),'simout');
  r.simulink.(name).stop_time=simout.tout(end);
  if startsWith(name,'beam')
   z=simout.release_scope_1; tt=z.Time; yy=z.Data(:,1);use=tt>5.1;
   [~,pk]=findpeaks(yy(use));tz=tt(use);yz=yy(use);
   peak_time=tz(pk);
   for v=1:numel(pk)
    ids=pk(v)+(-1:1);poly=polyfit(tz(ids)-tz(pk(v)),yz(ids),2);
    peak_time(v)=tz(pk(v))-poly(2)/(2*poly(1));
   end
   if numel(pk)>2
    frequency=1/median(diff(peak_time));r.simulink.(name).free_frequency_hz=frequency;
    if strcmp(name,'beam_A6_1'),reference=r.cantilever_reference_hz;
    elseif strcmp(name,'beam_A6_2'),reference=r.beam_frequencies_hz(1);
    else,reference=r.beam_frequencies_hz(1)/2;end
    r.simulink.(name).relative_error_pct=100*abs(frequency/reference-1);
   end
  elseif strcmp(name,'Task_B3')
   fields={'x1','x2','th1','th2'};offsets=[-.55 .55 0 0];
   for k=1:4
    z=simout.get(fields{k}); if isa(z,'timeseries'),zz=z.Data;tt=z.Time;else,zz=z;tt=simout.tout;end
    ref=interp1(t,X(:,k),tt,'pchip');
    r.simulink.(name).max_abs_difference(k)=max(abs(squeeze(zz)-offsets(k)-ref));
   end
  elseif strcmp(name,'PI_D_controller')
   z=simout.release_scope_1;inf=stepinfo(z.Data,z.Time,.25);
   r.simulink.(name).settling_s=inf.SettlingTime;r.simulink.(name).overshoot_pct=inf.Overshoot;
  end
 end
end
fid=fopen(fullfile(artifact_dir,'results.json'),'w');fprintf(fid,'%s',jsonencode(r,PrettyPrint=true));fclose(fid);
disp(r);
end
