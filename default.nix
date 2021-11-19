{ pkgs ? import <nixpkgs> { } }:
let
  lbstanza-bin = with pkgs; callPackage ./nix/lbstanza-bin { };
  lbstanza = with pkgs; callPackage ./nix/lbstanza { inherit lbstanza-bin; };
in
  lbstanza
