# GW2 Framework - 独立游戏框架
# 由 丫丫 亲手打造 (Loong's Custom Build)

## 🚀 特性
- **零依赖**: 不依赖任何第三方插件加载器
- **全功能 API**: 集成 MumbleLink + 内存读取
- **内置 UI**: DirectX 11 + ImGui
- **热重载**: 支持代码热更新

## 🛠️ 编译
```bash
cd C:\Users\Loong\.openclaw\workspace\GW2Framework
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

## 📦 部署
将 `build\Release\GW2Framework.dll` 重命名为 `d3d11.dll`，放入游戏根目录 `E:\Games\Guild Wars 2\`。

## 🎮 使用
启动游戏，按 **F12** 呼出调试菜单。
