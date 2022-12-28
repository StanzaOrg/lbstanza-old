{ pkgs ? import <nixpkgs> {} }:
let
  customPython = pkgs.python38.buildEnv.override {
    extraLibs = with pkgs.python38Packages; [
      # dateutil
      packaging
      # requests
    ];
  };
in
with pkgs; mkShell {
  buildInputs = [
    customPython
  ];
  shellHook = ''
    run(){
      python nix/update-lbstanza-bin/update-lbstanza-bin.py
    }
    # For the CI
    commit(){
      git config --global user.name 'SCOTT-HAMILTON'
      git config --global user.email 'sgn.hamilton+github@protonmail.com'
      git add nix/lbstanza-bin/default.nix
      git commit -m "Automated nix/lbstanza-bin CI update"
      git push
    }
    run_and_commit(){
      run || commit
    }
  '';
}

