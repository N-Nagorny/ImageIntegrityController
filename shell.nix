{}:

with import <nixpkgs> {};
with libsForQt512;

stdenv.mkDerivation {
  name = "ImageIntegrityController";
  buildInputs = [ opencv3 qtbase ];
  nativeBuildInputs = [ cmake gnumake pkg-config valgrind wrapQtAppsHook ];
  src = ./.;
  shellHook = ''
    setQtEnvironment=$(mktemp)
    random=$(openssl rand -base64 20 | sed "s/[^a-zA-Z0-9]//g")
    makeWrapper "$(type -p sh)" "$setQtEnvironment" "''${qtWrapperArgs[@]}" --argv0 "$random"
    sed "/$random/d" -i "$setQtEnvironment"
    source "$setQtEnvironment"
  '';
}
