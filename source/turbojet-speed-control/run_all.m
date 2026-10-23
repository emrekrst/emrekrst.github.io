function results = run_all(artifact_dir)
% Initialise every lookup explicitly and log named signals in unsaved models.
if nargin<1,artifact_dir=fullfile(tempdir,'turbojet-release-results');end
if ~exist(artifact_dir,'dir'),mkdir(artifact_dir);end
root=fileparts(mfilename('fullpath'));
Simulink.fileGenControl('set','CacheFolder',fullfile(artifact_dir,'cache'), ...
 'CodeGenFolder',fullfile(artifact_dir,'codegen'),'createDir',true);
run(fullfile(root,'src','datas.m'));
variables={'actual_rpm_pts','delta_rpm_pts','decel_limit_fuel','accel_limit_fuel', ...
 'steady_state_fuel','throttle_pct','altitude_ft','target_rpm_map', ...
 'spool_kf','spool_k2','spool_k4','spool_n0'};
models={'override_n_feedfw_feedbk_ctrl','override_n_feedfw_feedbk_ctrl_torque_plant'};
for k=1:numel(models)
 if bdIsLoaded(models{k})
  error('release:ModelAlreadyLoaded','Close or save and close the already-open model %s before reproduction.',models{k});
 end
end
results.matlab_version=version;
for k=1:2
 mdl=models{k};load_system(fullfile(root,'src',[mdl '.slx']));
 cleanup=onCleanup(@()close_system(mdl,0));
 lines=find_system(mdl,'FindAll','on','SearchDepth',1,'Type','line');
 for j=1:numel(lines)
  name=get_param(lines(j),'Name');
  if isempty(name),continue,end
  src=get_param(lines(j),'SrcPortHandle');if src<0,continue,end
  blk=[mdl '/release_' name];
  if getSimulinkBlockHandle(blk)>0,continue,end
  add_block('simulink/Sinks/To Workspace',blk,'VariableName',name,'SaveFormat','Timeseries');
  ph=get_param(blk,'PortHandles');add_line(mdl,src,ph.Inport,'autorouting','on');
 end
 input=Simulink.SimulationInput(mdl);
 for j=1:numel(variables),input=input.setVariable(variables{j},eval(variables{j}));end
 input=input.setModelParameter('StopTime','90','MaxStep','0.01','RelTol','1e-7','ReturnWorkspaceOutputs','on');
 pid=find_system(mdl,'SearchDepth',1,'MaskType','PID 1dof');
 if isempty(pid),pid=find_system(mdl,'SearchDepth',1,'RegExp','on','Name','PI Fuel Flow Ctrl.*');end
 assert(strcmp(get_param(pid{1},'AntiWindupMode'),'clamping'),'PI anti-windup must remain enabled');
 results.(mdl).antiwindup='PI internal output clamping (not external limiter tracking)';
 for scenario=1:3
  names={'original_step','idle','full_to_idle'};name=names{scenario};
  trial=input;
  if scenario==2
   trial=trial.setBlockParameter([mdl '/Throttle'],'Before','0');
   trial=trial.setBlockParameter([mdl '/Throttle'],'After','0');
   trial=trial.setBlockParameter([mdl '/Throttle'],'Time','30');
  elseif scenario==3
   trial=trial.setBlockParameter([mdl '/Throttle'],'Before','100');
   trial=trial.setBlockParameter([mdl '/Throttle'],'After','0');
   trial=trial.setBlockParameter([mdl '/Throttle'],'Time','45');
  end
  fprintf('SIMULATING %s %s\n',mdl,name);out=sim(trial);
  rpm=out.rpm_actual;target=out.rpm_target;fuel=out.fuel_flow_target;
  results.(mdl).(name).final_rpm=rpm.Data(end);
  results.(mdl).(name).final_target=target.Data(end);
  results.(mdl).(name).final_error=abs(rpm.Data(end)-target.Data(end));
  results.(mdl).(name).fuel_min=min(fuel.Data);
  results.(mdl).(name).fuel_max=max(fuel.Data);
  actual=interp1(rpm.Time,rpm.Data,fuel.Time,'linear');
  upper=interp1(actual_rpm_pts,accel_limit_fuel,min(100,max(40,actual)));
  results.(mdl).(name).max_accel_limit_excess=max(fuel.Data-upper);
  assert(all(isfinite(rpm.Data)) && all(rpm.Data>=0),'Spool must remain finite and nonnegative');
  assert(min(fuel.Data)>=180-1e-6 && max(fuel.Data-upper)<1e-4,'Fuel schedules must bound target fuel');
  assert(results.(mdl).(name).final_error<1,'Steady-state tracking must be within 1 RPM percentage point');
  save(fullfile(artifact_dir,[mdl '_' name '.mat']),'out');
 end
 close_system(mdl,0);clear cleanup
end
fid=fopen(fullfile(artifact_dir,'results.json'),'w');fprintf(fid,'%s',jsonencode(results,PrettyPrint=true));fclose(fid);
fprintf('PASS: two plants, three scenarios each; fuel bounds, finite RPM, settled tracking, PI clamp\n');
end
