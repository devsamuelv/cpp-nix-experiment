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
    # Nix Ros Overlay
    nix-ros-overlay = {
      url = "github:lopsided98/nix-ros-overlay/master";
    };
    nixpkgs = {
      follows = "nix-ros-overlay/nixpkgs";
    };
  };
  outputs =
    {
      nixpkgs,
      flake-utils,
      torch,
      httplib,
      nix-ros-overlay,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = (
          import nixpkgs {
            inherit system;
            overlays = [ nix-ros-overlay.overlays.default ];
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

              # ROS Packages
              pkgs.colcon
              pkgs.rosPackages.humble.ros-core
              # ... other non-ROS packages
              (
                with pkgs.rosPackages.humble;
                buildEnv {
                  paths = [
                    image-transport
                    # ... other ROS packages
                  ];
                }
              )
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
              # Generic DevTools
              pkgs.cmake
              pkgs.gcc
              pkgs.glib
              pkgs.gtk2
              pkgs.clang
              # Has clangd which is important to find other libraries
              pkgs.clang-tools
              pkgs.vscode
              pkgs.gdb

              # Vision
              (pkgs.opencv.override {
                enableJPEG = true;
                enableFfmpeg = true;
              })

              # x86_64-linux only
              torch.packages.x86_64-linux.libtorch
              httplib.packages.x86_64-linux.cpp-httplib

              # ROS Packages
              pkgs.colcon
              pkgs.rosPackages.humble.ros-core
              # ... other non-ROS packages
              (
                with pkgs.rosPackages.humble;
                buildEnv {
                  paths = [
                    image-transport
                    # ... other ROS packages
                  ];
                }
              )
            ];
          };
      }
    );
}
