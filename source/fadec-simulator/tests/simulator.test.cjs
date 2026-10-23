const {test, before, after} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {pathToFileURL} = require('node:url');
const {chromium} = require('playwright');

let browser;
const root = path.resolve(__dirname, '..');
const evidence = process.env.EVIDENCE_DIR;
const results = {};
before(async () => {
  const chrome = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome';
  const executablePath = process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE ||
    (process.platform === 'darwin' && fs.existsSync(chrome) ? chrome : undefined);
  browser = await chromium.launch({headless: true, executablePath});
  if (evidence) fs.mkdirSync(evidence, {recursive: true});
});
after(async () => {
  if (evidence) fs.writeFileSync(path.join(evidence, 'scenarios.json'), JSON.stringify(results, null, 2) + '\n');
  if (browser) await browser.close();
});

async function pageFor(run) {
  const page = await browser.newPage({viewport: {width: 1600, height: 1100}, locale: 'en-GB'});
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  // Advance the real application's deterministic model without wall-clock waits.
  await page.addInitScript(() => {
    window.simulatorIntervals = [];
    window.setInterval = (callback, interval) => { window.simulatorIntervals.push(interval); return 0; };
  });
  try {
    await page.goto(pathToFileURL(path.join(root, 'index.html')).href);
    await run(page);
    assert.deepEqual(errors, [], 'no uncaught application errors');
  } finally { await page.close(); }
}

async function start(page, fault) {
  await page.evaluate(fault => {
    closeBrief();
    if (fault) BK.inj['_' + fault] = true;
    document.getElementById('startBtn').click();
    for (let i = 0; i < 600; i++) realStep(0.05);
    render();
  }, fault);
}

function state() {
  return {
    phase: BK.START.phase, n2: FADECState.n2, egt: FADECState.egt,
    degrade: FADECState.degrade, leader: FADECState.leader,
    voter: FADECState.voter, n2Synth: FADECState.obm.n2Synth,
    n2Out: FADECState.obm.n2Out, obmCrashed: FADECState.obm.crashed,
    openLoop: FADECState.openLoop, overspeed: FADECState.overspeed,
    command: BK.plant.WfCmd_applied, ccdl: FADECState.ccdl,
    iso: BK.fadec.iso, pfr: FADECState.pfr.length
  };
}

test('displayed update frequency agrees with the actual model timer', async () => pageFor(async page => {
  const {intervals, meta} = await page.evaluate(() => ({intervals: window.simulatorIntervals,
    meta: document.querySelector('[data-i18n="brf.meta"]').textContent}));
  assert.equal(intervals.length, 1);
  assert.ok(meta.includes(`${1000 / intervals[0]} Hz`), `${meta} disagrees with ${intervals[0]} ms`);
}));

test('fault-word fields round trip over their entire 16-bit range', async () => pageFor(async page => {
  assert.equal(await page.evaluate(() => {
    for (let word = 0; word < 65536; word++) {
      const d = FW.decode(word);
      if (FW.encode(d.alan, d.kod, d.ch, d.voter, d.comp) !== word) return false;
    }
    return true;
  }), true);
}));

test('start, idle, throttle response and shutdown use the actual UI controls', async () => pageFor(async page => {
  await start(page);
  const idle = await page.evaluate(state);
  assert.equal(idle.phase, 'RUN');
  assert.ok(Math.abs(idle.n2 - 60) < 1);
  assert.equal(idle.degrade, 3);
  await page.evaluate(() => {
    const throttle = document.querySelector('input[type="range"]');
    throttle.value = 75; throttle.dispatchEvent(new Event('input', {bubbles: true}));
    for (let i = 0; i < 600; i++) realStep(.05);
  });
  const high = await page.evaluate(state);
  assert.ok(high.n2 > idle.n2 + 20 && high.n2 < 100);
  await page.evaluate(() => {
    document.getElementById('startBtn').click();
    for (let i = 0; i < 600; i++) realStep(.05);
    render();
  });
  const stopped = await page.evaluate(state);
  assert.equal(stopped.phase, 'OFF');
  assert.ok(stopped.n2 < 1);
  results.startThrottleStop = {idle, high, stopped};
}));

const scenarios = {
  nominal: s => { assert.equal(s.degrade, 3); assert.equal(s.voter, 0); assert.equal(s.pfr, 0); },
  halfblind: s => { assert.equal(s.degrade, 2); assert.equal(s.iso.C, true); },
  twoIso: s => { assert.equal(s.degrade, 1); assert.equal(s.iso.A, true); assert.equal(s.iso.B, true); },
  leader: s => { assert.equal(s.leader, 'B'); assert.equal(s.iso.A, true); },
  totalLoss: s => { assert.equal(s.degrade, 0); assert.equal(s.n2Synth, true); assert.equal(s.openLoop, false); },
  openLoop: s => { assert.equal(s.degrade, 0); assert.equal(s.openLoop, true); },
  single: s => { assert.equal(s.degrade, 1); assert.equal(s.leader, 'C'); },
  commonMode: s => { assert.equal(s.degrade, 3); assert.equal(s.voter, 0); assert.equal(s.n2Out, true); },
  obmCrash: s => { assert.equal(s.obmCrashed, true); assert.equal(s.degrade, 3); },
  ccdl: s => { assert.equal(s.ccdl.AB, false); assert.equal(s.ccdl.AC, true); }
};
for (const [name, check] of Object.entries(scenarios)) {
  test(`scenario: ${name}`, async () => pageFor(async page => {
    await start(page);
    await page.evaluate(name => {
      const index = scenList.findIndex(s => s[0] === name);
      document.querySelectorAll('#scenGrid button')[index].click();
      for (let i = 0; i < 150; i++) realStep(.05);
      render();
    }, name);
    const actual = await page.evaluate(state);
    check(actual);
    if (name !== 'nominal') assert.ok(actual.pfr > 0, 'fault is latched in maintenance log');
    if (name === 'halfblind') assert.doesNotMatch(await page.locator('.eqp-stat').textContent(), /SYSTEM NOMINAL/,
      'a static footer must not assert normal system health while a channel is isolated');
    results[name] = actual;
    if (evidence && ['nominal', 'halfblind', 'commonMode'].includes(name)) {
      await page.screenshot({path: path.join(evidence, `${name}.png`), fullPage: true});
    }
    await page.evaluate(() => {
      document.getElementById('fiReset').click();
      for (let i = 0; i < 200; i++) realStep(.05);
      render();
    });
    const reset = await page.evaluate(state);
    assert.equal(reset.degrade, 3);
    assert.equal(reset.openLoop, false);
    assert.equal(reset.obmCrashed, false);
  }));
}

for (const fault of ['hot', 'hung']) {
  test(`${fault} start abort cuts commanded fuel and reset permits restart`, async () => pageFor(async page => {
    await start(page, fault);
    const abort = await page.evaluate(state);
    assert.equal(abort.phase, fault.toUpperCase());
    assert.equal(abort.command, 0);
    await page.evaluate(() => { resetInj(); });
    await start(page);
    assert.equal((await page.evaluate(state)).phase, 'RUN');
    results[fault] = abort;
  }));
}

test('overspeed latches and reduces commanded fuel until cleared', async () => pageFor(async page => {
  await start(page);
  await page.evaluate(() => {
    document.getElementById('fiOvsp').click();
    for (let i = 0; i < 150; i++) realStep(.05);
  });
  const trip = await page.evaluate(state);
  assert.equal(trip.overspeed, true);
  assert.ok(trip.command <= 120);
  await page.evaluate(() => { delete BK.inj._ovsp; for (let i = 0; i < 200; i++) realStep(.05); });
  assert.equal((await page.evaluate(state)).overspeed, true);
  await page.evaluate(() => { resetInj(); realStep(.05); });
  assert.equal((await page.evaluate(state)).overspeed, false);
  results.overspeed = trip;
}));

test('English/Turkish switch and technical help remain usable', async () => pageFor(async page => {
  await page.locator('#brfSkip').click();
  await page.locator('#langTr').click();
  assert.match(await page.locator('#modeManual').textContent(), /MANUEL/);
  await page.locator('#langEn').click();
  assert.match(await page.locator('#modeManual').textContent(), /MANUAL/);
  await page.locator('#ietmBtn').click();
  await page.evaluate(() => { ietmOpen('cycle', 'Cycle'); });
  await page.locator('[data-ietm-more]').click();
  assert.match(await page.locator('.ietm-why').textContent(), /20 Hz/);
  await page.locator('[data-ietm-close]').click();
  assert.equal(await page.locator('#ietmCard').evaluate(el => el.classList.contains('show')), false);
}));

test('identical input sequences reproduce identical model states', async () => {
  const samples = [];
  for (let i = 0; i < 2; i++) await pageFor(async page => {
    await start(page);
    await page.evaluate(() => { SCEN.commonMode(); for (let i = 0; i < 100; i++) realStep(.05); });
    samples.push(await page.evaluate(state));
  });
  assert.deepEqual(samples[0], samples[1]);
});

test('the unmodified browser timer advances the application after the briefing start button', async () => {
  const page = await browser.newPage();
  const errors = [];
  page.on('pageerror', e => errors.push(e.message));
  try {
    await page.goto(pathToFileURL(path.join(root, 'index.html')).href);
    await page.locator('#brfStart').click();
    await page.waitForFunction(() => FADECState.engineOn && FADECState.cycle >= 4, null, {timeout: 5000});
    assert.deepEqual(errors, []);
    assert.notEqual((await page.evaluate(state)).phase, 'OFF');
  } finally { await page.close(); }
});
