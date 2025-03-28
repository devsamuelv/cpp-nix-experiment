# hello.nix
{
  stdenv,
  fetchzip,
  lib,
  fetchFromGitHub,
  brotli,
  cmake,
}:

stdenv.mkDerivation rec {
  pname = "cpp-httplib";
  version = "2.12";

  src = fetchFromGitHub {
    owner = "devsamuelv";
    repo = "${pname}";
    rev = "84c850f29919d4cf40f3515a3799813c5f4ea45a";
    sha256 = "sha256-qEmKyQElp4Zpvag62q5l3qHd9J7i2w094OVGjBsTVPI=";
  };

  nativeBuildInputs = [ cmake ];
  buildInputs = [
    brotli
  ];

  # Instruct the build process to run tests.
  # The generic builder script of `mkDerivation` handles all the default
  # command lines of several build systems, so it knows how to run our tests.
  doCheck = true;
}
