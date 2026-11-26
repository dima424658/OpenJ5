PYTHON ?= python3
CARGO ?= cargo
BOOTSTRAP_DIR ?= ../target/bootstrap
BOOTSTRAP_FRAMES := $(BOOTSTRAP_DIR)/combi-bootstrap-c-frames.bin
BOOTSTRAP_SOURCES := bootstrap/build_c.py \
                     bootstrap/c509/bootstrap.c bootstrap/c509/bootstrap.h

.PHONY: all desktop bootstrap test
.DELETE_ON_ERROR:

all: bootstrap
	JANVAR_BOOTSTRAP_FRAMES="$(abspath $(BOOTSTRAP_FRAMES))" $(CARGO) build --locked --release -p janvar-flash-cli

desktop: bootstrap
	JANVAR_BOOTSTRAP_FRAMES="$(abspath $(BOOTSTRAP_FRAMES))" $(CARGO) build --locked --release -p janvar-editor-desktop

bootstrap: $(BOOTSTRAP_FRAMES)

$(BOOTSTRAP_FRAMES): $(BOOTSTRAP_SOURCES)
	mkdir -p "$(BOOTSTRAP_DIR)"
	$(PYTHON) bootstrap/build_c.py "$(BOOTSTRAP_DIR)/bootstrap-c.ihx" --frames "$@"

test: bootstrap
	JANVAR_BOOTSTRAP_FRAMES="$(abspath $(BOOTSTRAP_FRAMES))" $(CARGO) test --locked -p janvar-flash -p janvar-flash-cli
