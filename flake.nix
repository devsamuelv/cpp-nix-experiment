{
  inputs = {
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
  };
  outputs =
    {
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "teleop-control";
          version = "1.0.0";
          src = ./.;

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.makeWrapper
            pkgs.httplib
            (pkgs.opencv.override {
              enableJPEG = true;
              enableFfmpeg = true;
            })
          ];
          buildPhase = ''
            mkdir -p build
            cmake -S $src -B ./build -DCMAKE_BUILD_TYPE=Release
            cmake --build build
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp build/teleop-control $out/bin
          '';
        };

        devShell = pkgs.mkShell {
          buildInputs = [
            pkgs.cmake
            pkgs.gcc
            pkgs.clang
            pkgs.opencv
          ];
        };
      }
    );
}
