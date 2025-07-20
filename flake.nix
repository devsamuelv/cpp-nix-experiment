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
        packages.default =
          with pkgs.stdenv;
          mkDerivation {
            pname = "teleop-control";
            version = "1.0.0";
            src = ./.;

            cmakeFlags = [
              "-DGTK2_GLIBCONFIG_INCLUDE_DIR=${pkgs.glib.out}/lib/glib-2.0/include"
              "-DGTK2_GDKCONFIG_INCLUDE_DIR=${pkgs.gtk2.out}/lib/gtk-2.0/include"
              "-DGTK2_GDKCONFIG_INCLUDE_DIR=${pkgs.gtk2.out}/lib/gtk-2.0/include"
              "-DCMAKE_MODULE_PATH=$CMAKE_MODULE_PATH;${pkgs.opencv.out}/lib/cmake/opencv4"
            ];

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.makeWrapper
              pkgs.glib
              pkgs.gtk2
              (pkgs.opencv.override {
                enableJPEG = true;
                enableFfmpeg = true;
              })

              # x86_64-linux only
              torch.packages.x86_64-linux.libtorch
              httplib.packages.x86_64-linux.cpp-httplib
            ];
            buildInputs = [
              pkgs.gtk2
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

        devShells.default =
          with pkgs;
          mkShell {
            TORCH_LIBRARIES = "${torch.packages.x86_64-linux.libtorch.out}/lib";
            CMAKE_MODULE_PATH = "${pkgs.opencv.out}/lib/cmake/opencv4";

            packages = [
              pkgs.cmake
              pkgs.gcc
              pkgs.glib
              pkgs.gtk2
              pkgs.clang
              (pkgs.opencv.override {
                enableJPEG = true;
                enableFfmpeg = true;
              })

              # x86_64-linux only
              torch.packages.x86_64-linux.libtorch
              httplib.packages.x86_64-linux.cpp-httplib
              pkgs.gdb
            ];
          };
      }
    );
}
