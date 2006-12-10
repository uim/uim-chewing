EXTRA_DIST = $(SCM_FILES)
uim_plugin_datadir = @UIM_SCMDIR@
uim_plugin_data_DATA = $(SCM_FILES)
SCM_FILES = chewing.scm chewing-custom.scm chewing-key-custom.scm

install-data-hook:
	uim-module-manager --register chewing
