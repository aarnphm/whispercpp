{ sources ? import ./nix/sources.nix, pkgs ? import sources.nixpkgs {
  overlays = [ ];
  config = { };
} }:

with pkgs;
let
  lib = import <nixpkgs/lib>;
  inherit (lib) optional optionals;

  mach-nix = import (pkgs.fetchFromGitHub {
    owner = "DavHau";
    repo = "mach-nix";
    rev = "70daee1b200c9a24a0f742f605edadacdcb5c998";
    sha256 = "0krc4yhnpbzc4yhja9frnmym2vqm5zyacjnqb3fq9z9gav8vs9ls";
  }) { inherit pkgs; };

  pyEnv = mach-nix.mkPython {
    requirements = builtins.readFile ./requirements/pypi.txt;
  };

  basePackages = with pkgs; [
    pyEnv
    git
    bazel
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

  LOCALE_ARCHIVE =
    if stdenv.isLinux then "${glibcLocales}/lib/locale/locale-archive" else "";

  nobuild = ''
    echo Do not run this derivation with nix-build, it can only be used with nix-shell
  '';
}
