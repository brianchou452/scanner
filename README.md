# Scanner

這是一個使用 C 語言撰寫的簡易 Scanner（詞法分析器）專案，主要功能是將輸入的原始碼文件內容，
依照預先定義的規則（如 identifier、reserved word、數字、運算子等）切割並轉換為 Tokens（詞法單元），以方便後續的 Parser（語法分析器）進行處理。

## 編譯

這個專案使用 CMake 進行編譯。請確保你的系統已經安裝了 CMake。
如果你還沒有安裝 CMake，可以參考 [CMake 官方網站](https://cmake.org/download/) 進行安裝。

```bash
cmake -S . -B build && cmake --build build
```

## 執行

```bash
# ./build/scanner <input_file> <output_file>
./build/scanner scanner.c output.txt
```

## 單元測試

本專案使用 Google Test 進行測試。

```bash
cd build
./scanner_test
```

可以再執行以下指令產生 code coverage report：

```bash
cd build
make coverage
```

可以在 `build/coverage_report` 目錄下找到 `index.html`，使用瀏覽器打開即可查看。

> 要產生 code coverage report，請先安裝 `lcov`。
> mac 使用者可以使用 Homebrew 安裝：`brew install lcov`

## 用到的專案

- Super Light Regular Expressions [Github](https://github.com/aquefir/slre)
- Google Test [Github](https://github.com/google/googletest)

## License

本 Scanner 專案使用 GNU GPL v2.0 釋出。
