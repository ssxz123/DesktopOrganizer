# 桌面整理工具（DesktopOrganizer）

C++20 + Qt 6 Widgets 开发的 Windows 桌面整理工具（练手项目）。
按内置类别（图片/文档/视频/音频/压缩包/程序/其他）或自定义规则把桌面文件归档，
支持预览、撤销、实时监控自动整理。详见 [设计方案.md](设计方案.md)。

## 构建与运行（VS Code）

打开项目文件夹 → 底部状态栏选 **Qt-MinGW** 工具包（来自 `CMakePresets.json`）→ ⚙️ 构建 → ▶️ 运行。

```bash
# 命令行等价操作（cmake/ninja 来自 pip）
py -m cmake --preset Qt-MinGW
py -m cmake --build --preset Qt-MinGW
```

构建产物在项目文件夹下的 `build/` 目录。
`desktop_organizer.exe` 已用 windeployqt 部署 Qt DLL，可直接双击运行。

## 运行测试

```bash
"C:/Users/HUAWEI/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/bin/ctest.exe" \
  --test-dir E:/ymh/ymh_learning/DesktopOrganizer/build --output-on-failure
```

4 个 Qt Test 测试：classifier / rules / organizer / undo_log（用 QTemporaryDir 构造场景，不碰真实桌面）。

## 目录结构

```
├─ CMakeLists.txt / CMakePresets.json   # 顶层构建
├─ core/                  # organizer_core 静态库（只依赖 Qt6::Core，可独立测试）
│  ├─ classifier.*        # 扩展名 → 类别
│  ├─ rules.*             # 自定义规则：扩展名/通配/N 天前 → 目标目录
│  ├─ organizer.*         # 扫描 + 移动计划 + 执行（冲突自动改名 name (1).ext）
│  ├─ undo_log.*          # JSON 撤销日志，重启后仍可撤销
│  ├─ config.*            # rules.json / settings.json（%APPDATA%/DesktopOrganizer）
│  └─ desktop_watch.*     # QFileSystemWatcher + 防抖定时器
├─ app/                   # desktop_organizer（Qt Widgets GUI）
│  ├─ main_window.*       # 预览表格 / 一键整理 / 撤销
│  ├─ rules_dialog.*      # 规则编辑 + 类别映射
│  └─ settings_dialog.*   # 自动整理开关 / 防抖时长 / 白名单
└─ tests/                 # Qt Test 单元测试（只测 core）
```

## 使用说明

- **预览**：扫描桌面，表格中可取消勾选个别文件
- **执行整理**：移动勾选文件（只移动，不复制不删除），重名自动改名
- **撤销上次**：按反向顺序移回最近一次整理（原位置被占也会自动改名）
- **整理规则**：自定义规则按列表顺序优先匹配，高于内置分类；可改类别目标目录
- **设置**：开启实时监控后，桌面变更防抖 2 秒自动归档；白名单文件永不移动

桌面根目录的文件夹不会被移动；整理不递归进已有文件夹。
