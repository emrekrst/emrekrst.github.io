function test_torque_limits
root=fileparts(fileparts(mfilename('fullpath')));
mdl='PI_D_controller';file=fullfile(root,'src','helicopter',[mdl '.slx']);
for amplitude=[.5 -.5]
 out=simulate_logged(file,2,amplitude);torque=out.release_scope_2.Data;
 fprintf('command %.2f rad: torque [%.6f %.6f] Nm\n',amplitude,min(torque),max(torque));
 assert(min(torque)>=-6-1e-9 && max(torque)<=10+1e-9,'Helicopter commanded torque exceeded -6/+10 Nm');
end
fprintf('PASS: helicopter enforces asymmetric actuator torque bounds\n');
end
