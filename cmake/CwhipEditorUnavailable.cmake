message(FATAL_ERROR
    "Cwhip Editor is unavailable in this configuration. Install pkg-config, GTK 4 and "
    "GtkSourceView 5 development files, then reconfigure with "
    "-DCWHIP_GTK=ON -DCWHIP_GTK_SOURCEVIEW=ON. "
    "Compiler-only builds can keep both options OFF.")
