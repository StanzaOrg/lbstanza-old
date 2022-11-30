
# Stanza version to download in the form 0_12_34 matching what's in the zip filename
STANZA_VERSION := 0_17_44

# detect platform
SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring linux, $(SYS)))
  PLATFORM := LINUX
  STANZA_ZIP := lstanza_$(STANZA_VERSION).zip
else ifneq (, $(findstring mingw, $(SYS)))
  PLATFORM := WINDOWS
  STANZA_ZIP := wstanza_$(STANZA_VERSION).zip
else ifneq (, $(findstring darwin, $(SYS)))
  PLATFORM := OS_X
  STANZA_ZIP := stanza_$(STANZA_VERSION).zip
else
  PLATFORM := UNKNOWN
  $(error Unknown platform: $(SYS))
endif

CURL := curl
CURL_ARGS := -LsS
UNZIP := unzip -q

BUILD_DIR := build
STANZA_DIR := stanza
STANZA_URL := http://lbstanza.org/resources/stanza/$(STANZA_ZIP)

STANZA := stanza/stanza
VERBOSE := -verbose

.ONESHELL:
.PHONY: help all clean clean-stanza clean-all


all: asmjit-app


# create build dir
$(BUILD_DIR):
	mkdir build

# download and unzip stanza
$(STANZA_DIR):
	@echo "Downloading $(STANZA_ZIP)"
	$(RM) -r $(STANZA_DIR)
	$(CURL) $(CURL_ARGS) -o $(STANZA_ZIP) $(STANZA_URL)
	$(UNZIP) -d $(STANZA_DIR) $(STANZA_ZIP)
	$(RM) $(STANZA_ZIP)

# install/configure stanza
.stanza: | $(STANZA_DIR)
	cd $(STANZA_DIR)
	./stanza install -platform linux -path ..
	cd ..


# link final executable
$(BUILD_DIR)/asmjit-app: stanza.proj | .stanza $(STANZA_DIR) $(BUILD_DIR)
	export STANZA_CONFIG="$$PWD"
	$(STANZA) compile asmjit-app -o $(BUILD_DIR)/asmjit-app $(VERBOSE)

# convenience target
asmjit-app: $(BUILD_DIR)/asmjit-app
	ls -l $(BUILD_DIR)/asmjit-app


# clean
clean:
	rm -rf $(BUILD_DIR) asmjit-app

clean-stanza:
	rm -rf $(STANZA_DIR) .stanza

clean-all: clean clean-stanza
	rm -rf .conan conanfile.txt
