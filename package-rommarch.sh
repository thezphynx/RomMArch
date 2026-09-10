#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
archive_url="https://buildbot.libretro.com/stable/1.17.0/nintendo/3ds/RetroArch_cia.7z"
archive_sha256="eba3fd09c5cb5698e38db73dccbeab7ccc683bed27ede80a43fb1ed91e49e3c8"

if [[ $# -ne 0 ]]; then
    printf 'Usage: bash package-rommarch.sh\n' >&2
    exit 1
fi

for tool in curl sha256sum; do
    command -v "$tool" >/dev/null || {
        printf 'Required tool missing: %s\n' "$tool" >&2
        exit 1
    }
done

sevenzip=""
for tool in 7zz 7z 7za; do
    if command -v "$tool" >/dev/null; then
        sevenzip="$(command -v "$tool")"
        break
    fi
done

if [[ -z "$sevenzip" && -f "/c/Program Files/7-Zip/7z.exe" ]]; then
    sevenzip="/c/Program Files/7-Zip/7z.exe"
fi

if [[ -z "$sevenzip" ]]; then
    printf 'Install 7-Zip and make its command available, then retry.\n' >&2
    exit 1
fi

for frontend in retroarch_3ds.cia retroarch_3ds.3dsx; do
    if [[ ! -f "$repo_dir/$frontend" ]]; then
        printf 'Build the frontend first; missing: %s\n' "$frontend" >&2
        exit 1
    fi
done

work_dir="$(mktemp -d "$HOME/RomMArch-download.XXXXXX")"
trap 'rm -rf -- "$work_dir"' EXIT

printf 'Downloading compatible RetroArch 1.17.0 CIA distribution...\n'
curl --fail --location --retry 3 \
    --output "$work_dir/RetroArch_cia.7z" "$archive_url"

(
    cd "$work_dir"
    printf '%s  RetroArch_cia.7z\n' "$archive_sha256" | sha256sum -c -
)

"$sevenzip" x "$work_dir/RetroArch_cia.7z" \
    "-o$work_dir/extracted" -y

support_dir="$work_dir/extracted/retroarch_cia/retroarch"

for folder in assets cheats cores database filters overlays remaps; do
    if [[ ! -d "$support_dir/$folder" ]]; then
        printf 'Archive is missing expected folder: %s\n' "$folder" >&2
        exit 1
    fi
done

package_dir="$(mktemp -d "$HOME/RomMArch-package.XXXXXX")"
mkdir "$package_dir/retroarch"

for folder in assets cheats cores database filters overlays remaps; do
    cp -a "$support_dir/$folder" "$package_dir/retroarch/"
done

cp "$repo_dir/retroarch_3ds.cia" "$repo_dir/retroarch_3ds.3dsx" "$package_dir/"
cp "$repo_dir/COPYING" "$package_dir/"

cat > "$package_dir/INSTALL.txt" <<'NOTES'
RomMArch 3DS — Nintendo 3DS CIA package

INSTALLATION
1. Copy the supplied retroarch folder to the SD card root:
   sdmc:/retroarch/
2. Copy retroarch_3ds.cia onto the SD card and install it using FBI.
3. Open RomMArch and launch the desired core through the frontend.
   Allow the frontend to install the bundled core on first launch.
4. Configure RomMArch's server, API token, ROM directory, and save sync.

The retroarch_3ds.3dsx file is an alternative frontend launch format.
It is not required for the CIA installation.

CORE COMPATIBILITY
RomMArch packages Nintendo 3DS emulator cores
and supporting files from the RetroArch 1.17.0 CIA distribution.
Later core builds tested by the maintainer exhibited touchscreen
crashes that remain unresolved in this project. RetroArch 1.17.0
remains the supported baseline until a newer version is
explicitly validated.

Not every bundled core has been individually tested.
The bundled core executables retain their original RetroArch frontend;
the RomMArch submenu belongs to the separately built frontend.

On an existing installation, back up your files before copying.
Merging folders can leave extra newer core installers behind.
Existing RetroArch settings may override default directory paths.
NOTES

printf 'Supporting distribution: %s\nSHA-256: %s\n' \
    "$archive_url" "$archive_sha256" > "$package_dir/SUPPORT-SOURCE.txt"

printf '\nPackage created: %s\n' "$package_dir"
