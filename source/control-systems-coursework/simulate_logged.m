function out = simulate_logged(model_file, stop_time, step_amplitude)
% Add logging only in memory. Never save over the supplied model.
[~,mdl]=fileparts(model_file);
if bdIsLoaded(mdl)
 error('release:ModelAlreadyLoaded','Close or save and close the already-open model %s before reproduction.',mdl);
end
load_system(model_file);
cleanup=onCleanup(@()close_system(mdl,0));
if nargin>2,set_param([mdl '/Step'],'After',num2str(step_amplitude));end
try,set_param(mdl,'SimMechanicsOpenEditorOnUpdate','off');catch,end
scopes=find_system(mdl,'SearchDepth',1,'BlockType','Scope');
for i=1:numel(scopes)
 ports=get_param(scopes{i},'PortHandles');
 for j=1:numel(ports.Inport)
  line=get_param(ports.Inport(j),'Line');if line<0,continue,end
  src=get_param(line,'SrcPortHandle');name=sprintf('release_scope_%d',i);
  if numel(ports.Inport)>1,name=sprintf('%s_%d',name,j);end
  block=[mdl '/' name];add_block('simulink/Sinks/To Workspace',block,'VariableName',name,'SaveFormat','Timeseries');
  ph=get_param(block,'PortHandles');add_line(mdl,src,ph.Inport,'autorouting','on');
 end
end
existing=find_system(mdl,'BlockType','ToWorkspace');
for k=1:numel(existing),set_param(existing{k},'SaveFormat','Timeseries','MaxDataPoints','inf');end
set_param(mdl,'LimitDataPoints','off');
set_param(mdl,'ReturnWorkspaceOutputs','on','MaxStep','0.002','RelTol','1e-7');
if nargin>1,set_param(mdl,'StopTime',num2str(stop_time));end
out=sim(mdl);
end
