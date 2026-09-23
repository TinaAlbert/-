# 构建与验证

## 依赖隔离

每个项目使用自己的 `.deps/`、`.cache/` 和 `build/`。`scripts/build.ps1` 仅在当前进程内临时设置 `TEMP` / `TMP`，结束后恢复；不会使用全局包管理器安装依赖。

SDL2 固定版本为 2.32.6，来源为官方 GitHub Release：

- 文件：`SDL2-devel-2.32.6-mingw.tar.gz`
- SHA-256：`2c5ef8cf20491649f7726d76bbfe487ffdd83e2a3a594c37734ac18bdddfec6b`
- 已解压到 `.deps/` 后，重复构建无需联网。

## MinGW 构建与测试

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Test
```

`-Test` 会编译并运行算法回归测试，再使用 SDL dummy video driver 分别启动四个示例场景，每个场景渲染两帧后退出。回归测试检查视口裁剪、圆对称性、凹多边形填充、点阵重复、Bézier 端点和立方体变换。

GUI 中的原生对话框不在无界面测试范围内。人工检查时应确认：颜色设置、无效数值输入、窗口缩放、鼠标绘制、模式切换、清空与导出。

## 可选：Visual Studio + CMake

将官方 **SDL2 2.32.6 VC 开发包**解压到本项目的 `.deps/SDL2-2.32.6-vc/`，令 `SDL2_DIR` 指向其中包含 `SDL2Config.cmake` 的 `cmake` 目录。例如：

```powershell
cmake -S . -B build/msvc -A x64 -DSDL2_DIR="$PWD/.deps/SDL2-2.32.6-vc/cmake"
cmake --build build/msvc --config Release
ctest --test-dir build/msvc -C Release --output-on-failure
.\build\msvc\Release\graphics-lab.exe
```

请按实际解压层级调整 `SDL2_DIR`。此入口用于已有开发环境，不自动安装 Visual Studio、SDK 或 CMake；此次维护未在 MSVC 环境实测。

非 Windows 环境下 CMake 仅构建独立算法测试，不构建 GUI。

## 生成预览图

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/export-previews.ps1
```

脚本调用程序的真实示例渲染路径，先输出 BMP 到 `.cache/previews/`，再使用 Windows 的 `System.Drawing` 无损转换为 `docs/images/` 下的 PNG。无需 Python 或额外图像库。

也可单独导出：

```powershell
.\build\graphics-lab.exe --scene 3 --export-demo build/cube.bmp
```

`--scene` 取值：1 图元，2 填充，3 立方体，4 曲线。`--export-demo` 运行时隐藏窗口并在输出后退出；输出路径的父目录须事先存在。

## 原课设与整理内容

原课设已经包含四类图形实验、像素缓冲区、Windows 对话框以及 `0827` 填充图案。本次整理保留这些内容，并增加：

- 相对路径构建脚本、固定版本依赖与忽略规则。
- 算法和交互代码分离，移除最新版本中的 IDE 缓存、机器路径配置和旧编译产物。
- 窗口标题提示、示例场景、画布导出、参数有效性检查。
- 颜色通道修复、圆对称性修复、屏幕外绘制范围限制和 SDL 资源创建失败处理。
- 算法回归测试和四场景无界面运行检查。

原始报告保留为历史材料，其界面和工程路径可能与当前整理版不同。原有提交历史未重写，因此旧缓存仍可存在于 Git 历史中；若只想下载最新源码，可使用 `git clone --depth 1`。
