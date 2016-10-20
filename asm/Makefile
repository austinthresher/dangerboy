# Copyright (C) 2014-2016 Joonas Javanainen <joonas.javanainen@gmail.com>
#
# This software may be modified and distributed under the terms
# of the MIT license. See the LICENSE file for details.

# This file is a modified portion of mooneye-gb
# The original file, and the rest of the project, can be viewed at
# https://github.com/Gekkio/mooneye-gb

WLA ?= wla-gb
WLALINK ?= wlalink

BUILD_PATH := tests

SRC := $(filter-out ./common/%.s,$(shell find . -type f -name '*.s'))

all: $(addprefix $(BUILD_PATH)/, $(patsubst %.s,%.gb, $(SRC)))
	@rm $(BUILD_PATH)/*.sym

$(BUILD_PATH)/%.o: %.s common/*.s
	@mkdir -p $(BUILD_PATH)/$(dir $<)
	@cd $(dir $<) && $(WLA) $(WLAFLAGS) -o $(abspath $@) $(notdir $<)

$(BUILD_PATH)/%.link: $(BUILD_PATH)/%.o
	@mkdir -p $(dir $<)
	@echo "[objects]\n$(notdir $<)" > $@

$(BUILD_PATH)/%.gb: $(BUILD_PATH)/%.link
	@mkdir -p $(dir $<)
	@cd $(dir $<) && $(WLALINK) -S $(notdir $<) $(abspath $@)
	@echo --- $(notdir $(basename $@))

clean:
	@rm -rf $(BUILD_PATH)

.PHONY: clean all
