# Japanese Helper

A **minimalistic** desktop application for Japanese vocabulary lookup and translation.

It has multiple API integrations, a classic-style user interface using FLTK, and speech-to-text. Compatible both with Windows and Linux.

## Screenshots:

![](images/screenshots/translator_bEI5b21csT.png)

![](images/screenshots/translator_bNfkxPuuSa.png)

## Usage:

For now, there are only two API integrations:

### Jisho

The **Jisho API** can be used to look up any Japanese words in all the writing systems (**Kanji**, **Hiragana**, **Katakana**, and even **Romaji**).

The **API** provides all Japanese words associated with the input text, their reading (in Hiragana), and their different meanings in English.

### 🔑 DeepL (Needs API Key)

This **API** simply translates anything in the input field from Japanese to English.

The **DeepL Translate API** can only be used with an **API key**. The free version supports up to **500,000 characters a month**.

## 🗣️ Speech Recognition:

Japanese Helper has a built-in speech recognition, allowing the user to transcribe Japanese speech into text that automatically goes into the search bar (input).

STT uses **whisper.cpp** with the `base` model. Additional models can be downloaded and configured manually.
For more information on downloading models, please see `README.md` for **whisper.cpp** or click [here](https://github.com/ggml-org/whisper.cpp/blob/master/README.md#:~:text=Then%2C%20download%20one%20of%20the%20Whisper%20models).

You can also modify the settings and paramaters for **whisper.cpp** and its structs in `speech_to_text.cpp`.
Notably, you can change the language that **whisper.cpp** takes in as input.

## 🖇️ Dependencies:

If you're planning to build this yourself, you'll need these:

- **CMake**

- **GNU Compiler Collection (GCC)**

- **FLTK (Fast Light Toolkit)**

- **libcurl (C/C++ Network Transfer Library)**

- **PortAudio (real-time audio I/O)**

**NOTE:** The `CMakeLists.txt` file sort of handles the acqusition of necessary dependencies.

## 🔨 Building:

There is a `CMakeLists.txt` in the root directory of the project. It can be used to build the program as is, however if you plan on making any changes, be sure to update it if necessary.

### Step-By-Step Instructions:

1. First clone the repository.
- **Ensure** "--recurse-submodules" is included in the command to clone the necessary dependency repositories.
  
  ```bash
    git clone --recurse-submodules https://github.com/mk-babian/japanese-helper.git
  ```

- For building with a single command, run this (in the `root` directory):
  
  ```bash
  cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release && cmake --build build/ && ./build/translator
  ```
  
  ##### Note on Whisper.cpp:

- By default, Japanese Helper uses the `base` model of Whisper.cpp.

- You can use the command below to download the `base` model:
  
  ```bash
  cmake --build build/ --target download_whisper_model
  ```

  I am planning on adding a feature to download models straight from the settings window.

## ⚙️ Configuration & Misc:
Add your DeepL API key under `Settings → API`.

Your search history is saved to `history.json`. 
Hover to see more info, click to re-search, or wipe it clean from `Settings → History`.

Example of the JSON file:
```json
{
    "api": [
        0
    ],
    "search": [
        "ばか"
    ],
    "time": [
        "Wednesday, May 06 03:33 PM"
    ]
}
```
**NOTE:** The history file and buffer only hold a 100 entries. This can be changed by resizing the vectors in `main.cpp`.

## 🙏 Acknowledgments:

- Big thanks for Georgi Gerganov [(ggerganov)](https://github.com/ggerganov) and contributors of **whisper.cpp** for creating an accessible and high-performance automatic speech recognition (ASR) model 

## 📜 License:

PolyForm Noncommercial License 1.0.0 - See `LICENSE` file.

Copyright (c) 2025 Saba
