SOURCE_DIR := src
HEADER_DIR := src
BUILD_DIR := build
DEBUG_DIR := $(BUILD_DIR)/debug
DEBUG_TARGET := $(DEBUG_DIR)/paz
RELEASE_DIR := $(BUILD_DIR)/release
RELEASE_TARGET := $(RELEASE_DIR)/paz

CPPFLAGS := -I src/
DEBUG_FLAGS := -g -fsanitize=address,undefined
RELEASE_FLAGS := -O3

HEADERS := $(shell find $(SOURCE_DIR) -type f -name "*.h")
SOURCES := $(shell find $(SOURCE_DIR) -type f -name "*.c")
DEBUG_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(DEBUG_DIR)/%.o,$(SOURCES))
RELEASE_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(RELEASE_DIR)/%.o,$(SOURCES))

.PHONY: debug
debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG_FLAGS) $^ -o $@

$(DEBUG_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS)
	mkdir -p $(@D) && $(CC) -c $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@

.PHONY: release
release: $(RELEASE_TARGET)

$(RELEASE_TARGET): $(RELEASE_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(RELEASE_FLAGS) $^ -o $@

$(RELEASE_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS)
	mkdir -p $(@D) && $(CC) -c $(CPPFLAGS) $(CFLAGS) $(RELEASE_FLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf build/
