import requests
import zipfile
from pathlib import Path

DIRECT_ZIP_URL = "https://cdrdv2.intel.com/v1/dl/getContent/830833"

ROOT = Path(__file__).resolve().parents[1]
ASSETS_DIR = ROOT / "content"
ZIP_PATH = ASSETS_DIR / "sponza.zip"
SPONZA_DIR = ASSETS_DIR / "sponza"

HEADERS = {
    "User-Agent": "Mozilla/5.0",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.9",
    "Referer": "https://www.intel.com/",
    "Connection": "keep-alive",
}


def download(url: str, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)

    if out_path.exists():
        print("[OK] Using cached zip")
        return

    print("[DL] Downloading Sponza...")

    r = requests.get(url, headers=HEADERS, stream=True)

    if r.status_code != 200:
        raise RuntimeError(f"Download failed: HTTP {r.status_code}")

    with open(out_path, "wb") as f:
        for chunk in r.iter_content(chunk_size=1024 * 1024):
            if chunk:
                f.write(chunk)

    print("[OK] Download complete")


def extract(zip_path: Path, out_dir: Path):
    if out_dir.exists() and any(out_dir.iterdir()):
        print("[OK] Already extracted")
        return

    print("[EXTRACT] Extracting...")

    out_dir.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(zip_path, "r") as z:
        z.extractall(out_dir)

    print("[OK] Done")


def main():
    download(DIRECT_ZIP_URL, ZIP_PATH)
    extract(ZIP_PATH, SPONZA_DIR)

    print("\n✅ Sponza ready at:", SPONZA_DIR)


if __name__ == "__main__":
    main()