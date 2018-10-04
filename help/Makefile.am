@YELP_HELP_RULES@

HELP_ID = engrampa
HELP_FILES = index.docbook legal.xml
HELP_MEDIA = 					\
	figures/engrampa_home.png		\
	figures/engrampa_leftarrow.png	\
	figures/engrampa_main_window.png	\
	figures/engrampa_rightarrow.png	\
	figures/engrampa_uparrow.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
