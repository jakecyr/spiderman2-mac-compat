.DEFAULT_GOAL := help
.PHONY: help flat highball vr mono stereo quest check
help:
	@./play help
flat highball vr mono stereo quest check:
	@./play $@
