{ sources ? import ./nix/sources.nix, pkgs ? import sources.nixpkgs {
  overlays = [
    (self: super: {
      # https://github.com/nmattia/niv/issues/332#issuecomment-958449218
      # TODO: remove this when upstream is fixed.
      niv = self.haskell.lib.compose.overrideCabal
        (drv: { enableSeparateBinOutput = false; }) super.haskellPackages.niv;
      python-overlay = super.buildEnv {
        name = "python-overlay";
        paths = [
          # A Python 3 interpreter with some packages
          (self.python310.withPackages
            (ps: with ps; [ pynvim pip virtualenv pipx ipython ]))
        ];
      };
    })
  ];
  config = { };
} }:

with pkgs;
let
  lib = import <nixpkgs/lib>;
  inherit (lib) optional optionals;

  basePackages = with pkgs; [
    # NOTE: keep this in sync with the version in .bazelversion
    bazel_6
    llvmPackages_15.llvm

    git
    niv
    nixfmt
    treefmt
    which
    python-overlay

    # Without this, we see a whole bunch of warnings about LANG, LC_ALL and locales in general.
    # The solution is from: https://github.com/NixOS/nix/issues/318#issuecomment-52986702
    glibcLocales
    coreutils

    ffmpeg
  ];

  env = buildEnv {
    name = "whispercpp-environment";
    paths = basePackages ++ lib.optional stdenv.isLinux inotify-tools
      ++ lib.optionals stdenv.isDarwin
      (with darwin.apple_sdk.frameworks; [ CoreFoundation CoreServices ]);
  };

in stdenv.mkDerivation rec {
  name = "dev-environment";

  phases = [ "nobuild" ];

  buildInputs = [ env ];

  enableParallelBuilding = true;

  shellHook = ''
    if [[ ! -d venv ]]; then
      python -m virtualenv venv --download
      source venv/bin/activate
    else
      source venv/bin/activate
    fi

    bazel run //requirements:pypi.update
    pip install -r ./requirements/bazel-pypi.lock.txt
  '';

  LOCALE_ARCHIVE =
    if stdenv.isLinux then "${glibcLocales}/lib/locale/locale-archive" else "";

  # NOTE: using bazel setup by nixpkgs, and hence disable the wrapper
  DISABLE_BAZEL_WRAPPER = 1;

  nobuild = ''
    echo Do not run this derivation with nix-build, it can only be used with nix-shell
  '';
}
