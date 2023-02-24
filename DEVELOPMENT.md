### Requirements

Install [nix](https://nixos.org/download.html#nix-install-linux) to setup adhoc development environment.

### Python

Compile Python requirements:

```bash
bazel run //:vendor-requirements
```

Start new shell:

```bash
nix-shell
```
