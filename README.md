# Emre Karausta — engineering portfolio

Static portfolio website for [emrekrst.github.io](https://emrekrst.github.io/).

## Local preview

From this repository root:

```sh
python3 -m http.server 8771 --bind 127.0.0.1
```

Open `http://127.0.0.1:8771/`. No installation or build step is required. Use HTTP rather than opening `index.html` as a file, because the demos use JavaScript modules and load trained weights.

## Contents

- Fifteen project studies across ten source packages.
- Interactive FADEC, rover-command, Sudoku and neural-decoder demonstrations.
- CSRT demonstration video, muted and looping while visible.
- Recorded engineering plots with full-size views.
- Readable project source pages and downloadable source ZIPs.

`index.html`, `style.css`, `app.mjs` and `projects.mjs` define the portfolio. `lab-core.mjs` contains the browser companion algorithms. `assets/`, `demos/`, `source/` and `downloads/` are runtime content. The empty `.nojekyll` file enables direct static serving by GitHub Pages.

## Checks

With Node.js 22 or newer:

```sh
node --test tests/*.test.mjs
```

The demos distinguish browser companions, recorded evidence and native applications. Each source package includes its own build instructions, verification and attribution. Existing licences remain applicable; this website does not grant additional rights to supplied coursework, shared work or third-party material.

## GitHub Pages

For deployment from this branch, choose **Settings → Pages → Deploy from a branch → main → /(root)**. Review the changes before committing and pushing. If `main` is already the publishing source, pushing to it can update the live site automatically.

This website contains project source copies and ZIP downloads. Publishing the website also publishes those copies, independently of the standalone project repositories. The browser JavaScript and demonstration video are served publicly too.

The GitHub profile README belongs in the separate `emrekrst/emrekrst` repository; it is not this file.
