# Production

Source and export files for assets past `APPROVED`.

```text
visual/production/<tracker-id>/
  src/     authoring files, .blend
  export/  engine-ready .glb or .fbx
  tex/     textures
```

Naming, pivots, scale and animation conventions are in
`visual/docs/TECHNICAL_STANDARD.md`. Binaries are covered by the Git LFS rules
in `visual/docs/PIPELINE.md`; run `git lfs install` once per clone before
committing the first one.
