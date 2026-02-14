{
  description = "OverWatch - Secret Detection & Notification System";

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

          shellHook = ''
            echo "╔═══════════════════════════════════════╗"
            echo "║   OverWatch Development Environment   ║"
            echo "╚═══════════════════════════════════════╝"
            echo ""
            echo "Available tools:"
            echo "  • CMake:  $(cmake --version | head -n1)"
            echo "  • GCC:    $(gcc --version | head -n1)"
            echo "  • Python: $(python --version)"
            echo ""
            echo "C++ libraries installed:"
            echo "  ✓ libcpr"
            echo "  ✓ nlohmann_json"
            echo "  ✓ yaml-cpp"
            echo "  ✓ spdlog"
            echo ""
            echo "Quick start:"
            echo "  cd scanner && cmake -B build && cmake --build build"
            echo ""
          '';
        };
      }
    );
}
