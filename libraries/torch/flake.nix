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
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        packages.libtorch = pkgs.stdenv.mkDerivation {
          pname = "libtorch";
          version = "2.7.0";
          src = nixpkgs.legacyPackages.x86_64-linux.fetchzip {
            name = "libtorch-cxx11-abi-shared-with-deps-2.7.0-cpu.zip";
            url = "https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.7.0%2Bcpu.zip";
            sha256 = "sha256-8REMU+E0DZQDRUw1zx0K5oMqVsTBJ8g88dqnLpUfcjM=";
          };

          nativeBuildInputs = [ pkgs.patchelf ];

          cmakeFlags = [
            "-DGTK2_GLIBCONFIG_INCLUDE_DIR=${pkgs.glib.out}/lib/glib-2.0/include"
            "-DGTK2_GDKCONFIG_INCLUDE_DIR=${pkgs.gtk2.out}/lib/gtk-2.0/include"
          ];

          installPhase = ''
            # Copy headers and CMake files.
            mkdir -p ./dev
            cp -r include ./dev
            cp -r share ./dev

            install -Dm755 -t $out/lib lib/*${pkgs.stdenv.hostPlatform.extensions.sharedLibrary}*

            # We do not care about Java support...
            rm -f $out/lib/lib*jni* 2> /dev/null || true

            # Fix up library paths for split outputs
            substituteInPlace ./dev/share/cmake/Torch/TorchConfig.cmake \
              --replace \''${TORCH_INSTALL_PREFIX}/lib "$out/lib" \

            substituteInPlace \
              ./dev/share/cmake/Caffe2/Caffe2Targets-release.cmake \
              --replace \''${_IMPORT_PREFIX}/lib "$out/lib" \

            cp -r ./dev/share $out/share
            cp -r ./dev/include $out/include
          '';

          outputs = [
            "out"
          ];

        };
      }
    );
}
