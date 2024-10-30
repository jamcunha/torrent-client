{
  description = "C/C++ Development Environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell.override
          {
            # Override stdenv in order to change compiler:
            # stdenv = pkgs.clangStdenv;
          }
          {
            packages = with pkgs; [
              clang-tools

              # cmake
              gnumake

              # Check this package
              # cppcheck

              # For documentation
              # doxygen

              # For testing
              # gtest
              criterion

              # Code coverage
              # lcov
            ] ++ (if system == "aarch64-darwin" then [ ] else [ gdb valgrind ]);
          };
      });
    };
}
