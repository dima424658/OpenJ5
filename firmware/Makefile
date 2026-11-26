LD8051 ?= sdld
SDCC ?= sdcc
PYTHON ?= python3

IMAGE := build/J5v05l19
REFERENCE ?= ../bins/J5v05l19.bin
include mk/layout.mk

C_FLAGS := -mmcs51 --model-small
C_INCLUDES := -I src/include
BUILD_RULES := Makefile mk/layout.mk mk/sources.mk mk/compiler.mk
IMAGE_TOOL := scripts/verify_image.py
C_OBJECTS := $(addprefix build/,$(addsuffix .rel,$(C_FUNCTIONS) $(C_ENTRY_VECTORS) $(C_VECTORS) $(C_DATA))) build/rom_layout.rel
C_PLACEMENT := $(foreach address,$(C_FUNCTIONS) $(C_ENTRY_VECTORS) $(C_VECTORS),-b code_$(address)=0x$(address)) \
	$(foreach address,$(C_DATA),-b const_$(address)=0x$(address))

.PHONY: all verify clean
.DELETE_ON_ERROR:

all: $(IMAGE).bin

# Each translation unit keeps its original section and object identifier.
define compile_section
build/$(1).rel: build/%.rel: $(2) $(BUILD_RULES)
	mkdir -p build
	$$(SDCC) $$(C_FLAGS) $$(C_INCLUDES) $$(C_OPTIONS) --codeseg code_$$* --constseg const_$$* -c "$$<" -o "$$@"
endef

include mk/sources.mk
include mk/compiler.mk

$(IMAGE).ihx: $(C_OBJECTS)
	$(LD8051) -n -i -m -w $(C_PLACEMENT) "$(IMAGE)" $(C_OBJECTS)

$(IMAGE).map: $(IMAGE).ihx
	@test -s "$@" || $(LD8051) -n -i -m -w $(C_PLACEMENT) "$(IMAGE)" $(C_OBJECTS)

$(IMAGE).bin: $(IMAGE).ihx $(IMAGE).map $(IMAGE_TOOL)
	$(PYTHON) $(IMAGE_TOOL) convert "$<" "$@"

verify: $(IMAGE).bin
	$(PYTHON) $(IMAGE_TOOL) verify "$(IMAGE).bin" "$(IMAGE).map" "$(REFERENCE)"

clean:
	rm -rf build
