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
        pkgs = (
          import nixpkgs {
            inherit system;
          }
        );
      in
      {
        packages.cpp-httplib = pkgs.stdenv.mkDerivation {
          pname = "cpp-httplib";
          version = "2.12";

          src = nixpkgs.legacyPackages.x86_64-linux.fetchFromGitHub {
            owner = "devsamuelv";
            repo = "cpp-httplib";
            rev = "629644ef9765ab33284498b2daa6842fc32c9ab6";
            sha256 = "sha256-gUgt/0gnAytPYQksYVwPQF9Pk/+TOCuxdYkmU9ocLy4=";
          };

          nativeBuildInputs = [ pkgs.cmake ];
          buildInputs = [ ];

          # Instruct the build process to run tests.
          # The generic builder script of `mkDerivation` handles all the default
          # command lines of several build systems, so it knows how to run our tests.
          doCheck = true;

        };
      }
    );
}
