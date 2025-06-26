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
            rev = "84c850f29919d4cf40f3515a3799813c5f4ea45a";
            sha256 = "sha256-qEmKyQElp4Zpvag62q5l3qHd9J7i2w094OVGjBsTVPI=";
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
