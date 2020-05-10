

ビルドコマンドは以下

```
# Visual Studio
cl main.cpp -Fetrash.exe -link Ole32.lib Shell32.lib

# clang
clang++ main.cpp -o trash.exe -l Ole32 -l Shell32
```
