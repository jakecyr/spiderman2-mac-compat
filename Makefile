.DEFAULT_GOAL := help
.PHONY: help flat vr mono stereo quest check
help:
	@./play help
flat vr mono stereo quest check:
	@./play $@
