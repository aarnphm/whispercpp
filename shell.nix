{ sources ? import ./nix/sources.nix, pkgs ? import sources.nixpkgs {
  overlays = [ ];
  config = { };
} }:

with pkgs;
let
  lib = import <nixpkgs/lib>;
  inherit (lib) optional optionals;

  basePackages = with pkgs; [
      (python310.withPackages (ps: with ps; [ pynvim pip virtualenv ipython ]))
      
    git
      bazel_6
      nixfmt
      treefmt
    which

    # Without this, we see a whole bunch of warnings about LANG, LC_ALL and locales in general.
    # The solution is from: https://github.com/NixOS/nix/issues/318#issuecomment-52986702
    glibcLocales
    coreutils
  ];

  env = buildEnv {
    name = "whispercpp-environment";
    paths = basePackages ++ lib.optional stdenv.isLinux inotify-tools
      ++ lib.optionals stdenv.isDarwin
      (with darwin.apple_sdk.frameworks; [ CoreFoundation CoreServices ]);
  };

in stdenv.mkDerivation rec {
  name = "python-environment";

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

    pip install -r ./requirements/bazel-pypi.lock.txt
  '';

  LOCALE_ARCHIVE =
    if stdenv.isLinux then "${glibcLocales}/lib/locale/locale-archive" else "";

  nobuild = ''
    echo Do not run this derivation with nix-build, it can only be used with nix-shell
  '';
}
