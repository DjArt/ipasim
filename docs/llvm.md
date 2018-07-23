# LLVM and Clang

This document describes submodules `deps/llvm` and `deps/clang`.
They were forked from <https://git.llvm.org/git/llvm.git/> and <https://git.llvm.org/git/clang.git/>, respectively, using `git clone --mirror` and `git push --mirror`.

## Microsoft patches

Then, Microsoft patches 0009-0019 from `deps/WinObjC/contrib/clang` were applied to Clang using `git am`.
They are applied in branch `microsoft` which is based on `google/stable` (that's some time after `release_60`, where patches prior to 0009 have been already applied by Microsoft).
In LLVM, there were no patches to apply, but branch `microsoft` was created nevertheless, based on `stable`, for consistency.
**TODO: Maybe base it on [`RELEASE_601` tag](http://llvm.org/viewvc/llvm-project/llvm/tags/RELEASE_601/final/), instead of `stable` branch.**

> There are also old branches named `port`, not currently used.
> Before using them again (if changing something in LLVM or Clang), delete them first.

## Building

Follow the instructions below to build patched LLVM and Clang.
**TODO: See and somehow use also repo `clang-build`.**

- Make sure you have installed CMake.
- Run these commands from Developer Command Prompt inside `deps/llvm`:
  **TODO: Build only projects (and `LLVM_TARGETS_TO_BUILD`) that are necessary.**
  **TODO: Maybe also build it in `Release` configuration, so that it is faster...**

```cmd
mkdir build && cd build
cmake -G "Visual Studio 15" -DLLVM_TARGETS_TO_BUILD="X86;ARM" -DLLVM_EXTERNAL_CLANG_SOURCE_DIR="..\..\clang" ..
msbuild /m "/t:CMakePredefinedTargets\ALL_BUILD" /p:Configuration=Debug /p:Platform=Win32 .\LLVM.sln
```

- The output will be in `deps/llvm/build/Debug/`.

## Porting

### Comment keywords

- `[ipasim-objc-runtime]` - We are adding a new runtime called `ipasim` that derives from the `ios` runtime and introduces changes that the `microsoft` runtime introduced.
- `[dllimport]` - See section "Objective-C symbols across DLLs".

### Objective-C symbols across DLLs

Currently, Clang works pretty well when configured to use Apple's Objective-C runtime and COFF object file format.
This is actually surprising since this combination doesn't really make sense in the real world.
However, it obviously starts making sense as soon as our runtime is involved.
And then, we will see that it doesn't work perfectly, so we need to make some changes to Clang.

One such thing is `__declspec(dllimport)`ing Objective-C classes.
Clang actually recognizes this attribute and sets it correctly for the corresponding `GlobalVariable` (see `CGObjCNonFragileABIMac::GetClassGlobal` in `CGObjCMac.cpp`).
But, this information is later not considered when emitting the `GlobalVariable` in `TargetMachine::getSymbol` in `AsmPrinter::getSymbol` in `AsmPrinter::EmitGlobalVariable` in `AsmPrinter::doFinalization`.