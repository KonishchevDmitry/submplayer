SUBDIRS = po src

ACLOCAL_AMFLAGS = -I m4

# Added by gettextize
EXTRA_DIST = build-aux/config.rpath


# gettext -->
.PHONY: gettext

gettext:
	make -C po update-po
	cd po || exit 1; \
	for lang in $$(cat LINGUAS | $(SED) 's/#.*$$//'); \
	do \
		msgmerge --force-po --no-wrap --no-fuzzy-matching --update --backup=off "$$lang.po" "@PACKAGE@" || exit 1; \
	done
## gettext <--


## loop -->
.PHONY: loop

loop:
	bash -c ' \
	\
	check_files=$$(find $(srcdir)/src -name "*.cpp"; find $(srcdir)/src -name "*.hpp"; find $(srcdir)/src -name "*.hh";); \
	\
	while true; \
	do \
		new_files=$$(ls -l $$check_files); \
		\
		if [ "$$files" != "$$new_files" ]; \
		then \
			make --no-print-directory --keep-going && echo "OK"; \
		fi; \
		\
		files="$$new_files"; \
		read -t 1 input && ./src/@PACKAGE@; \
	done'
## loop <--

