.DEFAULT_GOAL := help
.PHONY: help flat vr mono stereo check
help:
	@./play help
flat vr mono stereo check:
	@./play $@
