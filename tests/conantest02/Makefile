
BUILD_DIR := build
CURL := curl
CURL_ARGS := -LsS
STANZA_DIR := stanza
STANZA_ZIP := lstanza_0_17_27.zip
STANZA_URL := http://lbstanza.org/resources/stanza/${STANZA_ZIP}
UNZIP := unzip -q

STANZA := stanza/stanza

.ONESHELL:
.PHONY: help all stanza-download stanza-install conan-setup conan-install clean


all: stanza conan asmjit-app


stanza: stanza-download stanza-install

stanza-download:
	@echo "Downloading ${STANZA_ZIP}"
	rm -rf ${STANZA_DIR}
	${CURL} ${CURL_ARGS} -o ${STANZA_ZIP} ${STANZA_URL}
	${UNZIP} -d ${STANZA_DIR} ${STANZA_ZIP}
	rm -f ${STANZA_ZIP}

stanza-install:
	cd ${STANZA_DIR}
	./stanza install -platform linux -path ..
	cd ..

conan: conan-setup conan-install

conan-setup:
	export CONAN_USER_HOME="$$PWD"
	conan config set general.revisions_enabled=1
	conan remote clean
	conan remote add jitx-conan-remote http://ec2-44-226-24-141.us-west-2.compute.amazonaws.com:9300
	conan remote add conancenter https://center.conan.io
	conan remote list

conan-install:
	export CONAN_USER_HOME="$$PWD"
	conan install . --install-folder ${BUILD_DIR} --build never

%: %.stanza
	mkdir -p ${BUILD_DIR}
	export STANZA_CONFIG="$$PWD"
	export CONAN_USER_HOME="$$PWD"
	${STANZA} compile $< -o ${BUILD_DIR}/$@

clean:
	rm -rf ${BUILD_DIR}

clean-conan:
	rm -rf .conan

clean-stanza:
	rm -rf ${STANZA_DIR} .stanza

clean-all: clean clean-conan clean-stanza
