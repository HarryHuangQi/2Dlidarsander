# 贡献指南

欢迎提交问题、修复和改进。

## 提交前建议

- 确认代码能正常构建
- 尽量保持改动聚焦，避免把无关格式化混进来
- 如果修改了通信地址、端口或引脚，请同步更新 [README.md](README.md)

## 本地验证

建议至少执行以下命令：

```bash
idf.py build
```

如果涉及串口、网络或传感器逻辑，建议再进行一次真机烧录验证：

```bash
idf.py flash monitor
```

## 代码风格

- 继续沿用现有 ESP-IDF / C 风格
- 保持日志输出清晰，便于排查传感器和网络问题
- 新增模块时尽量在对应组件目录下补充说明文档

## 提交信息

建议使用简洁明确的提交信息，例如：

- `fix: adjust imu udp port`
- `docs: update readme`
- `feat: add ld14 data note`
