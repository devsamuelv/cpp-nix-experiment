# default.nix
# "https://nix.dev/tutorials/callpackage" # interdependent-package-sets
let
  nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/24.11";
  pkgs = import nixpkgs { };
  callPackage = pkgs.lib.callPackageWith (pkgs // packages);
  packages = {
    # httplib = pkgs.callPackage ./libraries/httplib.nix { };
    # libtorch = pkgs.callPackage ./libraries/torch/lib.nix { };
  };
in
packages
