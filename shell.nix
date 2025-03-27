with (import <nixpkgs> { });
mkShell {
  # BuildInputs vs NativeBuildInputs "https://discourse.nixos.org/t/use-buildinputs-or-nativebuildinputs-for-nix-shell/8464/2"
  buildInputs = [
    opencv
  ];
}
