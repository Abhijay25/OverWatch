{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # Build tools
    cmake
    gcc
    pkg-config

    # C++ libraries
    cpr
    nlohmann_json
    yaml-cpp
    spdlog

    # Python for the bot
    python311
    python311Packages.pip
    python311Packages.virtualenv

    # Git for version control
    git
  ];

  shellHook = ''
    echo "OverWatch Development Environment"
    echo "================================="
    echo "C++ Scanner dependencies: ✓"
    echo "Python available: $(python --version)"
    echo ""
    echo "Ready to build!"
  '';
}
