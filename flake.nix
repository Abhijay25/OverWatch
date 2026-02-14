{
  description = "OverWatch";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Build tools
            cmake
            gcc
            pkg-config

            # C++ libraries
            libcpr
            nlohmann_json
            yaml-cpp
            spdlog

            # Python for the bot
            python311
            python311Packages.pip
            python311Packages.virtualenv

            # Utilities
            git
          ];
        };
      }
    );
}
