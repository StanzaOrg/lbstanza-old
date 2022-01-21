{ pkgs ? import <nixpkgs> { } }:
rec {
  lbstanza-bin = with pkgs; callPackage ./lbstanza-bin { };
  lbstanza = with pkgs; callPackage ./lbstanza { inherit lbstanza-bin; };
}
