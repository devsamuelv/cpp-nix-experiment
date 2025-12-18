{
  inputs = { flake-utils = { url = "github:numtide/flake-utils"; }; };
  outputs = { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = (import nixpkgs { inherit system; });
        libfmt = (import ./fmtlib.nix {
          nixpkgs = nixpkgs;
          pkgs = pkgs;
          inherit ;
        });
      in {
        packages.default = pkgs.clangStdenv.mkDerivation {
          pname = "spdlog";
          version = "1.16.0";
          src = nixpkgs.legacyPackages.x86_64-linux.fetchFromGitHub {
            owner = "gabime";
            repo = "spdlog";
            rev = "486b55554f11c9cccc913e11a87085b2a91f706f";
            sha256 = "sha256-VB82cNfpJlamUjrQFYElcy0CXAbkPqZkD5zhuLeHLzs=";
          };

          cmakeFlags = [ ];

          nativeBuildInputs = [ pkgs.cmake pkgs.makeWrapper pkgs.glib ];
          buildInputs = [ libfmt ];

          buildPhase = ''
            mkdir -p build
            cmake -S $src -B ./build -DCMAKE_BUILD_TYPE=Release
            cmake --build build
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp -r build/ $out/bin
          '';
        };
      });
}
