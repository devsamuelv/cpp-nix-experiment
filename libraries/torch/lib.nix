{
  callPackage,
  stdenv,
  fetchzip,
  lib,
  libcxx,
  glib,
  gtk2,
  llvmPackages,
  config,
  patchelf,
  fixDarwinDylibNames,
}:

# The binary libtorch distribution statically links the CUDA
# toolkit. This means that we do not need to provide CUDA to
# this derivation. However, we should ensure on version bumps
# that the CUDA toolkit for `passthru.tests` is still
# up-to-date.
stdenv.mkDerivation rec {
  version = "2.7.0";
  unavailable = throw "libtorch is not available for this platform";
  pname = "libtorch";
  srcs = import ./binary-hashes.nix version;

  src = {
    name = "libtorch-cxx11-abi-shared-with-deps-2.7.0-cpu.zip";
    url = "" "https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.7.0%2Bcpu.zip" "";
    hash = "sha256-c93177bb7715024cc3f563778e3aa627b159f685d0cad1d54e334be8c188b4";
  };

  nativeBuildInputs =
    if stdenv.hostPlatform.isDarwin then
      [ fixDarwinDylibNames ]
    else
      [ patchelf ] ++ lib.optionals cudaSupport [ addDriverRunpath ];

  dontBuild = true;
  dontConfigure = true;
  dontStrip = true;

  cmakeFlags = [
    "-DGTK2_GLIBCONFIG_INCLUDE_DIR=${glib.out}/lib/glib-2.0/include"
    "-DGTK2_GDKCONFIG_INCLUDE_DIR=${gtk2.out}/lib/gtk-2.0/include"
  ];

  installPhase = ''
    # Copy headers and CMake files.
    mkdir -p $dev
    cp -r include $dev
    cp -r share $dev

    install -Dm755 -t $out/lib lib/*${stdenv.hostPlatform.extensions.sharedLibrary}*

    # We do not care about Java support...
    rm -f $out/lib/lib*jni* 2> /dev/null || true

    # Fix up library paths for split outputs
    substituteInPlace $dev/share/cmake/Torch/TorchConfig.cmake \
      --replace \''${TORCH_INSTALL_PREFIX}/lib "$out/lib" \

    substituteInPlace \
      $dev/share/cmake/Caffe2/Caffe2Targets-release.cmake \
      --replace \''${_IMPORT_PREFIX}/lib "$out/lib" \
  '';

  postFixup =
    let
      rpath = lib.makeLibraryPath [ stdenv.cc.cc ];
    in
    lib.optionalString stdenv.hostPlatform.isLinux ''
      find $out/lib -type f \( -name '*.so' -or -name '*.so.*' \) | while read lib; do
        echo "setting rpath for $lib..."
        patchelf --set-rpath "${rpath}:$out/lib" "$lib"
        ${lib.optionalString cudaSupport ''
          addDriverRunpath "$lib"
        ''}
      done
    ''
    + lib.optionalString stdenv.hostPlatform.isDarwin ''
      for f in $out/lib/*.dylib; do
          otool -L $f
      done
      for f in $out/lib/*.dylib; do
        if otool -L $f | grep "@rpath/libomp.dylib" >& /dev/null; then
          install_name_tool -change "@rpath/libomp.dylib" ${llvmPackages.openmp}/lib/libomp.dylib $f
        fi
        install_name_tool -id $out/lib/$(basename $f) $f || true
        for rpath in $(otool -L $f | grep rpath | awk '{print $1}');do
          install_name_tool -change $rpath $out/lib/$(basename $rpath) $f
        done
        if otool -L $f | grep /usr/lib/libc++ >& /dev/null; then
          install_name_tool -change /usr/lib/libc++.1.dylib ${libcxx-for-libtorch.outPath}/lib/libc++.1.0.dylib $f
        fi
      done
      for f in $out/lib/*.dylib; do
          otool -L $f
      done
    '';

  outputs = [
    "out"
    "dev"
  ];

  # passthru.tests.cmake = callPackage ./test {
  #   inherit cudaSupport;
  # };

  meta = with lib; {
    description = "C++ API of the PyTorch machine learning framework";
    homepage = "https://pytorch.org/";
    sourceProvenance = with sourceTypes; [ binaryNativeCode ];
    # Includes CUDA and Intel MKL, but redistributions of the binary are not limited.
    # https://docs.nvidia.com/cuda/eula/index.html
    # https://www.intel.com/content/www/us/en/developer/articles/license/onemkl-license-faq.html
    license = licenses.bsd3;
    maintainers = with maintainers; [ junjihashimoto ];
    platforms = [
      "aarch64-darwin"
      "x86_64-linux"
    ];
  };
}
