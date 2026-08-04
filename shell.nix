{
  pkgs ? import <nixpkgs> { },
}:

let
  qtEnv =
    with pkgs.qt6;
    env "game-qt-${qtbase.version}" [
      qtwayland
    ];
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    pkg-config
    gcc
    gdb
  ];

  buildInputs = with pkgs; [
    qtEnv
    libglvnd
  ];

  LD_LIBRARY_PATH = "/run/opengl-driver/lib";

  shellHook = ''
    echo "game development shell"
    echo "Qt:    $(qmake --version 2>/dev/null | tail -n 1 || true)"
    echo "CMake: $(cmake --version | head -n 1)"
    echo
    echo "Configure: cmake -S . -B build -G Ninja"
    echo "Build:     cmake --build build"
    echo "Test:      ctest --test-dir build --output-on-failure"
    echo "Run:       ./build/game"
  '';
}
