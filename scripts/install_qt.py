# -*- coding: utf-8 -*-
"""
一键安装 Qt 6.12.0 (MinGW) + GCC 13.1 工具链（清华镜像源）

用法:  py scripts/install_qt.py
产出:  E:\\Qt\\6.12.0\\mingw_64   (Qt 库)
       E:\\Qt\\Tools\\mingw1310_64  (编译器 gdb 等)
"""
import hashlib
import shutil
import sys
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path

try:
    import py7zr
except ImportError:
    sys.exit("缺少 py7zr，请先: py -m pip install aqtinstall")

BASE = "https://mirrors.tuna.tsinghua.edu.cn/qt/online/qtsdkrepository/windows_x86/desktop"
QT_REPO = f"{BASE}/qt6_6120/qt6_6120_mingw"
TOOL_REPO = f"{BASE}/tools_mingw1310"

QT_DIR = Path(r"E:\Qt\6.12.0\mingw_64")
TOOL_DIR = Path(r"E:\Qt\Tools\mingw1310_64")
CACHE = Path(__file__).parent / ".cache"

# 包名 -> 提取目的地；SKIP 里是要跳过的档案（省流量）
PKGS = {
    f"{QT_REPO}": [
        ("qt.qt6.6120.win64_mingw", QT_DIR),
        ("qt.qt6.6120.addons.qtimageformats.win64_mingw", QT_DIR),
    ],
    f"{TOOL_REPO}": [
        ("qt.tools.win64_mingw1310", TOOL_DIR),
    ],
}
SKIP_ARCHIVES = ("qtdoc-", "debug", "meta.7z")


def fetch(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists():
        print(f"  已缓存: {dest.name}")
        return
    print(f"  下载: {url}")
    with urllib.request.urlopen(url, timeout=60) as r, open(dest, "wb") as f:
        shutil.copyfileobj(r, f, 1024 * 256)


def verify(archive: Path, url: str) -> None:
    sha_url = url + ".sha1"
    with urllib.request.urlopen(sha_url, timeout=30) as r:
        expected = r.read().decode().split()[0].strip()
    actual = hashlib.sha1(archive.read_bytes()).hexdigest()
    if expected.lower() != actual.lower():
        archive.unlink(missing_ok=True)
        sys.exit(f"SHA1 校验失败: {archive.name} (期望 {expected}, 实际 {actual})")
    print(f"  校验OK: {archive.name}")


def parse_updates(repo: str, wanted: dict) -> list:
    with urllib.request.urlopen(f"{repo}/Updates.xml", timeout=30) as r:
        root = ET.fromstring(r.read())
    out = []
    for pu in root.iter("PackageUpdate"):
        name = pu.findtext("Name")
        if name in wanted:
            ver = pu.findtext("Version")
            for a in (pu.findtext("DownloadableArchives") or "").split(","):
                a = a.strip()
                if a and not any(s in a for s in SKIP_ARCHIVES):
                    out.append((name, ver, a))
    return out


def main() -> None:
    for repo, pkgs in PKGS.items():
        wanted = {n: d for n, d in pkgs}
        print(f"\n== 仓库 {repo.split('/')[-1]} ==")
        entries = parse_updates(repo, wanted)
        missing = set(wanted) - {e[0] for e in entries}
        if missing:
            sys.exit(f"Updates.xml 中找不到包: {missing}")
        for name, ver, arch in entries:
            dest_dir = wanted[name]
            # 官方仓库的档案命名规则：版本号作为文件名前缀（无子目录）
            url = f"{repo}/{name}/{ver}{arch}"
            f = CACHE / name / arch
            fetch(url, f)
            verify(f, url)
            print(f"  解压 -> {dest_dir}")
            dest_dir.mkdir(parents=True, exist_ok=True)
            with py7zr.SevenZipFile(f) as z:
                z.extractall(dest_dir)
    print("\n全部完成 ✅")
    print(f"Qt:   {QT_DIR}")
    print(f"编译器: {TOOL_DIR / 'bin' / 'g++.exe'}")


if __name__ == "__main__":
    main()
