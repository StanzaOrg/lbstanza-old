{ lib
, stdenv
, fetchzip
, autoPatchelfHook
}:

stdenv.mkDerivation rec {
  pname = "lbstanza-bin";
  version = "0.15.1";

  src = fetchzip {
    url = "http://lbstanza.org/resources/stanza/lstanza_${lib.replaceStrings ["."] ["_"] version}.zip";
    sha256 = "sha256-Ogfe5HX+fk3Tth5J07fWngyHnQjbkYBxKObqsNY8spc=";
    stripRoot=false;
  };

  nativeBuildInputs = [ autoPatchelfHook ];

  installPhase = ''
    runHook preInstall

    mkdir -p "$out/bin"
    cp -r * "$out"
    cp "$out/stanza" "$out/bin/stanza"

    runHook postInstall
  '';

  meta = with lib; {
    description = "L.B. Stanza Programming Language - stage 1";
    homepage = "http://lbstanza.org/";
    maintainers = [ "Scott Hamilton <sgn.hamilton+nixpkgs@protonmail.com>" ];
    platforms = platforms.linux;
  };
}
