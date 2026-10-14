# intended to be overriden to use release profile. For example:
# `$ make lib PROFILE=release`
PROFILE := debug

CFLAGS := -Wall -Wextra -Werror -Wno-sign-compare
ARFLAGS := rcs

ifeq ($(PROFILE),debug)
CFLAGS += -g -fsanitize=address,undefined
else ifeq ($(PROFILE),release)
CFLAGS += -O3
else
$(error invalid profile, pick debug (default) or release)
endif
BUILD_DIR := build/$(PROFILE)

SOURCES := $(shell find src -type f -name "*.c")
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
SUBFILES := $(patsubst src/%.c,$(BUILD_DIR)/%.d,$(SOURCES))

.PHONY: lib
lib: $(BUILD_DIR)/libpaz.a

$(BUILD_DIR)/libpaz.a $(BUILD_DIR)/lib/%.d: CPPFLAGS := -I src/lib
$(BUILD_DIR)/libpaz.a: $(filter $(BUILD_DIR)/lib/%,$(OBJECTS))
	$(AR) $(ARFLAGS) $@ $^

.PHONY: app
app: $(BUILD_DIR)/paz

$(BUILD_DIR)/paz $(BUILD_DIR)/app/%.d: CPPFLAGS := -I src/lib -I src/app
$(BUILD_DIR)/paz: $(BUILD_DIR)/libpaz.a $(filter $(BUILD_DIR)/app/%,$(OBJECTS))
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: test
test: $(BUILD_DIR)/testpaz

$(BUILD_DIR)/testpaz $(BUILD_DIR)/test/%.d: CPPFLAGS := -I src/lib -I src/test
$(BUILD_DIR)/testpaz: $(BUILD_DIR)/libpaz.a $(filter $(BUILD_DIR)/test/%,$(OBJECTS))
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: test_run
test_run: test
	$(BUILD_DIR)/testpaz

$(BUILD_DIR)/%.d: src/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -MM $< -MT $(patsubst %.d,%.o,$@) -o $@
	echo '	$$(CC) -c $$(CPPFLAGS) $$(CFLAGS) $$< -o $$@' >> $@

include $(SUBFILES)
