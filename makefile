# Minimal Makefile for Git Bash / Unix helpers. On Windows, prefer build.ps1 or build.bat.
.PHONY: help clean 68mix

help:
	@echo "Use: build.ps1 (Windows) or build.bat  |  68mix: bash build-68mix.sh  |  make clean"
	@echo "Set GDK_WIN to your SGDK root, or use _compilers/sgdk"

68mix:
	bash build-68mix.sh

clean:
	-rm -f build/genesis/*.o build/genesis/*.bin build/genesis/*.log
	-rm -rf server/target
	-rm -f clients/genesis/out/*
