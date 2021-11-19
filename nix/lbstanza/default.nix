{ lib
, stdenv
, fetchFromGitHub
, lbstanza-bin
, nix-gitignore
}:

stdenv.mkDerivation rec {
  pname = "lbstanza";
  version = "master";

  src = nix-gitignore.gitignoreSource [] ../..;

  postPatch = ''
    patchShebangs scripts/*.sh
  '';

  preBuild = ''
    mkdir build
    export HOME=$(pwd)
    cat << EOF > .stanza
      install-dir = "${lbstanza-bin}"
      platform = linux
      aux-file = "mystanza.aux"
    EOF
  '';

  buildPhase = ''
    runHook preBuild
    ./scripts/make.sh stanza linux compile-clean
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    mkdir "$out"
    cp -r * "$out"
    mv "$out/lpkgs" "$out/pkgs"
    runHook postInstall
  '';

  nativeBuildInputs = [ lbstanza-bin ];

  meta = with lib; {
    description = "L.B. Stanza Programming Language";
    homepage = "http://lbstanza.org/";
    maintainers = [ "Scott Hamilton <sgn.hamilton+nixpkgs@protonmail.com>" ];
    platforms = platforms.linux;
  };
}
