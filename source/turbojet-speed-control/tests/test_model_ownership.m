function test_model_ownership
mdl='override_n_feedfw_feedbk_ctrl';other='turbo_unsaved_sentinel';
assert(~bdIsLoaded(mdl) && ~bdIsLoaded(other),'Close test model names first');
new_system(mdl);add_block('simulink/Sources/Constant',[mdl '/unsaved_work']);
new_system(other);add_block('simulink/Sources/Constant',[other '/unsaved_work']);
cleanup=onCleanup(@()close_owned({mdl,other}));
try,run_all;refused=false;catch err,refused=strcmp(err.identifier,'release:ModelAlreadyLoaded');end
assert(refused && bdIsLoaded(mdl) && getSimulinkBlockHandle([mdl '/unsaved_work'])>0, ...
 'Refusing a preloaded target must preserve its unsaved changes');
assert(bdIsLoaded(other),'Unrelated unsaved model must stay open');
fprintf('PASS: turbojet refuses preloaded targets and preserves unrelated models\n');
end
function close_owned(names)
for k=1:numel(names),if bdIsLoaded(names{k}),close_system(names{k},0);end,end
end
