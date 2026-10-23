function test_model_ownership
% A reproduction must not close or modify someone else's open models.
root=fileparts(fileparts(mfilename('fullpath')));
sentinel='release_unsaved_sentinel';target='PI_D_controller';
assert(~bdIsLoaded(sentinel) && ~bdIsLoaded(target),'Close test model names before running ownership test');
new_system(sentinel);add_block('simulink/Sources/Constant',[sentinel '/unsaved_work']);
cleanup=onCleanup(@()close_owned({sentinel,target}));
run_all(false);
success_preserved=bdIsLoaded(sentinel);
% New unsaved model with the same name must be refused before it is touched.
new_system(target);add_block('simulink/Sources/Constant',[target '/unsaved_target']);
try
 simulate_logged(fullfile(root,'src','helicopter',[target '.slx']),0);
 refused=false;
catch err
 refused=strcmp(err.identifier,'release:ModelAlreadyLoaded');
end
target_preserved=bdIsLoaded(target) && getSimulinkBlockHandle([target '/unsaved_target'])>0;
% Missing-file failure must also leave the unrelated model open.
try,simulate_logged(fullfile(root,'not_a_model.slx'));catch,end
error_preserved=bdIsLoaded(sentinel);
assert(success_preserved && error_preserved,'Runner closed an unrelated unsaved model');
assert(refused && target_preserved,'Runner must refuse a preloaded target without changing or closing it');
fprintf('PASS: unrelated models survive success/error, preloaded targets are refused intact\n');
end
function close_owned(names)
for k=1:numel(names),if bdIsLoaded(names{k}),close_system(names{k},0);end,end
end
