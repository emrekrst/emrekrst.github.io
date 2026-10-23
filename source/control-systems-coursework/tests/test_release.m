function test_release
r = run_all(false);
assert(max(abs(r.cart_resonance_rads-[17.5 49.4]))<0.2);
assert(max(abs(r.body_damped_hz(1,:)-[14.20 1.73]))<0.01);
assert(max(abs(r.body_damped_hz(2,:)-[32.30 1.76]))<0.01);
assert(r.beam_rayleigh_relative_error<1e-10);
assert(abs(r.midspan_mass_ratio-1.5)<1e-12);
assert(abs(r.helicopter_margin(1,1)-65.5)<0.2);
assert(abs(r.helicopter_margin(2,1)+13)<0.2);
assert(r.nonlinear_energy_increase_max<1e-5);
assert(abs(r.inverted_pendulum_poles(2)-7)<0.02);
fprintf('PASS: controls analytical and nonlinear regression checks\n');
end
