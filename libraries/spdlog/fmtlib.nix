{ nixpkgs, pkgs }:

pkgs.clangStdenv.mkDerivation rec {
  pname = "libfmt";
  version = "12.1.0";
  src = nixpkgs.legacyPackages.x86_64-linux.fetchFromGitHub {
    owner = "fmtlib";
    repo = "fmt";
    rev = "407c905e45ad75fc29bf0f9bb7c5c2fd3475976f";
    sha256 = "sha256-ZmI1Dv0ZabPlxa02OpERI47jp7zFfjpeWCy1WyuPYZ0=";
  };

  nativeBuildInputs = [ pkgs.cmake pkgs.makeWrapper pkgs.glib ];
  buildInputs = [ ];

  buildPhase = ''
    mkdir -p build
    cmake -S $src -B ./build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=TRUE
    cmake --build build
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp -r build/ $out/bin
  '';
}
