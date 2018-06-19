# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    =
SPHINXBUILD   = sphinx-build
SPHINXPROJ    = nRFConnectSDK
SOURCEDIR     = .
BUILDDIR      = _build

nrfINPUT      = ./nrf
zephyrINPUT   = ../../zephyr/doc

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

all: zephyr nrf

nrf: nrf-doxy nrf-html
nrf-doxy:
	doxygen $(nrfINPUT)/nrf.doxyfile
	@echo Built nrf doxygen.

nrf-extrafiles:
	mkdir -p $(nrfINPUT)/_samples
	cd ../samples; cp --parents `find -name \*.rst` ../doc/$(nrfINPUT)/_samples/
	@echo Copied nrf samples.

nrf-html: nrf-extrafiles
	$(eval DIRECTORY = $(subst -html,,$@))
	$(SPHINXBUILD) -M html "$($(DIRECTORY)INPUT)" "$(BUILDDIR)/$(DIRECTORY)" $(SPHINXOPTS) $(O) -c $(DIRECTORY)/ 2>&1 | tee -a $(DIRECTORY)_doc.log;
	@echo Build nrf HTML.

zephyr: zephyr-doxy zephyr-html

zephyr-doxy:
	cd $(zephyrINPUT); (cat zephyr.doxyfile ; echo "STRIP_FROM_PATH=${ZEPHYR_BASE}" ) | doxygen -  2>&1 | tee doc.log
	@echo Built zephyr doxygen.

zephyr-html: zephyr-extrafiles
	$(eval DIRECTORY = $(subst -html,,$@))
	$(SPHINXBUILD) -M html "$($(DIRECTORY)INPUT)" "$(BUILDDIR)/$(DIRECTORY)" $(SPHINXOPTS) $(O) -c $(DIRECTORY)/ 2>&1 | tee -a $(DIRECTORY)_doc.log;
	@echo Built zephyr HTML.

zephyr-extrafiles:
	cd $(zephyrINPUT); scripts/extract_content.py

nrf-clean zephyr-clean:
	$(eval DIRECTORY = $(subst -clean,,$@))
	rm -rf $(BUILDDIR)/$(DIRECTORY)/
	rm -f $(DIRECTORY)_doc.log

clean-nrf: nrf-clean
	rm -rf $(nrfINPUT)/_samples
	rm -rf $(nrfINPUT)/_doxygen

clean-zephyr: zephyr-clean
	rm -rf $(zephyrINPUT)/doxygen
	rm -rf $(zephyrINPUT)/samples
	rm -rf $(zephyrINPUT)/boards

clean-all: clean-nrf clean-zephyr
	@echo Cleaned build and temp directories.
