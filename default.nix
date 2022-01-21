{ pkgs ? import <nixpkgs> { } }:
let
  myNix = import ./nix { inherit pkgs; };
in
myNix.lbstanza
