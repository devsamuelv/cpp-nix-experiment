{
  inputs = {
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
    torch = {
      url = "path:libraries/torch";
    };
    httplib = {
      url = "path:libraries/httplib";
    };
  };
  outputs =
    {
      nixpkgs,
      flake-utils,
      torch,
      httplib,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = (
          import nixpkgs {
            inherit system;
          }
        );
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "teleop-control";
          version = "1.0.0";
          src = ./.;

          cmakeFlags = [
            "-DGTK2_GLIBCONFIG_INCLUDE_DIR=${pkgs.glib.out}/lib/glib-2.0/include"
            "-DGTK2_GDKCONFIG_INCLUDE_DIR=${pkgs.gtk2.out}/lib/gtk-2.0/include"
          ];

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.makeWrapper
            pkgs.glib
            torch.packages.x86_64-linux.libtorch
            httplib.packages.${pkgs.system}.cpp-httplib
            pkgs.gtk2
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
            pkgs.gdb
          ];
        };
      }
    );
}
